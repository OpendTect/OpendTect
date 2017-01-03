/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seisparallelreader.h"

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

#include "hiddenparam.h"

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
	if ( !dp.addComponent( LineKey(ioobj.name().str(),cnm) ) )
	    return false;
    }

    return true;
}


HiddenParam<ParallelReader,TypeSet<int>* > seisrdroutcompmgr( 0 );

ParallelReader::ParallelReader( const IOObj& ioobj, const TrcKeyZSampling& cs )
    : dp_(0)
    , bidvals_(0)
    , tkzs_(cs)
    , ioobj_( ioobj.clone() )
    , totalnr_( cs.hsamp_.totalNr() )
{
    seisrdroutcompmgr.setParam( this, 0 );
    SeisIOObjInfo seisinfo( ioobj );
    const int nrcomponents = seisinfo.nrComponents();
    for ( int idx=0; idx<nrcomponents; idx++ )
	components_ += idx;
}


ParallelReader::ParallelReader( const IOObj& ioobj,
				BinIDValueSet& bidvals,
				const TypeSet<int>& components )
    : dp_(0)
    , components_( components )
    , bidvals_( &bidvals )
    , ioobj_( ioobj.clone() )
    , totalnr_( bidvals.totalSize() )
{
    seisrdroutcompmgr.setParam( this, 0 );
    errmsg_ = uiStrings::phrReading( ioobj_->uiName() );
}


ParallelReader::~ParallelReader()
{
    DPM( DataPackMgr::SeisID() ).release( dp_ );
    delete ioobj_;
    delete seisrdroutcompmgr.getParam( this );
    seisrdroutcompmgr.removeParam( this );
}


bool ParallelReader::setOutputComponents( const TypeSet<int>& compnrs )
{
    if ( compnrs.size() != components_.size() )
	return false;

    delete seisrdroutcompmgr.getParam( this );
    seisrdroutcompmgr.setParam( this, new TypeSet<int>( compnrs ) );

    return true;
}


void ParallelReader::setDataPack( RegularSeisDataPack* dp )
{
    DPM( DataPackMgr::SeisID() ).release( dp_ );
    dp_ = dp;
    DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );
}


RegularSeisDataPack* ParallelReader::getDataPack()
{
    return dp_;
}


uiString ParallelReader::uiNrDoneText() const
{ return tr("Traces read"); }


uiString ParallelReader::uiMessage() const
{
    return errmsg_.isEmpty() ? tr("Reading volume \'%1\'").arg(ioobj_->name())
			     : errmsg_;
}


bool ParallelReader::doPrepare( int nrthreads )
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


bool ParallelReader::doWork( od_int64 start, od_int64 stop, int threadid )
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

    const TypeSet<int>& outcompnrs = seisrdroutcompmgr.getParam( this )
				   ? *seisrdroutcompmgr.getParam( this )
				   : components_;
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


bool ParallelReader::doFinish( bool success )
{ return success; }



