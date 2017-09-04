/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seisparallelreader.h"

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
#include "samplingdata.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seiscommon.h"
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

	// LineKey assembles composite "<attribute>|<component>" name
	if ( !dp.addComponentNoInit(LineKey(ioobj.name().str(),cnm)) )
	    return false;
    }

    return true;
}

}; // namespace Seis



Seis::ParallelReader::ParallelReader( const IOObj& ioobj,
				      const TrcKeyZSampling& cs )
    : dp_(0)
    , bidvals_(0)
    , tkzs_(cs)
    , ioobj_( ioobj.clone() )
    , totalnr_( cs.hsamp_.totalNr() )
{
    SeisIOObjInfo seisinfo( ioobj );
    const int nrcomponents = seisinfo.nrComponents();
    for ( int idx=0; idx<nrcomponents; idx++ )
	components_ += idx;
}


Seis::ParallelReader::ParallelReader( const IOObj& ioobj,BinIDValueSet& bidvals,
				      const TypeSet<int>& components )
    : dp_(0)
    , components_( components )
    , bidvals_( &bidvals )
    , ioobj_( ioobj.clone() )
    , totalnr_( bidvals.totalSize() )
{
    errmsg_ = uiStrings::phrReading( ioobj_->uiName() );
}


Seis::ParallelReader::~ParallelReader()
{
    DPM( DataPackMgr::SeisID() ).release( dp_ );
    delete ioobj_;
}


bool Seis::ParallelReader::setOutputComponents( const TypeSet<int>& compnrs )
{
    if ( compnrs.size() != components_.size() )
	return false;

    seisrdroutcompmgr_ =  compnrs;
    return true;
}


void Seis::ParallelReader::setDataPack( RegularSeisDataPack* dp )
{
    DPM( DataPackMgr::SeisID() ).release( dp_ );
    dp_ = dp;
    DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );
}


RegularSeisDataPack* Seis::ParallelReader::getDataPack()
{
    return dp_;
}


uiString Seis::ParallelReader::uiNrDoneText() const
{ return tr("Traces read"); }


uiString Seis::ParallelReader::uiMessage() const
{
    return errmsg_.isEmpty() ? tr("Reading volume \'%1\'").arg(ioobj_->name())
			     : errmsg_;
}


bool Seis::ParallelReader::doPrepare( int nrthreads )
{
    uiString allocprob = tr("Cannot allocate memory");

    if ( bidvals_ )
    {
	pErrMsg("The bidval-code is not tested. Run through step by step, make "
		"sure everything is OK and remove this warning.");
	const int nrvals = 1+components_.size();
        if ( bidvals_->nrVals()!=nrvals )
	{
	    if ( !bidvals_->setNrVals( nrvals, true ) )
	    {
		errmsg_ = allocprob;
		return false;
	    }
	}
    }
    else if ( !dp_ )
    {
	dp_ = new RegularSeisDataPack(0);
	dp_->setSampling( tkzs_ );
	DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );

	uiString errmsg;
	if ( !addComponents(*dp_,*ioobj_,components_,errmsg) )
	{
	    errmsg_ = allocprob;
	    errmsg_.append( errmsg, true );
	    return false;
	}
    }

    return true;
}


