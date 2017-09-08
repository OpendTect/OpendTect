/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/


#include "seisloader.h"

#include "arrayndalgo.h"
#include "binidvalset.h"
#include "cbvsreadmgr.h"
#include "convmemvalseries.h"
#include "datapackbase.h"
#include "nrbytes2string.h"
#include "ioobj.h"
#include "od_ostream.h"
#include "odsysmem.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "prestackgather.h"
#include "samplingdata.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisprovider.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "threadwork.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

#include <string.h>


namespace Seis
{

static bool addComponents( RegularSeisDataPack& dp, const IOObj& ioobj,
			   TypeSet<int>& selcomponents, uiString& msg )
{
    BufferStringSet cnames;
    SeisIOObjInfo::getCompNames( ioobj.key(), cnames );
    const int nrcomp = selcomponents.size();
    od_int64 totmem, freemem;
    OD::getSystemMemory( totmem, freemem );
    NrBytesToStringCreator nbstr( totmem );
    const od_uint64 reqsz = nrcomp * dp.sampling().totalNr() *
			    dp.getDataDesc().nrBytes();
			    // dp.nrKBytes() cannot be used before allocation

    BufferString memszstr( nbstr.getString( reqsz ) );
    if ( reqsz >= freemem )
    {
	msg = od_static_tr("Seis::addComponents",
		"Insufficient memory for allocating %1").arg( memszstr );
	return false;
    }

    msg = od_static_tr("Seis::addComponents","Allocating %1").arg( memszstr );
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	const int cidx = selcomponents[idx];
	const char* cnm = cnames.size()>1 && cnames.validIdx(cidx) ?
			  cnames.get(cidx).buf() : BufferString::empty().buf();

	// assembles composite "<attribute>|<component>" name
	const StringPair compstr( ioobj.name().str(), cnm );
	if ( !dp.addComponent(compstr.getCompString(),false) )
	    return false;
    }

    return true;
}

}; // namespace Seis


Seis::Loader::Loader( const IOObj& ioobj, const TrcKeyZSampling* tkzs,
		      const TypeSet<int>* components )
    : ioobj_(ioobj.clone())
    , tkzs_(false)
    , dc_(OD::AutoFPRep)
    , outcomponents_(0)
    , scaler_(0)
    , seissummary_(0)
    , trcssampling_(0)
{
    compscalers_.allowNull( true );
    if ( tkzs )
	tkzs_ = *tkzs;

    if ( components )
	setComponents( *components );
}


Seis::Loader::~Loader()
{
    delete ioobj_;
    deepErase( compscalers_ );
    delete outcomponents_;
    delete scaler_;
    delete seissummary_;
    delete trcssampling_;
}


void Seis::Loader::setDataChar( DataCharacteristics::UserType type )
{ dc_ = DataCharacteristics(type); }


void Seis::Loader::setComponents( const TypeSet<int>& components )
{
    components_ = components;
    for ( int idx=0; idx<components.size(); idx++ )
	compscalers_ += 0;
}


void Seis::Loader::setComponentScaler( const Scaler& scaler, int compidx )
{
    if ( scaler.isEmpty() )
	return;

    for ( int idx=0; idx<=compidx; idx++ )
    {
	if ( !compscalers_.validIdx(idx) )
	    compscalers_ += 0;
    }

    delete compscalers_.replace( compidx, scaler.clone() );
}


bool Seis::Loader::setOutputComponents( const TypeSet<int>& compnrs )
{
    if ( compnrs.size() != components_.size() )
	return false;

    delete outcomponents_;
    outcomponents_ = new TypeSet<int>( compnrs );

    return true;
}


void Seis::Loader::setScaler( const Scaler* newsc )
{
    delete scaler_;
    scaler_ = newsc ? newsc->clone() : 0;
}


uiString Seis::Loader::nrDoneText() const
{
    return uiStrings::phrJoinStrings( uiStrings::sTrace(mPlural), tr("read") );
}


void Seis::Loader::adjustDPDescToScalers( const BinDataDesc& trcdesc )
{
    const DataCharacteristics floatdc( OD::F32 );
    const BinDataDesc floatdesc( floatdc );
    if ( dp_ && dp_->getDataDesc() == floatdesc )
	return;

    bool needadjust = false;
    for ( int idx=0; idx<compscalers_.size(); idx++ )
    {
	if ( !compscalers_[idx] || compscalers_[idx]->isEmpty() ||
	     trcdesc == floatdesc )
	    continue;

	needadjust = true;
	break;
    }

    if ( !needadjust )
	return;

    setDataChar( floatdc.userType() );
    if ( !dp_ )
	return;

    BufferStringSet compnms;
    for ( int idx=0; idx<dp_->nrComponents(); idx++ )
	compnms.add( dp_->getComponentName(idx) );

    dp_->setDataDesc( floatdesc ); //Will delete incompatible arrays
    for ( int idx=0; idx<compnms.size(); idx++ )
	dp_->addComponent( compnms.get(idx).str(), false );
}


