/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/


#include "seisloader.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "cbvsreadmgr.h"
#include "convmemvalseries.h"
#include "datapackbase.h"
#include "nrbytes2string.h"
#include "ioobj.h"
#include "od_ostream.h"
#include "odsysmem.h"
#include "posinfo.h"
#include "samplingdata.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisread.h"
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
	if ( !dp.addComponent( compstr.getCompString() ) )
	    return false;
    }

    return true;
}


Loader::Loader( const IOObj& ioobj, const TrcKeyZSampling* tkzs,
		const TypeSet<int>* components )
    : ioobj_(ioobj.clone())
    , tkzs_(false)
    , dc_(OD::AutoFPRep)
    , outcomponents_(0)
    , scaler_(0)
{
    compscalers_.allowNull( true );
    if ( tkzs )
	tkzs_ = *tkzs;

    if ( components )
	setComponents( *components );
}


Loader::~Loader()
{
    delete ioobj_;
    deepErase( compscalers_ );
    delete outcomponents_;
    delete scaler_;
}


void Loader::setDataChar( DataCharacteristics::UserType type )
{ dc_ = DataCharacteristics(type); }


void Loader::setComponents( const TypeSet<int>& components )
{
    components_ = components;
    for ( int idx=0; idx<components.size(); idx++ )
	compscalers_ += 0;
}


void Loader::setComponentScaler( const Scaler& scaler, int compidx )
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


bool Loader::setOutputComponents( const TypeSet<int>& compnrs )
{
    if ( compnrs.size() != components_.size() )
	return false;

    delete outcomponents_;
    outcomponents_ = new TypeSet<int>( compnrs );

    return true;
}


void Loader::setScaler( const Scaler* newsc )
{
    delete scaler_;
    scaler_ = newsc ? newsc->clone() : 0;
}


RegularSeisDataPack* Loader::getDataPack()
{
    return dp_;
}


uiString Loader::nrDoneText() const
{
    return uiStrings::phrJoinStrings( uiStrings::sTrace(mPlural), tr("read") );
}


void Loader::adjustDPDescToScalers( const BinDataDesc& trcdesc )
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
	dp_->addComponent( compnms.get(idx).str() );
}



ParallelFSLoader3D::ParallelFSLoader3D( const IOObj& ioobj,
					const TrcKeyZSampling& tkzs )
    : Loader(ioobj,&tkzs,0)
    , bidvals_(0)
    , totalnr_(tkzs.hsamp_.totalNr())
{
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() ) \
						  .arg( ioobj_->uiName() ) );
}


ParallelFSLoader3D::ParallelFSLoader3D( const IOObj& ioobj,
					BinIDValueSet& bidvals,
					const TypeSet<int>& components )
    : Loader(ioobj,0,&components)
    , bidvals_( &bidvals )
    , totalnr_(bidvals.totalSize())
{
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() ) \
						  .arg( ioobj_->uiName() ) );
}


void ParallelFSLoader3D::setDataPack( RegularSeisDataPack* dp )
{
    dp_ = dp;
    DPM( DataPackMgr::SeisID() ).add( dp );
}


uiString ParallelFSLoader3D::nrDoneText() const
{ return Loader::nrDoneText(); }

uiString ParallelFSLoader3D::message() const
{ return Loader::message(); }


bool ParallelFSLoader3D::doPrepare( int nrthreads )
{
    uiString allocprob = tr("Cannot allocate memory");
    SeisIOObjInfo seisinfo( *ioobj_ );
    if ( seisinfo.isOK() )
	return false;

    DataCharacteristics datasetchar;
    seisinfo.getDataChar( datasetchar );
    if ( dc_.userType() == OD::AutoFPRep )
	setDataChar( datasetchar.userType() );

    if ( components_.isEmpty() )
    {
	const int nrcomponents = seisinfo.nrComponents();
	TypeSet<int> components;
	for ( int idx=0; idx<nrcomponents; idx++ )
	    components += idx;

	setComponents( components );
    }

    adjustDPDescToScalers( datasetchar );

    if ( bidvals_ )
    {
	pErrMsg("The bidval-code is not tested. Run through step by step, make "
		"sure everything is OK and remove this warning.");
	const int nrvals = 1+components_.size();
        if ( bidvals_->nrVals()!=nrvals )
	{
	    if ( !bidvals_->setNrVals( nrvals ) )
	    {
		msg_ = allocprob;
		return false;
	    }
	}
    }
    else if ( !dp_ )
    {
	dp_ = new RegularSeisDataPack(0);
	DPM( DataPackMgr::SeisID() ).add( dp_.ptr() );
	dp_->setName( ioobj_->name() );
	dp_->setSampling( tkzs_ );
	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	uiString errmsg;
	if ( !addComponents(*dp_,*ioobj_,components_,errmsg) )
	{
	    msg_ = allocprob;
	    msg_.append( errmsg, true );
	    return false;
	}
    }

    return true;
}