bool Seis::ParallelReader::doWork( od_int64 start, od_int64 stop, int threadid )
{
    PtrMan<IOObj> ioobj = ioobj_->clone();
    PtrMan<SeisTrcReader> reader = new SeisTrcReader( ioobj );
    if ( !reader )
    {
	errmsg_ = tr("Cannot open storage");
	return false;
    }

    if ( !reader->prepareWork() )
    {
	errmsg_ = reader->errMsg();
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
	errmsg_ = tr("Storage does not support random access");
	return false;
    }

    const TypeSet<int>& outcompnrs = !seisrdroutcompmgr_.isEmpty()
				   ? seisrdroutcompmgr_ : components_;
    TrcKeySamplingIterator iter;
    BinIDValueSet::SPos bidvalpos;
    BinID curbid;
    if ( bidvals_ )
    {
        bidvalpos = bidvals_->getPos( start );
        if ( !bidvalpos.isValid() )
            return false;

        curbid = bidvals_->getBinID( bidvalpos );
    }
    else
    {
	iter.setSampling( tkzs_.hsamp_ );
	iter.setNextPos( tkzs_.hsamp_.trcKeyAt(start) );
	iter.next( curbid );
    }

    SeisTrc trc;

#define mUpdateInterval 100
    int nrdone = 0;
    for ( od_int64 idx=start; true; idx++, nrdone++ )
    {
	if ( nrdone>mUpdateInterval )
	{
	    addToNrDone( nrdone );
	    nrdone = 0;

	    if ( !shouldContinue() )
		return false;
	}

        if ( translator->goTo( curbid ) && reader->get( trc ) &&
            trc.info().binid==curbid )
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
			vals[idc+1] = trc.getValue( z, components_[idcx] );
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
				Array3D<float>& arr3d = dp_->data( idc );
				arr3d.set( inlidx, crlidx, idz, val);
			    }
			}
		    }
		}
            }
        }

        if ( idx==stop )
            break;

        if ( bidvals_ )
        {
            if ( !bidvals_->next(bidvalpos,false) )
                return false;

            curbid = bidvals_->getBinID( bidvalpos );
        }
        else
        {
            if ( !iter.next( curbid ) )
                return false;
        }
    }

    addToNrDone( nrdone );

    return true;
}


bool Seis::ParallelReader::doFinish( bool success )
{ return success; }



// ParallelReader2D (probably replace by a Sequential reader)
Seis::ParallelReader2D::ParallelReader2D( const IOObj& ioobj,Pos::GeomID geomid,
					  const TrcKeyZSampling* tkzs,
					  const TypeSet<int>* comps )
    : geomid_(geomid)
    , ioobj_(ioobj.clone())
    , dc_(DataCharacteristics::Auto)
    , dpclaimed_(false)
    , scaler_(0)
    , dp_(0)
{
    if ( comps )
	components_ = *comps;

    if ( tkzs )
	tkzs_ = *tkzs;

    totalnr_ = tkzs_.isEmpty() ? 1 : tkzs_.hsamp_.totalNr();
}


bool Seis::ParallelReader2D::doPrepare( int )
{
    return init();
}


bool Seis::ParallelReader2D::init()
{
    const SeisIOObjInfo info( *ioobj_ );
    if ( !info.isOK() ) return false;

    if ( dc_.userType() == DataCharacteristics::Auto )
	info.getDataChar( dc_ );
    if ( components_.isEmpty() )
    {
	const int nrcomps = info.nrComponents( geomid_ );
	for ( int idx=0; idx<nrcomps; idx++ )
	    components_ += idx;
    }

    if ( tkzs_.isEmpty() )
    {
	StepInterval<int> trcrg;
	info.getRanges( geomid_, trcrg, tkzs_.zsamp_ );
	tkzs_.hsamp_.set( Interval<int>(geomid_,geomid_), trcrg );
    }

    totalnr_ = tkzs_.hsamp_.totalNr();

    dp_ = new RegularSeisDataPack( SeisDataPack::categoryStr(true,true), &dc_ );
    DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );
    dp_->setSampling( tkzs_ );
    if ( scaler_ )
	dp_->setScaler( *scaler_ );

    if ( !addComponents(*dp_,*ioobj_,components_,msg_) )
    {
	DPM( DataPackMgr::SeisID() ).release( dp_ );
	return false;
    }

    msg_ = uiStrings::phrReading( ioobj_->uiName() );
    return true;
}


Seis::ParallelReader2D::~ParallelReader2D()
{
    delete ioobj_;
    delete scaler_;

    DPM( DataPackMgr::SeisID() ).release( dp_ );
}


void Seis::ParallelReader2D::setDataChar( DataCharacteristics::UserType type )
{ dc_ = DataCharacteristics(type); }


void Seis::ParallelReader2D::setScaler( Scaler* newsc )
{
    delete scaler_;
    scaler_ = newsc ? newsc->clone() : 0;
}