void Seis::Loader::submitUdfWriterTasks( int queueid )
{
    if ( trcssampling_->totalSize() >= tkzs_.hsamp_.totalNr() )
	return;

    TaskGroup* udfwriters = new TaskGroup;
    for ( int idx=0; idx<dp_->nrComponents(); idx++ )
    {
	udfwriters->addTask(
	new Array3DUdfTrcRestorer<float>( *trcssampling_, tkzs_.hsamp_,
					  dp_->data(idx) ) );
    }

    Threads::WorkManager::twm().addWork(
	    Threads::Work(*udfwriters,true),0,queueid,false,false,true);
}



Seis::ParallelFSLoader3D::ParallelFSLoader3D( const IOObj& ioobj,
					      const TrcKeyZSampling& tkzs )
    : Seis::Loader(ioobj,&tkzs,0)
    , bidvals_(0)
    , totalnr_(tkzs.hsamp_.totalNr())
{
    seissummary_ = new ObjectSummary( ioobj );
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() ) \
						  .arg( ioobj_->uiName() ) );
}


Seis::ParallelFSLoader3D::ParallelFSLoader3D( const IOObj& ioobj,
					      BinIDValueSet& bidvals,
					      const TypeSet<int>& components )
    : Seis::Loader(ioobj,0,&components)
    , bidvals_( &bidvals )
    , totalnr_(bidvals.totalSize())
{
    seissummary_ = new ObjectSummary( ioobj );
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() ) \
						  .arg( ioobj_->uiName() ) );
}


void Seis::ParallelFSLoader3D::setDataPack( RegularSeisDataPack* dp )
{
    dp_ = dp;
}


uiString Seis::ParallelFSLoader3D::nrDoneText() const
{ return Seis::Loader::nrDoneText(); }

uiString Seis::ParallelFSLoader3D::message() const
{ return Seis::Loader::message(); }


bool Seis::ParallelFSLoader3D::doPrepare( int nrthreads )
{
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

    const SeisIOObjInfo& seisinfo = seissummary_->getFullInformation();
    TrcKeyZSampling seistkzs( tkzs_ );
    seisinfo.getRanges( seistkzs );
    const DataCharacteristics datasetdc( seissummary_->getDataChar() );
    if ( dc_.userType() == OD::AutoFPRep )
	setDataChar( datasetdc.userType() );

    if ( components_.isEmpty() )
    {
	const int nrcomponents = seisinfo.nrComponents();
	TypeSet<int> components;
	for ( int idx=0; idx<nrcomponents; idx++ )
	    components += idx;

	setComponents( components );
    }

    adjustDPDescToScalers( datasetdc );

    if ( bidvals_ )
    {
	pErrMsg("The bidval-code is not tested. Run through step by step, make "
		"sure everything is OK and remove this warning.");
	const int nrvals = 1+components_.size();
        if ( bidvals_->nrVals()!=nrvals )
	{
	    if ( !bidvals_->setNrVals( nrvals ) )
	    {
		msg_ = tr("Cannot allocate memory");
		return false;
	    }
	}
    }
    else if ( !dp_ )
    {
	dp_ = new RegularSeisDataPack( VolumeDataPack::categoryStr(true,false),
				       &dc_ );
	dp_->setName( ioobj_->name() );
	dp_->setSampling( tkzs_ );
	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( !addComponents(*dp_,*ioobj_,components_,msg_) )
	    return false;

	delete trcssampling_;
	trcssampling_ = new PosInfo::CubeData();
	uiRetVal uirv;
	PtrMan<IOObj> ioobj = ioobj_->clone();
	PtrMan<Seis::Provider> prov =
			       Seis::Provider::create(ioobj->key(),&uirv);
	mDynamicCastGet(Provider3D*,prov3d,prov.ptr())
	if ( !prov3d )
	    { msg_ = uirv; return false; }

	prov3d->getGeometryInfo( *trcssampling_ );
	trcssampling_->limitTo( tkzs_.hsamp_ );
	const od_int64& totalnr = totalnr_;
	const_cast<od_int64&>( totalnr ) = trcssampling_->totalSize();
	dp_->setTrcsSampling( new PosInfo::SortedCubeData(*trcssampling_) );

	submitUdfWriterTasks( Threads::WorkManager::cDefaultQueueID() );
    }

    return true;
}