bool ParallelFSLoader3D::doWork( od_int64 start, od_int64 stop, int threadid )
{
    PtrMan<IOObj> ioobj = ioobj_->clone();
    PtrMan<SeisTrcReader> reader = new SeisTrcReader( ioobj );
    if ( !reader )
    {
	msg_ = tr("Cannot open storage");
	return false;
    }

    if ( !reader->prepareWork() )
    {
	msg_ = reader->errMsg();
	return false;
    }

    if ( !threadid && !dp_->is2D() )
    {
	PosInfo::CubeData cubedata;
	if ( reader->get3DGeometryInfo(cubedata) )
	{
	    cubedata.limitTo( tkzs_.hsamp_ );
	    if ( !cubedata.isFullyRectAndReg() )
		dp_->setTrcsSampling( new PosInfo::SortedCubeData(cubedata) );
	}
    }

    //Set ranges, at least z-range
    mDynamicCastGet( SeisTrcTranslator*, translator, reader->translator() );
    if ( !translator || !translator->supportsGoTo() )
    {
	msg_ = tr("Storage does not support random access");
	return false;
    }

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

        if ( translator->goTo( curbid )
	  && reader->get( trc )
	  && trc.info().binID() == curbid )
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



// ParallelFSLoader3D (probably replace by a SequentialFSLoader)
ParallelFSLoader2D::ParallelFSLoader2D( const IOObj& ioobj,
					const TrcKeyZSampling& tkzs,
					const TypeSet<int>* comps )
    : Loader(ioobj,&tkzs,comps)
    , dpclaimed_(false)
    , totalnr_(tkzs.hsamp_.totalNr())
{
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() ) \
						  .arg( ioobj_->uiName() ) );
}


uiString ParallelFSLoader2D::nrDoneText() const
{ return Loader::nrDoneText(); }

uiString ParallelFSLoader2D::message() const
{ return Loader::message(); }


bool ParallelFSLoader2D::doPrepare( int nrthreads )
{
    const SeisIOObjInfo seisinfo( *ioobj_ );
    if ( !seisinfo.isOK() )
	return false;

    DataCharacteristics datasetchar;
    seisinfo.getDataChar( datasetchar );
    if ( dc_.userType() == OD::AutoFPRep )
	setDataChar( datasetchar.userType() );

    if ( components_.isEmpty() )
    {
	const int nrcomps = seisinfo.nrComponents( tkzs_.hsamp_.getGeomID() );
	TypeSet<int> components;
	for ( int idx=0; idx<nrcomps; idx++ )
	    components += idx;

	setComponents( components );
    }

    adjustDPDescToScalers( datasetchar );

    dp_ = new RegularSeisDataPack( VolumeDataPack::categoryStr(true,true),
				    &dc_ );
    DPM(DataPackMgr::SeisID()).add( dp_.ptr() );
    dp_->setName( ioobj_->name() );
    dp_->setSampling( tkzs_ );
    if ( scaler_ && !scaler_->isEmpty() )
	dp_->setScaler( *scaler_ );

    if ( !addComponents(*dp_,*ioobj_,components_,msg_) )
	return false;

    return true;
}