uiString Seis::ParallelReader2D::uiNrDoneText() const
{ return tr("Traces read"); }

uiString Seis::ParallelReader2D::uiMessage() const
{ return msg_.isEmpty() ? tr("Reading") : msg_; }

od_int64 Seis::ParallelReader2D::nrIterations() const
{ return totalnr_; }


bool Seis::ParallelReader2D::doWork( od_int64 start,od_int64 stop,int threadid )
{
    if ( !dp_ || dp_->nrComponents()==0 )
	return false;

    PtrMan<IOObj> ioobj = ioobj_->clone();
    const Seis2DDataSet dataset( *ioobj );
    const int lidx = dataset.indexOf( geomid_ );
    if ( lidx<0 ) return false;

    const char* fnm = SeisCBVS2DLineIOProvider::getFileName( *ioobj,
							dataset.geomID(lidx) );
    PtrMan<CBVSSeisTrcTranslator> trl =
	CBVSSeisTrcTranslator::make( fnm, false, true );
    if ( !trl ) return false;

    SeisTrc trc;
    BinID curbid;
    StepInterval<int> trcrg = tkzs_.hsamp_.crlRange();
    trl->toStart();
    curbid = trl->readMgr()->binID();

    const int nrzsamples = tkzs_.zsamp_.nrSteps()+1;
    for ( int idc=0; idc<dp_->nrComponents(); idc++ )
    {
	Array3D<float>& arr = dp_->data( idc );
	ValueSeries<float>* stor = arr.getStorage();
	mDynamicCastGet(ConvMemValueSeries<float>*,storptr,stor);
	char* storarr = storptr ? storptr->storArr() : (char*)stor->arr();

	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    curbid.crl() = trcrg.atIndex( mCast(int,idx) );
	    if ( trl->goTo(curbid) && trl->read(trc) )
	    {
		const BinDataDesc trcdatadesc =
			trc.data().getInterpreter(idc)->dataChar();
		if ( storarr && dp_->getDataDesc()==trcdatadesc )
		{
		    const DataBuffer* databuf = trc.data().getComponent( idc );
		    const int bytespersamp = databuf->bytesPerSample();
		    const od_int64 offset =
			arr.info().getOffset( 0, mCast(int,idx), 0 );
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
			if ( memcmp(dstptr,srcptr,bytespersamp) )
			    OD::sysMemCopy(dstptr,srcptr,bytespersamp );
			else
			    arr.set( 0, (int)idx, zidx, trc.getValue(zval,idc));
		    }
		}
		else
		{
		    for ( int zidx=0; zidx<nrzsamples; zidx++ )
		    {
			const float zval = tkzs_.zsamp_.atIndex( zidx );
			arr.set( 0, (int)idx, zidx, trc.getValue(zval,idc) );
		    }
		}
	    }

	    addToNrDone( 1 );
	}
    }

    return true;
}


bool Seis::ParallelReader2D::doFinish( bool success )
{ return success; }


RegularSeisDataPack* Seis::ParallelReader2D::getDataPack()
{
    dpclaimed_ = true;
    return dp_;
}