// ParallelReader2D (probably replace by a Sequential reader)
ParallelReader2D::ParallelReader2D( const IOObj& ioobj, Pos::GeomID geomid,
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


bool ParallelReader2D::doPrepare( int )
{
    return init();
}


bool ParallelReader2D::init()
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


ParallelReader2D::~ParallelReader2D()
{
    delete ioobj_;
    delete scaler_;

    DPM( DataPackMgr::SeisID() ).release( dp_ );
}


void ParallelReader2D::setDataChar( DataCharacteristics::UserType type )
{ dc_ = DataCharacteristics(type); }


void ParallelReader2D::setScaler( Scaler* newsc )
{
    delete scaler_;
    scaler_ = newsc ? newsc->clone() : 0;
}


uiString ParallelReader2D::uiNrDoneText() const
{ return tr("Traces read"); }

uiString ParallelReader2D::uiMessage() const
{ return msg_.isEmpty() ? tr("Reading") : msg_; }

od_int64 ParallelReader2D::nrIterations() const
{ return totalnr_; }


bool ParallelReader2D::doWork( od_int64 start, od_int64 stop, int threadid )
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

    const Scaler* scaler = dp_->getScaler();
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
		if ( scaler )
		{
		    SeisTrcPropChg seistrcpropchg( trc, idc );
		    seistrcpropchg.scale( *scaler );
		}

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


bool ParallelReader2D::doFinish( bool success )
{ return success; }


RegularSeisDataPack* ParallelReader2D::getDataPack()
{
    dpclaimed_ = true;
    return dp_;
}



// SequentialReader
class ArrayFiller : public Task
{
public:
ArrayFiller( SeisTrc& trc, const TypeSet<int>& components,
	     const ObjectSet<Scaler>& compscalers,
	     const TypeSet<int>& outcomponents, RegularSeisDataPack& dp )
    : trc_(trc)
    , components_(components)
    , compscalers_(compscalers)
    , outcomponents_(outcomponents)
    , dp_(dp)
{}


~ArrayFiller()
{ delete &trc_; }

bool execute()
{
    const int idx0 = dp_.sampling().hsamp_.lineIdx( trc_.info().binid.inl() );
    const int idx1 = dp_.sampling().hsamp_.trcIdx( trc_.info().binid.crl() );

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
	if ( dp_.getScaler() )
	{
	    SeisTrcPropChg seistrcpropchg( trc_, idcin );
	    seistrcpropchg.scale( *dp_.getScaler() );
	}

	const BinDataDesc trcdatadesc =
		trc_.data().getInterpreter(idcin)->dataChar();
	if ( storarr && dp_.getDataDesc()==trcdatadesc )
	{
	    const DataBuffer* databuf = trc_.data().getComponent( idcin );
	    const int bytespersamp = databuf->bytesPerSample();
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
};


HiddenParam<SequentialReader,TypeSet<int>* > seisseqrdroutcompmgr( 0 );
HiddenParam<SequentialReader,ObjectSet<Scaler>* > seisseqrdrcompscalers( 0 );

SequentialReader::SequentialReader( const IOObj& ioobj,
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
{
    seisseqrdroutcompmgr.setParam( this, 0 );
    ObjectSet<Scaler>* compscalers = new ObjectSet<Scaler>;
    compscalers->allowNull( true );
    seisseqrdrcompscalers.setParam( this, compscalers );
    SeisIOObjInfo info( ioobj );
    info.getDataChar( dc_ );
    if ( !comps )
    {
	const int nrcomps = info.nrComponents();
	for ( int idx=0; idx<nrcomps; idx++ )
	{
	    components_ += idx;
	    *compscalers += 0;
	}
    }
    else
    {
	components_ = *comps;
	for ( int idx=0; idx<components_.size(); idx++ )
	    *compscalers += 0;
    }

    info.getRanges( tkzs_ );
    if ( tkzs )
	tkzs_.limitTo( *tkzs );

    totalnr_ = tkzs_.hsamp_.totalNr();
    nrdone_ = 0;

    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::MultiThread,
				"SequentialReader" );
}


SequentialReader::~SequentialReader()
{
    delete &rdr_; delete ioobj_;
    delete scaler_;

    DPM( DataPackMgr::SeisID() ).release( dp_ );
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    delete seisseqrdroutcompmgr.getParam( this );
    deepErase( *seisseqrdrcompscalers.getParam( this ) );
    delete seisseqrdrcompscalers.getParam( this );
    seisseqrdroutcompmgr.removeParam( this );
    seisseqrdrcompscalers.removeParam( this );
}


uiString SequentialReader::uiNrDoneText() const
{
    return uiStrings::phrJoinStrings( uiStrings::sTrace(mPlural), tr("read") );
}


bool SequentialReader::setOutputComponents( const TypeSet<int>& compnrs )
{
    if ( compnrs.size() != components_.size() )
	return false;

    delete seisseqrdroutcompmgr.getParam( this );
    seisseqrdroutcompmgr.setParam( this, new TypeSet<int>( compnrs ) );

    return true;
}


void SequentialReader::setDataChar( DataCharacteristics::UserType type )
{ dc_ = DataCharacteristics(type); }


void SequentialReader::setScaler( Scaler* newsc )
{
    delete scaler_;
    scaler_ = newsc ? newsc->clone() : 0;
//    rdr_.forceFloatData( scaler_ ); // Not sure if needed
}


void SequentialReader::setComponentScaler( const Scaler& scaler, int compidx )
{
    if ( scaler.isEmpty() )
	return;

    ObjectSet<Scaler>& compscalers = *seisseqrdrcompscalers.getParam( this );
    for ( int idx=0; idx<=compidx; idx++ )
    {
	if ( !compscalers.validIdx(idx) )
	    compscalers += 0;
    }

    delete compscalers.replace( compidx, scaler.clone() );
}


void SequentialReader::adjustDPDescToScalers( const BinDataDesc& trcdesc )
{
    const DataCharacteristics floatdc( DataCharacteristics::F32 );
    const BinDataDesc floatdesc( floatdc );
    if ( dp_ && dp_->getDataDesc() == floatdesc )
	return;

    ObjectSet<Scaler>& compscalers = *seisseqrdrcompscalers.getParam( this );

    bool needadjust = false;
    for ( int idx=0; idx<compscalers.size(); idx++ )
    {
	if ( !compscalers[idx] || compscalers[idx]->isEmpty() ||
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


RegularSeisDataPack* SequentialReader::getDataPack()
{ return dp_; }


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

bool SequentialReader::init()
{
    if ( initialized_ )
	return true;

    nrdone_ = 0;
    msg_ = tr("Initializing reader");

    ObjectSet<Scaler>& compscalers = *seisseqrdrcompscalers.getParam( this );
    for ( int idx=0; idx<components_.size(); idx++ )
    {
	if ( !compscalers.validIdx(idx) )
	    compscalers += 0;
    }

    const SeisIOObjInfo seisinfo( *ioobj_ );
    if ( !seisinfo.isOK() ) return false;

    DataCharacteristics datasetdc;
    seisinfo.getDataChar( datasetdc );
    adjustDPDescToScalers( datasetdc );

    mSetSelData()

    dp_ = new RegularSeisDataPack( SeisDataPack::categoryStr(true,false), &dc_);
    DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );
    dp_->setSampling( tkzs_ );
    dp_->setName( ioobj_->name() );
    if ( scaler_ && !scaler_->isEmpty() )
	dp_->setScaler( *scaler_ );

    if ( !addComponents(*dp_,*ioobj_,components_,msg_) )
	return false;

    PosInfo::CubeData cubedata;
    if ( rdr_.get3DGeometryInfo(cubedata) )
    {
	cubedata.limitTo( tkzs_.hsamp_ );
	if ( !cubedata.isFullyRectAndReg() )
	    dp_->setTrcsSampling( new PosInfo::SortedCubeData(cubedata) );
    }


    initialized_ = true;
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() )
						  .arg( ioobj_->uiName() ) );
    return true;
}


bool SequentialReader::setDataPack( RegularSeisDataPack& dp,
				    od_ostream* extstrm )
{
    nrdone_ = 0;
    DPM( DataPackMgr::SeisID() ).release( dp_ );
    dp_ = &dp;
    DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );

    setDataChar( DataCharacteristics( dp.getDataDesc() ).userType() );
    setScaler( dp.getScaler() && !dp.getScaler()->isEmpty()
	       ? dp.getScaler()->clone() : 0 );
    //scaler_ won't be used with external dp, but setting it for consistency

    if ( dp.sampling().isDefined() )
	tkzs_ = dp.sampling();

    dp.setSampling( tkzs_ );

    mSetSelData()
    if ( dp.nrComponents() < components_.size() &&
	 !addComponents(*dp_,*ioobj_,components_,msg_) )
    {
	if ( extstrm )
	    *extstrm << msg_.getFullString() << od_endl;

	return false;
    }

    ObjectSet<Scaler>& compscalers = *seisseqrdrcompscalers.getParam( this );
    if ( compscalers.size() < components_.size() )
    {
	for ( int idx=compscalers.size(); idx<components_.size(); idx++ )
	    compscalers += 0;
    }

    PosInfo::CubeData cubedata;
    if ( rdr_.get3DGeometryInfo(cubedata) )
	dp_->setTrcsSampling( new PosInfo::SortedCubeData(cubedata) );

    initialized_ = true;
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() )
						  .arg( ioobj_->uiName() ) );
    return true;
}


int SequentialReader::nextStep()
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

    const ObjectSet<Scaler>& compscalers =
					*seisseqrdrcompscalers.getParam( this );
    const TypeSet<int>& outcomponents = seisseqrdroutcompmgr.getParam( this )
				      ? *seisseqrdroutcompmgr.getParam( this )
				      : components_;
    Task* task = new ArrayFiller( *trc, components_, compscalers, outcomponents,
				  *dp_ );
    Threads::WorkManager::twm().addWork(
	Threads::Work(*task,true), 0, queueid_, false, false, true );

    nrdone_++;
    return MoreToDo();
}

} // namespace Seis