bool ParallelFSLoader2D::doWork( od_int64 start, od_int64 stop, int threadid )
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
		    const DataBuffer* databuf = trc.data().getComponent(idcin);
		    const int bytespersamp = databuf->bytesPerElement();
		    const od_int64 offset = arr.info().getOffset( 0, idy, 0 );
		    char* dststartptr = storarr + offset*bytespersamp;

		    for ( int zidx=0; zidx<nrzsamples; zidx++ )
		    {
			const float zval = tkzs_.zsamp_.atIndex( zidx );
			const int trczidx = trc.nearestSample( zval );
			const unsigned char* srcptr =
				databuf->data() + trczidx*bytespersamp;
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



RegularSeisDataPack* ParallelFSLoader2D::getDataPack()
{
    dpclaimed_ = true;
    return Loader::getDataPack();
}



// SequentialReader
class ArrayFiller : public Task
{
public:
ArrayFiller( SeisTrc& trc,
	     const TypeSet<int>& components,
	     const ObjectSet<Scaler>& compscalers,
	     const TypeSet<int>& outcomponents, RegularSeisDataPack& dp,
	     bool is2d )
    : trc_(trc)
    , components_(components)
    , compscalers_(compscalers)
    , outcomponents_(outcomponents)
    , dp_(dp),is2d_(is2d)
{}


~ArrayFiller()
{ delete &trc_; }

bool execute()
{
    const int idx0 = is2d_ ? 0
		: dp_.sampling().hsamp_.lineIdx( trc_.info().lineNr() );
    const int idx1 = dp_.sampling().hsamp_.trcIdx( trc_.info().trcNr() );

    StepInterval<float> dpzsamp = dp_.sampling().zsamp_;
    const StepInterval<float>& trczsamp = trc_.zRange();
    dpzsamp.limitTo( trczsamp );
    const int startidx = dp_.sampling().zsamp_.nearestIndex( dpzsamp.start );
    const int nrzsamples = dpzsamp.nrSteps()+1;

    for ( int cidx=0; cidx<outcomponents_.size(); cidx++ )
    {
	const int idcin = components_[cidx];
	const Scaler* scaler = compscalers_[cidx];
	const int idcout = outcomponents_[cidx];
	Array3D<float>& arr = dp_.data( idcout );
	ValueSeries<float>* stor = arr.getStorage();
	mDynamicCastGet(ConvMemValueSeries<float>*,storptr,stor);
	char* storarr = storptr ? storptr->storArr() : (char*)stor->arr();
	const BinDataDesc trcdatadesc =
		trc_.data().getInterpreter(idcin)->dataChar();
	if ( storarr && dp_.getDataDesc()==trcdatadesc )
	{
	    const DataBuffer* databuf = trc_.data().getComponent( idcin );
	    const int bytespersamp = databuf->bytesPerElement();
	    const od_int64 offset = arr.info().getOffset( idx0, idx1, 0 );
	    char* dststartptr = storarr + offset*bytespersamp;
	    for ( int zidx=0; zidx<nrzsamples; zidx++ )
	    {
		// Check if amplitude equals undef value of underlying data
		// type knowing that array has been initialized with undefs
		const float zval = dpzsamp.atIndex( zidx );
		const int trczidx = trc_.nearestSample( zval );
		const unsigned char* srcptr =
			databuf->data() + trczidx*bytespersamp;
		char* dstptr = dststartptr + (zidx+startidx)*bytespersamp;
		if ( !scaler && memcmp(dstptr,srcptr,bytespersamp) )
		{
		    OD::sysMemCopy(dstptr,srcptr,bytespersamp );
		    continue;
		}

		const float rawval = trc_.getValue( zval, idcin );
		const float trcval = scaler
				   ? mCast(float,scaler->scale(rawval) )
				   : rawval;
		arr.set( idx0, idx1, zidx+startidx, trcval );
	    }
	}
	else
	{
	    for ( int zidx=0; zidx<nrzsamples; zidx++ )
	    {
		const float zval = dpzsamp.atIndex( zidx );
		const float rawval = trc_.getValue( zval, idcin );
		const float trcval = scaler
				   ? mCast(float,scaler->scale(rawval) )
				   : rawval;
		arr.set( idx0, idx1, zidx, trcval );
	    }
	}
    }

    return true;
}

protected:

    SeisTrc&			trc_;
    const TypeSet<int>&		components_;
    const ObjectSet<Scaler>&	compscalers_;
    const TypeSet<int>&		outcomponents_;
    RegularSeisDataPack&	dp_;
    bool			is2d_;
};



SequentialFSLoader::SequentialFSLoader( const IOObj& ioobj,
					const TrcKeyZSampling* tkzs,
					const TypeSet<int>* comps )
    : Loader(ioobj,tkzs,comps)
    , Executor("Volume Reader")
    , sd_(0)
    , rdr_(*new SeisTrcReader(&ioobj))
    , initialized_(false)
    , nrdone_(0)
{
    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::MultiThread,
				"SequentialFSLoader" );
}