// SequentialReader
namespace Seis {

class ArrayFiller : public Task
{
public:
ArrayFiller( const RawTrcsSequence& databuf, const StepInterval<float>& zsamp,
	     bool samedatachar, bool needresampling,
	     const TypeSet<int>& components,
	     const ObjectSet<Scaler>& compscalers,
	     const TypeSet<int>& outcomponents,
	     RegularSeisDataPack& dp, bool is2d )
    : databuf_(databuf)
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
    delete &databuf_;
}

bool execute()
{
    const int nrpos = databuf_.nrPositions();
    for ( int itrc=0; itrc<nrpos; itrc++ )
    {
	if ( !doTrace(itrc) )
	    return false;
    }

    return true;
}

bool doTrace( int itrc )
{
    const TrcKey& tk = databuf_.getPosition( itrc );
    const int idx0 = is2d_ ? 0 : dp_.sampling().hsamp_.lineIdx( tk.lineNr() );
    const int idx1 = dp_.sampling().hsamp_.trcIdx( tk.trcNr() );

    const int startidx = dp_.sampling().zsamp_.nearestIndex( zsamp_.start );
    const int nrzsamples = zsamp_.nrSteps()+1;

    for ( int cidx=0; cidx<outcomponents_.size(); cidx++ )
    {
	const int idcin = components_[cidx];
	const Scaler* scaler = compscalers_[cidx];
	const int idcout = outcomponents_[cidx];
	Array3D<float>& arr = dp_.data( idcout );
	ValueSeries<float>* stor = arr.getStorage();
	mDynamicCastGet(ConvMemValueSeries<float>*,storptr,stor);
	char* storarr = storptr ? storptr->storArr() : (char*)stor->arr();
	if ( storarr && samedatachar_ )
	{
	    const int bytespersamp = dp_.getDataDesc().nrBytes();
	    const od_int64 offset = arr.info().getOffset( idx0, idx1, 0 );
	    char* dststartptr = storarr + offset*bytespersamp;
	    if ( needresampling_ || scaler )
	    {
		for ( int zidx=0; zidx<nrzsamples; zidx++ )
		{
		    // Check if amplitude equals undef value of underlying data
		    // type knowing that array has been initialized with undefs
		    const float zval = zsamp_.atIndex( zidx );
		    const int trczidx = databuf_.getZRange().nearestIndex(zval);
		    const unsigned char* srcptr =
					 databuf_.getData(itrc,idcin,trczidx);
		    char* dstptr = dststartptr + (zidx+startidx)*bytespersamp;
		    if ( !scaler && memcmp(dstptr,srcptr,bytespersamp) )
		    {
			OD::sysMemCopy(dstptr,srcptr,bytespersamp );
			continue;
		    }

		    const float rawval = databuf_.getValue( zval, itrc, idcin );
		    const float trcval = scaler
				       ? mCast(float,scaler->scale(rawval) )
				       : rawval;
		    arr.set( idx0, idx1, zidx+startidx, trcval );
		}
	    }
	    else
	    {
		const int trczidx = databuf_.getZRange().nearestIndex(
						zsamp_.atIndex( 0 ) );
		const unsigned char* srcptr = databuf_.getData( itrc, idcin,
								trczidx );
		char* dstptr = dststartptr;
		OD::sysMemCopy(dstptr,srcptr,nrzsamples*bytespersamp );
	    }
	}
	else
	{
	    for ( int zidx=0; zidx<nrzsamples; zidx++ )
	    {
		const float zval = zsamp_.atIndex( zidx );
		const float rawval = databuf_.getValue( zval, itrc, idcin );
		const float trcval = scaler
				   ? mCast(float,scaler->scale(rawval) )
				   : rawval;
		arr.set( idx0, idx1, zidx+startidx, trcval );
	    }
	}
    }

    return true;
}

protected:

    const RawTrcsSequence&	databuf_;
    const StepInterval<float>&	zsamp_;
    const TypeSet<int>&		components_;
    const ObjectSet<Scaler>&	compscalers_;
    const TypeSet<int>&		outcomponents_;
    RegularSeisDataPack&	dp_;
    bool			is2d_;
    bool			samedatachar_;
    bool			needresampling_;
};

}; //namespace Seis



Seis::SequentialReader::SequentialReader( const IOObj& ioobj,
					  const TrcKeyZSampling* tkzs,
					  const TypeSet<int>* comps )
    : Executor("Volume Reader")
    , ioobj_(ioobj.clone())
    , dp_(0)
    , sd_(0)
    , scaler_(0)
    , rdr_(*new SeisTrcReader(ioobj_))
    , dc_(DataCharacteristics::Auto)
    , initialized_(false)
    , is2d_(false)
    , trcssampling_(0)
    , trcsiterator3d_(0)
    , samedatachar_(false)
    , needresampling_(true)
    , seissummary_(0)
{
    compscalers_.allowNull( true );
    SeisIOObjInfo info( ioobj );
    info.getDataChar( dc_ );
    if ( !comps )
    {
	const int nrcomps = info.nrComponents();
	for ( int idx=0; idx<nrcomps; idx++ )
	{
	    components_ += idx;
	    compscalers_ += 0;
	}
    }
    else
    {
	components_ = *comps;
	for ( int idx=0; idx<components_.size(); idx++ )
	    compscalers_ += 0;
    }

    if ( tkzs )
	tkzs_ = *tkzs;

    seissummary_ = new ObjectSummary( ioobj );
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'")
				.arg( uiStrings::sVolume() )
				.arg( ioobj.uiName() ) );

    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::MultiThread,
				"SequentialReader" );
}