bool Seis::ParallelFSLoader3D::doWork(od_int64 start,od_int64 stop,int)
{
    uiRetVal uirv;
    PtrMan<IOObj> ioobj = ioobj_->clone();
    PtrMan<Seis::Provider> prov = Seis::Provider::create(ioobj->key(),&uirv);
    if ( !prov )
	{ msg_ = uirv; return false; }

    //Set ranges, at least z-range
    const TypeSet<int>& outcompnrs = outcomponents_
				   ? *outcomponents_
				   : components_;
    TrcKeySamplingIterator iter;
    BinIDValueSet::SPos bidvalpos;
    BinID curbid;
    if ( bidvals_ )
    {
        bidvalpos = bidvals_->getPos( start );
        if ( !bidvalpos.isValid() )
            return false;
    }
    else
    {
	iter.setSampling( tkzs_.hsamp_ );
	iter.setCurrentPos( start );
    }

    SeisTrc trc;
    uirv = prov->goTo( TrcKey(tkzs_.hsamp_.trcKeyAt(start)) );
    if ( !uirv.isOK() )
	{ msg_ = uirv; return false; }

#define mUpdateInterval 100
    int nrdone = 0;
    for ( od_int64 idx=start; idx<=stop; idx++, nrdone++ )
    {
	if ( nrdone>mUpdateInterval )
	{
	    addToNrDone( nrdone );
	    nrdone = 0;

	    if ( !shouldContinue() )
		return false;
	}

	if ( ( bidvals_ && !bidvals_->next(bidvalpos,false) ) ||
	     ( !bidvals_ && !iter.next() ) )
	    return false;

	curbid = bidvals_ ? bidvals_->getBinID( bidvalpos )
			  : iter.curBinID();

	uirv = prov->getNext( trc );
	if ( uirv.isOK() && trc.info().binID()==curbid )
        {
	    const StepInterval<float> trczrg = trc.zRange();

            if ( bidvals_ )
            {
		float* vals = bidvals_->getVals(bidvalpos);
		const float z = vals[0];

		if ( !mIsUdf(z) && trczrg.includes( z, false ) )
		{
		    for ( int idcx=outcompnrs.size()-1; idcx>=0; idcx-- )
		    {
			const int idc = outcompnrs[idcx];
			const float rawval = trc.getValue(z,components_[idcx]);
			vals[idc+1] = compscalers_[idcx]
			    ? mCast(float,compscalers_[idcx]->scale(rawval) )
			    : rawval;
		    }
		}
            }
            else
            {
		const int inlidx = tkzs_.hsamp_.inlIdx( curbid.inl() );
		const int crlidx = tkzs_.hsamp_.crlIdx( curbid.crl() );

		for ( int idz=dp_->sampling().nrZ()-1; idz>=0; idz--)
		{
		    float val;
		    const double z = tkzs_.zsamp_.atIndex( idz );
		    if ( trczrg.includes( z, false ) )
		    {
			for ( int idcx=outcompnrs.size()-1; idcx>=0; idcx-- )
			{
			    val = trc.getValue( (float) z, components_[idcx] );
			    if ( !mIsUdf(val) )
			    {
				const int idc = outcompnrs[idcx];
				if ( idc < dp_->nrComponents() )
				{
				    if ( compscalers_[idcx] )
				    {
					val = mCast(float,compscalers_[idcx]->
								scale(val) );
				    }

				    Array3D<float>& arr3d = dp_->data( idc );
				    arr3d.set( inlidx, crlidx, idz, val);
				}
			    }
			}
		    }
		}
            }
        }
    }

    addToNrDone( nrdone );

    return true;
}



// ParallelFSLoader2D (probably replace by a SequentialFSLoader)
Seis::ParallelFSLoader2D::ParallelFSLoader2D( const IOObj& ioobj,
					      const TrcKeyZSampling& tkzs,
					      const TypeSet<int>* comps )
    : Seis::Loader(ioobj,&tkzs,comps)
    , dpclaimed_(false)
    , totalnr_(tkzs.hsamp_.totalNr())
{
    seissummary_ = new ObjectSummary( ioobj );
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() ) \
						  .arg( ioobj_->uiName() ) );
}


uiString Seis::ParallelFSLoader2D::nrDoneText() const
{ return Seis::Loader::nrDoneText(); }