SequentialFSLoader::~SequentialFSLoader()
{
    delete &rdr_;
    Threads::WorkManager::twm().removeQueue( queueid_, false );
}


uiString SequentialFSLoader::nrDoneText() const
{ return Loader::nrDoneText(); }

uiString SequentialFSLoader::message() const
{ return Loader::message(); }


#define mSetSelData() \
{ \
    sd_ = new Seis::RangeSelData( tkzs_ ); \
    rdr_.setSelData( sd_ ); \
    if ( !rdr_.prepareWork() ) \
    { \
	msg_ = rdr_.errMsg(); \
	return false; \
    } \
}

bool SequentialFSLoader::init()
{
    if ( initialized_ )
	return true;

    msg_ = tr("Initializing reader");
    SeisIOObjInfo seisinfo( ioobj_ );
    if ( !seisinfo.isOK() )
	return false;

    const bool is2d = seisinfo.is2D();
    DataCharacteristics datasetdc;
    seisinfo.getDataChar( datasetdc );
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

    if ( is2d && !TrcKey::is2D(tkzs_.hsamp_.survid_) )
    {
	pErrMsg("TrcKeySampling for 2D data needed with GeomID as lineNr");
	return false;
    }

    adjustDPDescToScalers( datasetdc );
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
	DPM( DataPackMgr::SeisID() ).add( dp_.ptr() );
	dp_->setName( ioobj_->name() );
	dp_->setSampling( tkzs_ );
	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( !addComponents(*dp_,*ioobj_,components_,msg_) )
	    return false;
    }

    if ( !is2d )
    {
	PosInfo::CubeData cubedata;
	if ( rdr_.get3DGeometryInfo(cubedata) )
	{
	    cubedata.limitTo( tkzs_.hsamp_ );
	    if ( !cubedata.isFullyRectAndReg() )
		dp_->setTrcsSampling( new PosInfo::SortedCubeData(cubedata) );
	}
    }

    nrdone_ = 0;

    mSetSelData()

    initialized_ = true;
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() ) \
						  .arg( ioobj_->uiName() ) );

    return true;
}


bool SequentialFSLoader::setDataPack( RegularSeisDataPack& dp,
				    od_ostream* extstrm )
{
    initialized_ = false;
    dp_ = &dp;
    DPM( DataPackMgr::SeisID() ).add( dp_.ptr() );
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

    return init(); // New datapack, hence re-init of trace reader
}


int SequentialFSLoader::nextStep()
{
    if ( !initialized_ && !init() )
	return ErrorOccurred();

    if ( Threads::WorkManager::twm().queueSize(queueid_) >
	 100*Threads::WorkManager::twm().nrThreads() )
	return MoreToDo();

    SeisTrc* trc = new SeisTrc;
    const int res = rdr_.get( trc->info() );
    if ( res==-1 )
    { delete trc; msg_ = rdr_.errMsg(); return ErrorOccurred(); }
    if ( res==0 ) { delete trc; return Finished(); }
    if ( res==2 ) { delete trc; return MoreToDo(); }

    if ( !rdr_.get(*trc) )
    { delete trc; msg_ = rdr_.errMsg(); return ErrorOccurred(); }

    const TypeSet<int>& outcomponents = outcomponents_
				      ? *outcomponents_ : components_;
    Task* task =
	new ArrayFiller( *trc, components_, compscalers_, outcomponents, *dp_,
			 tkzs_.hsamp_.is2D() );
    Threads::WorkManager::twm().addWork(
	Threads::Work(*task,true), 0, queueid_, false, false, true );

    nrdone_++;
    return MoreToDo();
}

} // namespace Seis