Seis::SequentialReader::~SequentialReader()
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    DPM( DataPackMgr::SeisID() ).release( dp_ );
    delete &rdr_; delete ioobj_;
    delete scaler_;
    delete seissummary_;
    delete trcssampling_;
    delete trcsiterator3d_;

    deepErase( compscalers_ );
}


uiString Seis::SequentialReader::uiNrDoneText() const
{
    return uiStrings::phrJoinStrings( uiStrings::sTrace(mPlural), tr("read") );
}


bool Seis::SequentialReader::goImpl( od_ostream* strm, bool first, bool last,
				     int delay )
{
    const bool success = Executor::goImpl( strm, first, last, delay );
    Threads::WorkManager::twm().emptyQueue( queueid_, success );

    return success;
}


bool Seis::SequentialReader::setOutputComponents( const TypeSet<int>& compnrs )
{
    if ( compnrs.size() != components_.size() )
	return false;

    outcomponents_ = compnrs;
    return true;
}


void Seis::SequentialReader::setDataChar( DataCharacteristics::UserType type )
{ dc_ = DataCharacteristics(type); }


void Seis::SequentialReader::setScaler( Scaler* newsc )
{
    delete scaler_;
    scaler_ = newsc ? newsc->clone() : 0;
//    rdr_.forceFloatData( scaler_ ); // Not sure if needed
}


void Seis::SequentialReader::setComponentScaler( const Scaler& scaler,
						 int compidx )
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


void Seis::SequentialReader::adjustDPDescToScalers( const BinDataDesc& trcdesc )
{
    const DataCharacteristics floatdc( DataCharacteristics::F32 );
    const BinDataDesc floatdesc( floatdc );
    if ( dp_ && dp_->getDataDesc() == floatdesc )
	return;

    bool needadjust = false;
    for ( int idx=0; idx<compscalers_.size(); idx++ )
    {
	if ( !compscalers_[idx] ||
	     compscalers_[idx]->isEmpty() || trcdesc == floatdesc )
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
	dp_->addComponentNoInit( compnms.get(idx).str() );
}


RegularSeisDataPack* Seis::SequentialReader::getDataPack()
{ return dp_; }


bool Seis::SequentialReader::init()
{
    if ( initialized_ )
	return true;

    msg_ = tr("Initializing reader");
    delete seissummary_;
    seissummary_ = new ObjectSummary( *ioobj_ );
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

    is2d_ = seissummary_->is2D();
    const SeisIOObjInfo& seisinfo = seissummary_->getFullInformation();
    TrcKeyZSampling seistkzs( tkzs_ );
    seisinfo.getRanges( seistkzs );
    const DataCharacteristics datasetdc( seissummary_->getDataChar() );

    if ( components_.isEmpty() )
    {
	const int nrcomps = seisinfo.nrComponents();
	for ( int idx=0; idx<nrcomps; idx++ )
	    components_ += idx;
    }

    for ( int idx=0; idx<components_.size(); idx++ )
    {
	if ( !compscalers_.validIdx(idx) )
	    compscalers_ += 0;
    }

    adjustDPDescToScalers( datasetdc );
    if ( is2d_ && !tkzs_.is2D() )
    {
	pErrMsg("TrcKeySampling for 2D data needed with GeomID as lineNr");
	return false;
    }

    if ( !dp_ )
    {
	if ( is2d_ )
	{
	    const Pos::GeomID geomid = tkzs_.hsamp_.start_.lineNr();
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
		tkzs_. limitTo( storedtkzs );
	    else
		tkzs_ = storedtkzs;
	}

	dp_ = new RegularSeisDataPack( SeisDataPack::categoryStr(true,false),
				       &dc_);
	DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );
	dp_->setSampling( tkzs_ );
	dp_->setName( ioobj_->name() );
	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( !addComponents(*dp_,*ioobj_,components_,msg_) )
	    return false;
    }

    PosInfo::CubeData cubedata;
    if ( rdr_.get3DGeometryInfo(cubedata) )
	dp_->setTrcsSampling( new PosInfo::SortedCubeData(cubedata) );

    nrdone_ = 0;

    seistkzs.hsamp_.limitTo( tkzs_.hsamp_ );
    sd_ = new Seis::RangeSelData( seistkzs );
    rdr_.setSelData( sd_ );
    if ( !rdr_.prepareWork() )
    {
	msg_ = rdr_.errMsg();
	return false;
    }

    if ( !trcssampling_ )
	trcssampling_ = new PosInfo::CubeData( *dp_->getTrcsSampling() );

    trcssampling_->limitTo( tkzs_.hsamp_ );
    delete trcsiterator3d_;
    trcsiterator3d_ = new PosInfo::CubeDataIterator( *trcssampling_ );
    totalnr_ = trcssampling_->totalSize();

    samedatachar_ = seissummary_->hasSameFormatAs( dp_->getDataDesc() );
    dpzsamp_ = dp_->sampling().zsamp_;
    dpzsamp_.limitTo( seissummary_->zRange() );
    needresampling_ = !dpzsamp_.isCompatible( seissummary_->zRange() );

    initialized_ = true;
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() )
						  .arg( ioobj_->uiName() ) );
    return true;
}