uiString Seis::ParallelFSLoader2D::message() const
{ return Seis::Loader::message(); }


bool Seis::ParallelFSLoader2D::doPrepare( int nrthreads )
{
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

    const SeisIOObjInfo& seisinfo = seissummary_->getFullInformation();
    TrcKeyZSampling seistkzs( tkzs_ );
    seisinfo.getRanges( seistkzs );
    const DataCharacteristics datasetdc( seissummary_->getDataChar() );
    if ( dc_.userType() == OD::AutoFPRep )
	setDataChar( datasetdc.userType() );

    const Pos::GeomID geomid( tkzs_.hsamp_.getGeomID() );
    if ( components_.isEmpty() )
    {
	const int nrcomponents = seisinfo.nrComponents( geomid );
	TypeSet<int> components;
	for ( int idx=0; idx<nrcomponents; idx++ )
	    components += idx;

	setComponents( components );
    }

    dp_ = new RegularSeisDataPack( VolumeDataPack::categoryStr(true,true),
				   &dc_ );
    dp_->setName( ioobj_->name() );
    dp_->setSampling( tkzs_ );
    if ( scaler_ && !scaler_->isEmpty() )
	dp_->setScaler( *scaler_ );

    if ( !addComponents(*dp_,*ioobj_,components_,msg_) )
	return false;

    delete trcssampling_;
    trcssampling_ = new PosInfo::CubeData();
    uiRetVal uirv;
    PtrMan<IOObj> ioobj = ioobj_->clone();
    PtrMan<Seis::Provider> prov = Seis::Provider::create(ioobj->key(),&uirv);
    mDynamicCastGet(Provider2D*,prov2d,prov.ptr())
    if ( !prov2d )
	{ msg_ = uirv; return false; }

    PosInfo::Line2DData line2ddata;
    prov2d->getGeometryInfo( prov2d->lineNr(geomid), line2ddata );
    PosInfo::LineData* linedata = new PosInfo::LineData( geomid );
    line2ddata.limitTo( tkzs_.hsamp_.trcRange() );
    line2ddata.getSegments( *linedata );
    trcssampling_->add( linedata );
    trcssampling_->limitTo( tkzs_.hsamp_ );
    const od_int64& totalnr = totalnr_;
    const_cast<od_int64&>( totalnr ) = trcssampling_->totalSize();
    dp_->setTrcsSampling( new PosInfo::SortedCubeData(*trcssampling_) );

    submitUdfWriterTasks( Threads::WorkManager::cDefaultQueueID() );

    return true;
}


bool Seis::ParallelFSLoader2D::doWork(od_int64 start,od_int64 stop,int threadid)
{
    if ( !dp_ || dp_->isEmpty() )
	return false;

    PtrMan<IOObj> ioobj = ioobj_->clone();
    const Seis2DDataSet dataset( *ioobj );
    const Pos::GeomID geomid( tkzs_.hsamp_.getGeomID() );
    if ( !dataset.isPresent(geomid) )
	return false;

    const BufferString fnm(
		    SeisCBVS2DLineIOProvider::getFileName( *ioobj, geomid ) );
    PtrMan<CBVSSeisTrcTranslator> trl =
			CBVSSeisTrcTranslator::make( fnm.str(), false, true );
    if ( !trl ) return false;

    SeisTrc trc;
    BinID curbid;
    StepInterval<int> trcrg = tkzs_.hsamp_.trcRange();
    trl->toStart();
    curbid = trl->readMgr()->binID();

    const TypeSet<int>& outcomponents = outcomponents_
				      ? *outcomponents_ : components_;

    const int nrzsamples = tkzs_.zsamp_.nrSteps()+1;
    for ( int cidx=0; cidx<outcomponents.size(); cidx++ )
    {
	const int idcin = components_[cidx];
	const Scaler* scaler = compscalers_[cidx];
	const int idcout = outcomponents[cidx];
	Array3D<float>& arr = dp_->data( idcout );
	ValueSeries<float>* stor = arr.getStorage();
	mDynamicCastGet(ConvMemValueSeries<float>*,storptr,stor);
	char* storarr = storptr ? storptr->storArr() : (char*)stor->arr();

	for ( int idy=mCast(int,start); idy<=stop; idy++, addToNrDone(1) )
	{
	    curbid.crl() = trcrg.atIndex( idy );
	    if ( trl->goTo(curbid) && trl->read(trc) )
	    {
		const BinDataDesc trcdatadesc =
			trc.data().getInterpreter(idcin)->dataChar();
		if ( storarr && dp_->getDataDesc()==trcdatadesc )
		{
		    const DataBuffer* rawseq = trc.data().getComponent(idcin);
		    const int bytespersamp = rawseq->bytesPerElement();
		    const od_int64 offset = arr.info().getOffset( 0, idy, 0 );
		    char* dststartptr = storarr + offset*bytespersamp;

		    for ( int zidx=0; zidx<nrzsamples; zidx++ )
		    {
			const float zval = tkzs_.zsamp_.atIndex( zidx );
			const int trczidx = trc.nearestSample( zval );
			const unsigned char* srcptr =
				rawseq->data() + trczidx*bytespersamp;
			char* dstptr = dststartptr + zidx*bytespersamp;
			// Checks if amplitude equals undef value of underlying
			// data type as the array is initialized with undefs.
			if ( !scaler && memcmp(dstptr,srcptr,bytespersamp) )
			    OD::sysMemCopy(dstptr,srcptr,bytespersamp );
			else
			{
			    const float rawval = trc.getValue( zval, idcin );
			    const float trcval = scaler
					? mCast(float,scaler->scale(rawval) )
					: rawval;
			    arr.set( 0, idy, zidx, trcval );
			}
		    }
		}
		else
		{
		    for ( int zidx=0; zidx<nrzsamples; zidx++ )
		    {
			const float zval = tkzs_.zsamp_.atIndex( zidx );
			const float rawval = trc.getValue( zval, idcin );
			const float trcval = scaler
					   ? mCast(float,scaler->scale(rawval) )
					   : rawval;
			arr.set( 0, idy, zidx, trcval );
		    }
		}
	    }
	}
    }

    return true;
}