bool Seis::SequentialReader::setDataPack( RegularSeisDataPack& dp,
					  od_ostream* extstrm )
{
    initialized_ = false;
    DPM( DataPackMgr::SeisID() ).release( dp_ );
    dp_ = &dp;
    DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );
    setDataChar( DataCharacteristics( dp.getDataDesc() ).userType() );
    setScaler( dp.getScaler() && !dp.getScaler()->isEmpty()
	       ? dp.getScaler()->clone() : 0 );
    //scaler_ won't be used with external dp, but setting it for consistency

    if ( dp.sampling().isDefined() )
	tkzs_ = dp.sampling();
    else
	dp_->setSampling( tkzs_ );

    if ( dp.nrComponents() < components_.size() &&
	 !addComponents(*dp_,*ioobj_,components_,msg_) )
    {
	if ( extstrm )
	    *extstrm << msg_.getFullString() << od_endl;

	return false;
    }

    if ( compscalers_.size() < components_.size() )
    {
	for ( int idx=compscalers_.size();idx<components_.size(); idx++ )
	    compscalers_ += 0;
    }

    return init(); // New datapack, hence re-init of trace reader
}

namespace Seis
{

static bool fillTrcsBuffer( SeisTrcReader& rdr, RawTrcsSequence& databuf )
{
    SeisTrc trc( 0 );
    const int nrpos = databuf.nrPositions();
    for ( int ipos=0; ipos<nrpos; ipos++ )
    {
	if ( !rdr.get(trc) )
	    return false;

	databuf.copyFrom( trc, &ipos );
    }

    return true;
}

}; //namespace Seis

#define cTrcChunkSz	1000


int Seis::SequentialReader::nextStep()
{
    if ( !initialized_ && !init() )
	return ErrorOccurred();

    if ( nrdone_ == 0 )
	submitUdfWriterTasks();

    if ( nrdone_ >= totalnr_ )
	return Finished();

    if ( Threads::WorkManager::twm().queueSize(queueid_) >
	 100*Threads::WorkManager::twm().nrThreads() )
	return MoreToDo();

    int nrposperchunk = cTrcChunkSz;
    TypeSet<TrcKey>* tks = new TypeSet<TrcKey>;
    if ( !getTrcsPosForRead(nrposperchunk,*tks) )
	{ delete tks; return Finished(); }

    RawTrcsSequence* databuf = new RawTrcsSequence( *seissummary_,
						    tks->size() );
    if ( databuf ) databuf->setPositions( *tks );
    if ( !databuf || !databuf->isOK() || !fillTrcsBuffer(rdr_,*databuf) )
    {
	if ( !databuf ) delete tks;
	delete databuf;
	msg_ = tr("Cannot allocate trace data");
	return ErrorOccurred();
    }

    const TypeSet<int>& outcomponents = !outcomponents_.isEmpty()
				      ? outcomponents_ : components_;
    Task* task = new ArrayFiller( *databuf, dpzsamp_, samedatachar_,
				  needresampling_, components_,
				  compscalers_, outcomponents, *dp_, is2d_ );
    nrdone_ += databuf->nrPositions();
    Threads::WorkManager::twm().addWork(
		Threads::Work(*task,true), 0, queueid_, false, false, true );

    return MoreToDo();
}


bool Seis::SequentialReader::getTrcsPosForRead( int& desirednrpos,
						TypeSet<TrcKey>& tks ) const
{
    tks.setEmpty();
    BinID bid;
    for ( int idx=0; idx<desirednrpos; idx++ )
    {
	if ( !trcsiterator3d_->next(bid) )
	    break;

	tks += TrcKey( tkzs_.hsamp_.survid_, bid );
    }

    desirednrpos = tks.size();

    return !tks.isEmpty();
}


void Seis::SequentialReader::submitUdfWriterTasks()
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
	    Threads::Work(*udfwriters,true),0,queueid_,false,false,true);
}



Seis::RawTrcsSequence::RawTrcsSequence( const ObjectSummary& info, int nrpos )
    : info_(info)
    , nrpos_(nrpos)
    , tks_(0)
    , intpol_(0)
{
    TraceData td;
    for ( int icomp=0; icomp<info.nrcomp_; icomp++ )
	td.addComponent( info.nrsamppertrc_, info.getDataChar() );

    if ( !td.allOk() )
	return;

    for ( int idx=0; idx<nrpos; idx++ )
    {
	TraceData* newtd = new TraceData( td );
	if ( !newtd || !newtd->allOk() )
	{
	    delete newtd; deepErase( data_ );
	    return;
	}

	data_ += newtd;
    }
}


Seis::RawTrcsSequence::~RawTrcsSequence()
{
    deepErase( data_ );
    delete tks_;
}


bool Seis::RawTrcsSequence::isOK() const
{
    return data_.size() == nrpos_ && info_.isOK() && tks_ &&
	   tks_->size() == nrpos_;
}


const ValueSeriesInterpolator<float>&
				  Seis::RawTrcsSequence::interpolator() const
{
    if ( !intpol_ )
    {
	ValueSeriesInterpolator<float>* newintpol =
					new ValueSeriesInterpolator<float>();
	newintpol->snapdist_ = Seis::cDefSampleSnapDist();
	newintpol->smooth_ = true;
	newintpol->extrapol_ = false;
	newintpol->udfval_ = 0;

	if ( !intpol_.setIfNull(newintpol) )
	    delete newintpol;
    }

    intpol_->maxidx_ = info_.nrsamppertrc_ - 1;

    return *intpol_;
}


bool Seis::RawTrcsSequence::isPS() const
{ return info_.isPS(); }


const DataCharacteristics Seis::RawTrcsSequence::getDataChar() const
{ return info_.getDataChar(); }


const StepInterval<float>& Seis::RawTrcsSequence::getZRange() const
{ return info_.zsamp_; }


int Seis::RawTrcsSequence::nrPositions() const
{
    return tks_ ? nrpos_ : 0;
}


void Seis::RawTrcsSequence::setPositions( const TypeSet<TrcKey>& tks )
{ tks_ = &tks; }


const TrcKey& Seis::RawTrcsSequence::getPosition( int ipos ) const
{ return (*tks_)[ipos]; }


float Seis::RawTrcsSequence::get( int idx, int pos, int comp ) const
{ return data_[pos]->getValue( idx, comp ); }