RegularSeisDataPack* Seis::ParallelFSLoader2D::getDataPack()
{
    dpclaimed_ = true;
    return Seis::Loader::getDataPack();
}



// SequentialReader
namespace Seis
{

class ArrayFiller : public Task
{
public:
ArrayFiller( const RawTrcsSequence& rawseq, const StepInterval<float>& zsamp,
	     bool samedatachar, bool needresampling,
	     const TypeSet<int>& components,
	     const ObjectSet<Scaler>& compscalers,
	     const TypeSet<int>& outcomponents,
	     RegularSeisDataPack& dp, bool is2d )
    : rawseq_(rawseq)
    , zsamp_(zsamp)
    , samedatachar_(samedatachar)
    , needresampling_(needresampling)
    , components_(components)
    , compscalers_(compscalers)
    , outcomponents_(outcomponents)
    , dp_(dp),is2d_(is2d)
{
}


~ArrayFiller()
{
    delete &rawseq_;
}


bool execute()
{
    const int nrpos = rawseq_.nrPositions();
    for ( int itrc=0; itrc<nrpos; itrc++ )
    {
	if ( !doTrace(itrc) )
	    return false;
    }

    return true;
}

bool doTrace( int itrc )
{
    const TrcKey& tk = rawseq_.getPosition( itrc );
    const int idx0 = is2d_ ? 0 : dp_.sampling().hsamp_.lineIdx( tk.lineNr() );
    const int idx1 = dp_.sampling().hsamp_.trcIdx( tk.trcNr() );

    const int startidx0 = dp_.sampling().zsamp_.nearestIndex( zsamp_.start );
    const int nrzsamples = zsamp_.nrSteps()+1;
    const int trczidx0 = rawseq_.getZRange().nearestIndex( zsamp_.atIndex(0) );
    const int bytespersamp = dp_.getDataDesc().nrBytes();

    for ( int cidx=0; cidx<outcomponents_.size(); cidx++ )
    {
	const int idcin = components_[cidx];
	const Scaler* scaler = compscalers_[cidx];
	const int idcout = outcomponents_[cidx];
	Array3D<float>& arr = dp_.data( idcout );
	ValueSeries<float>* stor = arr.getStorage();
	float* storptr = stor ? stor->arr() : 0;
	mDynamicCastGet(ConvMemValueSeries<float>*,convmemstor,stor);
	char* storarr = convmemstor ? convmemstor->storArr()
				    : (char*)storptr;
	const od_int64 offset =  storarr ? arr.info().getOffset( idx0, idx1, 0 )
					 : 0;
	char* dststartptr = storarr ? storarr + offset*bytespersamp : 0;
	if ( storarr && samedatachar_ && !needresampling_ && !scaler )
	{
	    const unsigned char* srcptr = rawseq_.getData( itrc, idcin,
							   trczidx0 );
	    OD::sysMemCopy( dststartptr, srcptr, nrzsamples*bytespersamp );
	}
	else
	{
	    int startidx = startidx0;
	    od_int64 valueidx = stor ? offset+startidx : 0;
	    int trczidx = trczidx0;
	    float zval = zsamp_.start;
	    float* destptr = storptr ? (float*)dststartptr : 0;
	    for ( int zidx=0; zidx<nrzsamples; zidx++ )
	    {
		const float rawval = needresampling_
				   ? rawseq_.getValue( zval, itrc, idcin )
				   : rawseq_.get( trczidx++, itrc, idcin );
		if ( needresampling_ ) zval += zsamp_.step;
		const float trcval = scaler
				   ? mCast(float,scaler->scale(rawval) )
				   : rawval;
		if ( storptr )
		    *destptr++ = trcval;
		else if ( stor )
		    stor->setValue( valueidx++, trcval );
		else
		    arr.set( idx0, idx1, startidx++, trcval );
	    }
	}
    }

    return true;
}

protected:

    const RawTrcsSequence&	rawseq_;
    const StepInterval<float>&	zsamp_;
    const TypeSet<int>&		components_;
    const ObjectSet<Scaler>&	compscalers_;
    const TypeSet<int>&		outcomponents_;
    RegularSeisDataPack&	dp_;
    bool			is2d_;
    bool			samedatachar_;
    bool			needresampling_;
};

}; // namespace Seis



Seis::SequentialFSLoader::SequentialFSLoader( const IOObj& ioobj,
					      const TrcKeyZSampling* tkzs,
					      const TypeSet<int>* comps )
    : Seis::Loader(ioobj,tkzs,comps)
    , Executor("Volume Reader")
    , sd_(0)
    , prov_(0)
    , initialized_(false)
    , nrdone_(0)
    , line2ddata_(0)
    , trcsiterator2d_(0)
    , trcsiterator3d_(0)
    , samedatachar_(false)
    , needresampling_(true)
{
    seissummary_ = new ObjectSummary( ioobj );
    uiRetVal uirv;
    prov_ = Seis::Provider::create( ioobj.key(), &uirv );
    if ( prov_ )
    {
	msg_ = uiStrings::phrReading( tr(" %1 \'%2\'")
			  .arg( uiStrings::sVolume() )
			  .arg( ioobj.uiName() ) );
    }
    else
	msg_ = uirv;

    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::MultiThread,
				"SequentialFSLoader" );
}


Seis::SequentialFSLoader::~SequentialFSLoader()
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    delete prov_;
    delete trcsiterator2d_;
    delete trcsiterator3d_;
    delete line2ddata_;
}


uiString Seis::SequentialFSLoader::nrDoneText() const
{ return Seis::Loader::nrDoneText(); }

uiString Seis::SequentialFSLoader::message() const
{ return Seis::Loader::message(); }


bool Seis::SequentialFSLoader::goImpl( od_ostream* strm, bool first, bool last,
				       int delay )
{
    const bool success = Executor::goImpl( strm, first, last, delay );
    Threads::WorkManager::twm().emptyQueue( queueid_, success );

    return success;
}