float Seis::RawTrcsSequence::getValue( float z, int pos, int comp ) const
{
    const int sz = info_.nrsamppertrc_;
    const int sampidx = info_.zsamp_.getIndex( z );
    if ( sampidx < 0 || sampidx >= sz )
	return interpolator().udfval_;

    const float samppos = ( z - info_.zsamp_.start ) / info_.zsamp_.step;
    if ( sampidx-samppos > -cDefSampleSnapDist() &&
	 sampidx-samppos <  cDefSampleSnapDist() )
	return get( sampidx, pos, comp );

    return interpolator().value( RawTrcsSequenceValueSeries(*this,pos,comp),
				 samppos );
}


void Seis::RawTrcsSequence::set( int idx, float val, int pos, int comp )
{ data_[pos]->setValue( idx, val, comp ); }


const unsigned char* Seis::RawTrcsSequence::getData( int ipos, int icomp,
						     int is ) const
{
    const int offset = is > 0 ? is * info_.nrbytespersamp_ : 0;

    return data_[ipos]->getComponent(icomp)->data() + offset;
}


unsigned char* Seis::RawTrcsSequence::getData( int ipos, int icomp, int is )
{
    return const_cast<unsigned char*>(
     const_cast<const Seis::RawTrcsSequence&>( *this ).getData(ipos,icomp,is) );
}


void Seis::RawTrcsSequence::copyFrom( const SeisTrc& trc, int* ipos )
{
    int pos = ipos ? *ipos : -1;
    const bool is2d = info_.is2D();
    if ( tks_ )
    {
	if ( !ipos )
	{
	    for ( int idx=0; idx<nrpos_; idx++ )
	    {
		if ( (is2d && trc.info().nr != (*tks_)[idx].trcNr() ) ||
		    (!is2d && trc.info().binID() != (*tks_)[idx].position() ) )
		{
		    pErrMsg("wrong position");
		    continue;
		}

		pos = idx;
		break;
	    }
	}
#ifdef __debug__
	else
	{
	    if ( (is2d && trc.info().nr != (*tks_)[*ipos].trcNr() ) ||
		(!is2d && trc.info().binID() != (*tks_)[*ipos].position() ) )
		pErrMsg("wrong position");
	}
#endif
    }

    for ( int icomp=0; icomp<info_.nrcomp_; icomp++ )
    {
	if ( *trc.data().getInterpreter(icomp) ==
	     *data_[pos]->getInterpreter(icomp) )
	{
	    const od_int64 nrbytes = info_.nrdatabytespespercomptrc_;
	    OD::sysMemCopy( getData( pos, icomp ),
			    trc.data().getComponent( icomp )->data(), nrbytes);
	}
	else
	{
	    const int nrz = info_.zRange().nrSteps()+1;
	    for ( int idz=0; idz<nrz; idz++ )
	    {
		const float val = trc.get( idz, icomp );
		set( idz, val, pos, icomp );
	    }
	}
    }
}



Seis::RawTrcsSequenceValueSeries::RawTrcsSequenceValueSeries(
					const Seis::RawTrcsSequence& seq,
					int pos, int comp )
    : seq_(const_cast<Seis::RawTrcsSequence&>(seq))
    , ipos_(pos)
    , icomp_(comp)
{
}


Seis::RawTrcsSequenceValueSeries::~RawTrcsSequenceValueSeries()
{
}


ValueSeries<float>* Seis::RawTrcsSequenceValueSeries::clone() const
{ return new RawTrcsSequenceValueSeries( seq_, ipos_, icomp_ ); }


void Seis::RawTrcsSequenceValueSeries::setValue( od_int64 idx, float val )
{ seq_.set( (int)idx, val, ipos_, icomp_ ); }


float* Seis::RawTrcsSequenceValueSeries::arr()
{ return (float*)seq_.getData(ipos_,icomp_); }


float Seis::RawTrcsSequenceValueSeries::value( od_int64 idx ) const
{ return seq_.get( (int)idx, ipos_, icomp_ ); }

const float* Seis::RawTrcsSequenceValueSeries::arr() const
{ return (float*)seq_.getData(ipos_,icomp_); }