bool Seis::SequentialFSLoader::init()
{
    if ( initialized_ )
	return true;

    msg_ = tr("Initializing reader");
    delete seissummary_;
    seissummary_ = new ObjectSummary( *ioobj_ );
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

    const bool is2d = seissummary_->is2D();
    const SeisIOObjInfo& seisinfo = seissummary_->getFullInformation();
    TrcKeyZSampling seistkzs( tkzs_ );
    seisinfo.getRanges( seistkzs );
    const DataCharacteristics datasetdc( seissummary_->getDataChar() );
    if ( dc_.userType() == OD::AutoFPRep )
	setDataChar( datasetdc.userType() );

    if ( components_.isEmpty() )
    {
	const int nrcomps = seisinfo.nrComponents();
	TypeSet<int> components;
	for ( int idx=0; idx<nrcomps; idx++ )
	    components += idx;

	setComponents( components );
    }

    adjustDPDescToScalers( datasetdc );
    if ( is2d && !tkzs_.is2D() )
    {
	pErrMsg("TrcKeySampling for 2D data needed with GeomID as lineNr");
	return false;
    }

    if ( !dp_ )
    {
	if ( is2d )
	{
	    const Pos::GeomID geomid( tkzs_.hsamp_.getGeomID() );
	    StepInterval<int> trcrg;
	    StepInterval<float> zrg;
	    if ( !seisinfo.getRanges(geomid,trcrg,zrg) )
		return false;

	    trcrg.limitTo( tkzs_.hsamp_.trcRange() );
	    tkzs_.zsamp_.limitTo( zrg );
	    tkzs_.hsamp_.setTrcRange( trcrg );
	}
	else
	{
	    TrcKeyZSampling storedtkzs;
	    seisinfo.getRanges( storedtkzs );
	    if ( tkzs_.isDefined() )
		tkzs_.limitTo( storedtkzs );
	    else
		tkzs_ = storedtkzs;
	}

	dp_ = new RegularSeisDataPack( VolumeDataPack::categoryStr(true,false),
					&dc_);
	dp_->setName( ioobj_->name() );
	dp_->setSampling( tkzs_ );
	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( !addComponents(*dp_,*ioobj_,components_,msg_) )
	    return false;
    }

    delete trcssampling_;
    trcssampling_ = new PosInfo::CubeData();
    if ( is2d )
    {
	mDynamicCastGet(Provider2D*,prov2d,prov_)
	if ( !prov2d ) return false;
	const Pos::GeomID geomid( tkzs_.hsamp_.getGeomID() );
	delete line2ddata_;
	line2ddata_ = new PosInfo::Line2DData();
	prov2d->getGeometryInfo( prov2d->lineNr(geomid), *line2ddata_ );
	PosInfo::LineData* linedata = new PosInfo::LineData( geomid );
	line2ddata_->limitTo( tkzs_.hsamp_.trcRange() );
	line2ddata_->getSegments( *linedata );
	trcssampling_->add( linedata );
	delete trcsiterator2d_;
	trcsiterator2d_ = new PosInfo::Line2DDataIterator( *line2ddata_ );
    }
    else
    {
	mDynamicCastGet(Provider3D*,prov3d,prov_)
	if ( !prov3d ) return false;
	prov3d->getGeometryInfo( *trcssampling_ );
	delete trcsiterator3d_;
	trcsiterator3d_ = new PosInfo::CubeDataIterator( *trcssampling_ );
    }

    trcssampling_->limitTo( tkzs_.hsamp_ );
    totalnr_ = line2ddata_ ? line2ddata_->size() : trcssampling_->totalSize();
    dp_->setTrcsSampling( new PosInfo::SortedCubeData(*trcssampling_) );

    nrdone_ = 0;

    seistkzs.hsamp_.limitTo( tkzs_.hsamp_ );
    sd_ = new Seis::RangeSelData( seistkzs );
    prov_->setSelData( sd_ );

    samedatachar_ = seissummary_->hasSameFormatAs( dp_->getDataDesc() );
    dpzsamp_ = dp_->sampling().zsamp_;
    dpzsamp_.limitTo( seissummary_->zRange() );
    needresampling_ = !dpzsamp_.isCompatible( seissummary_->zRange() );

    initialized_ = true;
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() ) \
						  .arg( ioobj_->uiName() ) );

    return true;
}


bool Seis::SequentialFSLoader::setDataPack( RegularSeisDataPack& dp,
					    od_ostream* extstrm )
{
    initialized_ = false;
    dp_ = &dp;
    setDataChar( DataCharacteristics( dp.getDataDesc() ).userType() );
    setScaler( dp.getScaler() && !dp.getScaler()->isEmpty()
	       ? dp.getScaler() : 0 );
    //scaler_ won't be used with external dp, but setting it for consistency

    if ( dp.sampling().isDefined() )
	tkzs_ = dp.sampling();
    else
	dp_->setSampling( tkzs_ );

    if ( dp_->nrComponents() < components_.size() &&
	 !addComponents(*dp_,*ioobj_,components_,msg_) )
    {
	if ( extstrm )
	    *extstrm << msg_.getFullString() << od_endl;

	return false;
    }

    return init(); // New datapack, hence re-init of provider
}


int Seis::SequentialFSLoader::nextStep()
{
    if ( !init() )
	return ErrorOccurred();

    if ( nrdone_ == 0 )
	submitUdfWriterTasks( queueid_ );

    if ( nrdone_ >= totalnr_ )
	return Finished();

    if ( Threads::WorkManager::twm().queueSize(queueid_) >
	 100*Threads::WorkManager::twm().nrThreads() )
	return MoreToDo();

    TypeSet<TrcKey>* tks = new TypeSet<TrcKey>;
    if ( !getTrcsPosForRead(*tks) )
	{ delete tks; return Finished(); }

    RawTrcsSequence* rawseq = new RawTrcsSequence( *seissummary_,
						    tks->size() );
    if ( rawseq ) rawseq->setPositions( *tks );
    if ( !rawseq || !rawseq->isOK() )
    {
	if ( !rawseq ) delete tks;
	delete rawseq;
	msg_ = tr("Cannot allocate trace data");
	return ErrorOccurred();
    }

    const uiRetVal uirv = prov_->getSequence( *rawseq );
    if ( !uirv.isOK() )
    {
	delete rawseq;
	if ( isFinished(uirv) )
	    return Finished();

	msg_ = uirv;
	return ErrorOccurred();
    }

    const TypeSet<int>& outcomponents = outcomponents_
				      ? *outcomponents_ : components_;

    Task* task = new ArrayFiller( *rawseq, dpzsamp_, samedatachar_,
				  needresampling_, components_, compscalers_,
				   outcomponents, *dp_, (*tks)[0].is2D() );
    nrdone_ += rawseq->nrPositions();
    Threads::WorkManager::twm().addWork(
		Threads::Work(*task,true), 0, queueid_, false, false, true );

    return MoreToDo();
}


#define cTrcChunkSz	1000

bool Seis::SequentialFSLoader::getTrcsPosForRead( TypeSet<TrcKey>& tks ) const
{
    tks.setEmpty();
    BinID bid;
    const bool is2d = !trcsiterator3d_;
    const Pos::GeomID geomid( trcsiterator2d_ ? trcsiterator2d_->geomID()
					      : mUdfGeomID );
    for ( int idx=0; idx<cTrcChunkSz; idx++ )
    {
	if ( (is2d && !trcsiterator2d_->next()) ||
	    (!is2d && !trcsiterator3d_->next(bid)) )
	    break;

	tks += is2d ? TrcKey( geomid, trcsiterator2d_->trcNr() )
		    : TrcKey( tkzs_.hsamp_.survid_, bid );
    }

    return !tks.isEmpty();
}



Seis::SequentialPSLoader::SequentialPSLoader( const IOObj& ioobj,
					      const Interval<int>* linerg,
					      Pos::GeomID geomid )
    : Executor("Volume Reader")
    , ioobj_(ioobj.clone())
    , geomid_(geomid)
    , sd_(0)
    , prov_(0)
    , nrdone_(0)
{
    uiRetVal uirv;
    prov_ = Seis::Provider::create( ioobj.key(), &uirv );
    if ( !prov_ )
	msg_ = uirv;

    TrcKeyZSampling tkzs;
    SeisIOObjInfo info( ioobj );
    if ( info.is2D() )
    {
	StepInterval<int> trcrg; ZSampling zsamp;
	if ( !info.getRanges(geomid,trcrg,zsamp) )
	    return;

	tkzs.set2DDef();
	tkzs.hsamp_.setLineRange( Interval<int>(geomid,geomid) );
	tkzs.hsamp_.setTrcRange( trcrg );
	tkzs.zsamp_ = zsamp;

	prov_->setSelData( new Seis::RangeSelData(tkzs) );
    }
    else
    {
	if ( !info.getRanges(tkzs) )
	    return;

	Seis::RangeSelData* seldata = new Seis::RangeSelData( tkzs );
	if ( linerg )
	    seldata->setInlRange( *linerg );

	prov_->setSelData( seldata );
    }

    init();
}


Seis::SequentialPSLoader::~SequentialPSLoader()
{
    delete ioobj_;
    delete prov_;
}


od_int64 Seis::SequentialPSLoader::totalNr() const
{
    return prov_ ? prov_->totalNr() : 0;
}


uiString Seis::SequentialPSLoader::nrDoneText() const
{
    return uiStrings::phrJoinStrings( uiStrings::sTrace(mPlural),
				      tr("read") );
}


bool Seis::SequentialPSLoader::init()
{
    gatherdp_ = new GatherSetDataPack( ioobj_->name() );
    const StringPair strpair( prov_->name(), Survey::GM().getName(geomid_));
    gatherdp_->setName( strpair.getCompString() );
    return true;
}


int Seis::SequentialPSLoader::nextStep()
{
    if ( !prov_ || !gatherdp_ )
	return ErrorOccurred();

    SeisTrcBuf trcbuf( true );
    const uiRetVal uirv = prov_->getNextGather( trcbuf );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    return Finished();

	msg_ = uirv;
	return ErrorOccurred();
    }

    Gather* gather = new Gather();
    gather->setFromTrcBuf( trcbuf, 0 );
    gatherdp_->addGather( gather );

    nrdone_++;
    return MoreToDo();
}

