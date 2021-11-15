/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/


#include "seisparallelreader.h"

#include "arrayndalgo.h"
#include "binidvalset.h"
#include "cbvsreadmgr.h"
#include "convmemvalseries.h"
#include "datapackbase.h"
#include "ioobj.h"
#include "nrbytes2string.h"
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
    , trcscalers_(0)
{
    startidx0_ = dp.sampling().zsamp_.nearestIndex( zsamp_.start );
    stopidx0_ = dp.sampling().zsamp_.nearestIndex( zsamp_.stop );
    nrzsamples_ = zsamp_.nrSteps()+1;
    dpnrzsamples_ = dp.sampling().zsamp_.nrSteps()+1;
    needsudfpaddingattop_ = dp.sampling().zsamp_.start < zsamp_.start;
    needsudfpaddingatbottom_ = dp.sampling().zsamp_.stop > zsamp_.stop;
    nrpadtail_ = needsudfpaddingatbottom_ ? dpnrzsamples_-stopidx0_-1 : 0;
    trczidx0_ = databuf_.getZRange().nearestIndex( zsamp_.atIndex(0) );
    bytespersamp_ = dp.getDataDesc().nrBytes();
    nrbytes_ = nrzsamples_ * bytespersamp_;
}


~ArrayFiller()
{
    delete &databuf_;
    if ( !trcscalers_ )
	return;

    deepErase( const_cast<ObjectSet<Scaler>& >(*trcscalers_) );
    delete trcscalers_;
}


void setTrcScalers( const ObjectSet<Scaler>* trcscalers )
{
    if ( trcscalers_ )
	deepErase( const_cast<ObjectSet<Scaler>&>( *trcscalers_ ) );
    delete trcscalers_;
    trcscalers_ = trcscalers;
}


bool execute()
{
#ifdef __debug__
    if ( dp_.nrComponents() != outcomponents_.size() )
    {
	pErrMsg("Array Filler is incorrectly setup");
	DBG::forceCrash( false );
	return false;
    }
#endif

    const int nrpos = databuf_.nrPositions();
    for ( int itrc=0; itrc<nrpos; itrc++ )
    {
	if ( !doTrace(itrc) )
	    return false;
    }

    return true;
}


#define mpErrRet(msg) \
{ \
    pErrMsg(msg); \
    DBG::forceCrash( false ); \
    return false; \
}


#define mCheckPtr(ptr,writesz) \
{ \
    if ( ptr < storstartptr || ptr > storstopptr ) \
	mpErrRet("Invalid write pointer") \
\
    if ( writesz > 1 ) \
    { \
	if ( ptr+writesz > storstopptr ) \
	    mpErrRet("Invalid write pointer") \
    } \
}

bool doTrace( int itrc )
{
    const TrcKey& tk = databuf_.getPosition( itrc );
    const int idx0 = is2d_ ? 0 : dp_.sampling().hsamp_.lineIdx( tk.lineNr() );
    const int idx1 = dp_.sampling().hsamp_.trcIdx( tk.trcNr() );
    const Scaler* trcscaler = trcscalers_ && trcscalers_->validIdx(itrc)
			    ? (*trcscalers_)[itrc] : 0;

    for ( int cidx=0; cidx<outcomponents_.size(); cidx++ )
    {
	const int idcin = components_[cidx];
	const Scaler* compscaler = compscalers_[cidx];
	const int idcout = outcomponents_[cidx];
#ifdef __debug__
	if ( !dp_.validComp(idcout) )
	    mpErrRet("Array Filler is incorrectly setup")
#endif
	Array3D<float>& arr = dp_.data( idcout );
	ValueSeries<float>* stor = arr.getStorage();
	float* storptr = stor ? stor->arr() : 0;
	mDynamicCastGet(ConvMemValueSeries<float>*,convmemstor,stor);
	char* storarr = convmemstor ? convmemstor->storArr()
				    : (char*)storptr;
	const od_int64 offset =  stor ? arr.info().getOffset( idx0, idx1, 0 )
				      : 0;
	char* dststartptr = storarr ? storarr +(offset+startidx0_)*bytespersamp_
				    : 0;
	if ( storarr && samedatachar_ && !needresampling_ &&
	     !compscaler && !trcscaler )
	{
	    const unsigned char* srcptr = databuf_.getData( itrc, idcin,
							    trczidx0_ );
#ifdef __debug__
	    const char* storstartptr = storarr;
	    const char* storstopptr  = storarr +
				       arr.info().getTotalSz() * bytespersamp_;

	    mCheckPtr(dststartptr,nrbytes_)
#endif
	    OD::sysMemCopy( dststartptr, srcptr, nrbytes_ );
	}
	else
	{
	    int startidx = startidx0_;
	    od_int64 valueidx = stor ? offset+startidx : 0;
	    int trczidx = trczidx0_;
	    float zval = zsamp_.start;
	    float* destptr = storptr ? (float*)dststartptr : 0;
#ifdef __debug__
	    const float* storstartptr = storptr;
	    const float* storstopptr  = storptr + arr.info().getTotalSz();
#endif
	    for ( int zidx=0; zidx<nrzsamples_; zidx++ )
	    {
		float rawval = needresampling_
			     ? databuf_.getValue( zval, itrc, idcin )
			     : databuf_.get( trczidx++, itrc, idcin );
		if ( trcscaler ) rawval = mCast(float,trcscaler->scale(rawval));
		if ( needresampling_ ) zval += zsamp_.step;
		const float trcval = compscaler
				   ? mCast(float,compscaler->scale(rawval) )
				   : rawval;
		if ( storptr )
		{
#ifdef __debug__
		    mCheckPtr(destptr,1)
#endif
		    *destptr++ = trcval;
		}
		else if ( stor )
		    stor->setValue( valueidx++, trcval );
		else
		    arr.set( idx0, idx1, startidx++, trcval );
	    }
	}

	if ( storptr )
	{
#ifdef __debug__
	    const float* storstartptr = storptr;
	    const float* storstopptr  = storptr + arr.info().getTotalSz();
#endif
	    if ( needsudfpaddingattop_ )
	    {
#ifdef __debug__
		mCheckPtr(storptr+offset,startidx0_)
#endif
		OD::sysMemValueSet( storptr + offset, mUdf(float), startidx0_ );
	    }
	    if ( needsudfpaddingatbottom_ )
	    {
#ifdef __debug__
		mCheckPtr(storptr+offset+stopidx0_+1,nrpadtail_)
#endif
		OD::sysMemValueSet( storptr + offset + stopidx0_ + 1,
				    mUdf(float), nrpadtail_ );
	    }
	    continue;
	}

	if ( needsudfpaddingattop_ )
	{
	    if ( stor )
	    {
		for ( int validx=0; validx<startidx0_; validx++ )
		    stor->setValue( offset + validx, mUdf(float) );
	    }
	    else
	    {
		for ( int validx=0; validx<startidx0_; validx++ )
		    arr.set( idx0, idx1, validx, mUdf(float) );
	    }
	}
	if ( needsudfpaddingatbottom_ )
	{
	    if ( stor )
	    {
		for ( int validx=stopidx0_+1; validx<dpnrzsamples_; validx++ )
		    stor->setValue( offset + validx, mUdf(float) );
	    }
	    else
	    {
		for ( int validx=stopidx0_+1; validx<dpnrzsamples_; validx++ )
		    arr.set( idx0, idx1, validx, mUdf(float) );
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
    const ObjectSet<Scaler>*	trcscalers_;
    const TypeSet<int>&		outcomponents_;
    RegularSeisDataPack&	dp_;
    bool			is2d_;
    bool			samedatachar_;
    bool			needresampling_;

    int				startidx0_;
    int				stopidx0_;
    int				nrzsamples_;
    int				dpnrzsamples_;
    bool			needsudfpaddingattop_;
    bool			needsudfpaddingatbottom_;
    int				nrpadtail_;
    int				trczidx0_;
    int				bytespersamp_;
    od_int64			nrbytes_;

};

}; // namespace Seis



Seis::ParallelReader::ParallelReader( const IOObj& ioobj,
				      const TrcKeyZSampling& tkzs )
    : dp_(0)
    , bidvals_(0)
    , tkzs_(tkzs)
    , ioobj_( ioobj.clone() )
{
    const SeisIOObjInfo seisinfo( ioobj );
    const Pos::GeomID gid = tkzs.hsamp_.getGeomID();
    const int nrcomponents = seisinfo.nrComponents( gid );
    for ( int idx=0; idx<nrcomponents; idx++ )
	components_ += idx;

    const Seis::GeomType gt = seisinfo.geomType();
    SeisTrcReader rdr( ioobj, gid, &gt );
    rdr.setSelData( new Seis::RangeSelData(tkzs) );
    if ( rdr.prepareWork() )
    {
	PosInfo::CubeData cubedata;
	if ( rdr.get3DGeometryInfo(cubedata) )
	{
	    cubedata.limitTo( tkzs.hsamp_ );
	    totalnr_ = cubedata.totalSize();
	    trcssampling_ = new PosInfo::SortedCubeData( cubedata );
	}
	else
	    totalnr_ = tkzs.hsamp_.totalNr();
    }
    else
	totalnr_ = tkzs.hsamp_.totalNr();

    errmsg_ = uiStrings::phrReading( tr("%1 \'%2\'")
			    .arg( uiStrings::sVolDataName(false,true,false) )
			    .arg( ioobj_->uiName() ) );
}


Seis::ParallelReader::ParallelReader( const IOObj& ioobj,BinIDValueSet& bidvals,
				      const TypeSet<int>& components )
    : dp_(0)
    , components_( components )
    , bidvals_( &bidvals )
    , ioobj_( ioobj.clone() )
    , totalnr_( bidvals.totalSize() )
{
    errmsg_ = uiStrings::phrReading( tr("%1 \'%2\'")
			    .arg( uiStrings::sVolDataName(false,true,false) )
			    .arg( ioobj_->uiName() ) );
}


Seis::ParallelReader::~ParallelReader()
{
    DPM( DataPackMgr::SeisID() ).release( dp_ );
    delete ioobj_;

    deepErase( tks_ );
    delete trcssampling_;
}


bool Seis::ParallelReader::setOutputComponents()
{
    if ( !dp_ || dp_->nrComponents() != components_.size() )
	return false;

     const int nrcomps = dp_->nrComponents();
     seisrdroutcompmgr_.setEmpty();
     for ( int idx=0; idx<nrcomps; idx++ )
	 seisrdroutcompmgr_ += idx;

     return true;
}


#define mAddNewDP(act) \
{ \
    if ( !dp_ || !DPM( DataPackMgr::SeisID() ).obtain(dp_->id()) ) \
    { \
	dp_ = 0; \
	act; \
    } \
}

void Seis::ParallelReader::setDataPack( RegularSeisDataPack* dp )
{
    DPM( DataPackMgr::SeisID() ).release( dp_ );
    dp_ = dp;
    mAddNewDP(;)
}


RegularSeisDataPack* Seis::ParallelReader::getDataPack()
{
    return dp_;
}


uiString Seis::ParallelReader::uiNrDoneText() const
{ return tr("Traces read"); }


uiString Seis::ParallelReader::uiMessage() const
{
    return errmsg_;
}


void Seis::ParallelReader::submitUdfWriterTasks()
{
    if ( !trcssampling_ || trcssampling_->totalSize() >= tkzs_.hsamp_.totalNr())
	return;

    TaskGroup* udfwriters = new TaskGroup;
    for ( int idx=0; idx<dp_->nrComponents(); idx++ )
    {
	udfwriters->addTask(
		new Array3DUdfTrcRestorer<float>( *trcssampling_, tkzs_.hsamp_,
						  dp_->data(idx) ) );
    }

    Threads::WorkManager::twm().addWork(
		    Threads::Work(*udfwriters,true),0,
		    Threads::WorkManager::cDefaultQueueID(),false,false,true );
}


bool Seis::ParallelReader::doPrepare( int nrthreads )
{
    uiString allocprob = tr("Cannot allocate memory");

    if ( bidvals_ )
    {
	pErrMsg("The bidval-code is not used. Run through step by step, make "
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
	dp_ = new RegularSeisDataPack( SeisDataPack::categoryStr(true,false) );
	dp_->setName( ioobj_->name() );
	DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );
	dp_->setSampling( tkzs_ );
	if (trcssampling_)
	    dp_->setTrcsSampling( new PosInfo::SortedCubeData(*trcssampling_) );

	uiString errmsg;
	if ( !addComponents(*dp_,*ioobj_,components_,errmsg) )
	{
	    DPM( DataPackMgr::SeisID() ).release( dp_ ); dp_ = 0;
	    errmsg_ = allocprob;
	    errmsg_.append( errmsg, true );
	    return false;
	}
    }

    if ( !setOutputComponents() )
    {
	DPM( DataPackMgr::SeisID() ).release( dp_ ); dp_ = 0;
	return false;
    }

    submitUdfWriterTasks();

    deepErase( tks_ );
    for ( int idx=0; idx<nrthreads; idx++ )
        tks_ += new TrcKeySampling( tkzs_.hsamp_.getLineChunk(nrthreads,idx) );


    errmsg_ = uiStrings::phrReading( tr("%1 \'%2\'")
			    .arg( uiStrings::sVolDataName(false,true,false) )
			    .arg( ioobj_->uiName() ) );

    return true;
}


bool Seis::ParallelReader::doWork( od_int64 start, od_int64, int threadid )
{
    if ( !tks_.validIdx(threadid) )
	return false;

    const TrcKeySampling& tks = *tks_[threadid];
    const Pos::GeomID gid = tks.getGeomID();
    const Seis::GeomType gt = Seis::geomTypeOf( tks.is2D(), false );

    SeisTrcReader rdr( *ioobj_, gid, &gt );
    rdr.setSelData( new Seis::RangeSelData(tks) );
    if ( !rdr.prepareWork() )
	{ errmsg_ = rdr.errMsg(); return false; }

    const Seis::ObjectSummary seissummary( *ioobj_ );
    const int nrcrl = tks.nrTrcs();
    auto* databuf = new RawTrcsSequence( seissummary, nrcrl );
    if ( !databuf ) return false;
    ObjectSet<Scaler>* trcscalers = new ObjectSet<Scaler>;
    trcscalers->allowNull( true );

    TypeSet<TrcKey>* tkss = new TypeSet<TrcKey>;
    const TrcKey tkstart = tks.trcKeyAt( 0 );
    for ( int idx=0; idx<nrcrl; idx++ )
    {
	*tkss += tkstart; // Only for SurvID
	(*trcscalers) += 0;
    }
    databuf->setPositions( *tkss );

    if ( !seissummary.isOK() || !databuf->isOK() )
    {
	delete databuf; delete trcscalers;
	return false;
    }

    const StepInterval<float>& zsamp = tkzs_.zsamp_;
    const bool samedatachar = seissummary.hasSameFormatAs( dp_->getDataDesc() );
    const bool needresampling = !zsamp.isCompatible( seissummary.zRange() );
    ObjectSet<Scaler> compscalers;
    compscalers.allowNull( true );
    const TypeSet<int>& outcompnrs = !seisrdroutcompmgr_.isEmpty()
				   ? seisrdroutcompmgr_ : components_;
    for ( int idx=0; idx<components_.size(); idx++ )
	compscalers += 0;

    ArrayFiller fillertask( *databuf, zsamp, samedatachar, needresampling,
			   components_, compscalers, outcompnrs, *dp_, false );
    fillertask.setTrcScalers( trcscalers );
    SeisTrc trc;
    SeisTrcInfo& trcinfo = trc.info();
    int res;
    int currentinl = tks.start_.inl();
    int nrdone = 0;
    do
    {
	res = rdr.get( trcinfo );
	const BinID bid = trcinfo.binID();
	if ( bid.lineNr() > currentinl || res == 0 )
	{
	    addToNrDone( nrdone ); nrdone = 0;
	    currentinl = bid.lineNr();
	}

	nrdone++;
	if ( res == -1 )
	    { errmsg_ = rdr.errMsg(); return false; }
	else if ( res == 0 )
	    break;
	else if ( res == 2 )
	    continue;

	int crlidx = tks.trcIdx( bid.trcNr() );
	(*tkss)[crlidx].setPosition( bid );

	delete trcscalers->replace( crlidx, (Scaler*)rdr.getTraceScaler() );
	TraceData& trcdata = databuf->getTraceData( crlidx );
	if ( !rdr.getData(trcdata) )
	{
	    if ( !rdr.get(trc) )
		{ errmsg_ = rdr.errMsg(); return false; }

	    databuf->copyFrom( trc, &crlidx );
	}

	if ( !fillertask.doTrace(crlidx) )
	    return false;
    } while ( res == 1 );

    return true;
}


bool Seis::ParallelReader::doFinish( bool success )
{ return success; }



// ParallelReader2D (probably replace by a Sequential reader)
Seis::ParallelReader2D::ParallelReader2D( const IOObj& ioobj,Pos::GeomID geomid,
					  const TrcKeyZSampling* tkzs,
					  const TypeSet<int>* comps )
    : ioobj_(ioobj.clone())
    , dc_(DataCharacteristics::Auto)
    , dpclaimed_(false)
    , scaler_(0)
    , dp_(0)
{
    if ( comps )
	components_ = *comps;

    if ( tkzs )
	tkzs_ = *tkzs;
    else
	tkzs_.hsamp_ = TrcKeySampling( geomid );

    totalnr_ = tkzs_.isEmpty() ? 1 : tkzs_.hsamp_.totalNr();
    msg_ = uiStrings::phrReading( tr("%1 \'%2\'")
			    .arg( uiStrings::sVolDataName(true,false,false) )
			    .arg( ioobj_->uiName() ) );
    const BufferString linenm( Survey::GM().getName(geomid) );
    if ( !linenm.isEmpty() )
	msg_.append( tr("|%1" ).arg( linenm.str() ) );

}


Seis::ParallelReader2D::~ParallelReader2D()
{
    delete ioobj_;
    delete scaler_;

    DPM( DataPackMgr::SeisID() ).release( dp_ );
}


void Seis::ParallelReader2D::setDataChar( DataCharacteristics::UserType type )
{ dc_ = DataCharacteristics(type); }

bool Seis::ParallelReader2D::doPrepare( int )
{ return init(); }


bool Seis::ParallelReader2D::init()
{
    const SeisIOObjInfo info( *ioobj_ );
    if ( !info.isOK() ) return false;

    const Seis::GeomType gt = info.geomType();
    const Pos::GeomID geomid = tkzs_.hsamp_.getGeomID();

    if ( dc_.userType() == DataCharacteristics::Auto )
	info.getDataChar( dc_ );
    if ( components_.isEmpty() )
    {
	const int nrcomps = info.nrComponents( geomid );
	for ( int idx=0; idx<nrcomps; idx++ )
	    components_ += idx;
    }

    if ( tkzs_.isEmpty() )
    {
	StepInterval<int> trcrg;
	info.getRanges( geomid, trcrg, tkzs_.zsamp_ );
	tkzs_.hsamp_.setTrcRange( trcrg );
    }

    SeisTrcReader rdr( *ioobj_, geomid, &gt );
    rdr.setSelData( new Seis::RangeSelData(tkzs_.hsamp_) );
    if ( !rdr.prepareWork() )
	{ msg_ = rdr.errMsg(); return false; }

    trcnrs_.setEmpty();
    SeisTrcInfo trcinfo;
    int res;
    do
    {
	res = rdr.get( trcinfo );
	if ( res == -1 )
	    { msg_ = rdr.errMsg(); return false; }
	else if ( res == 0 )
	    break;
	else if ( res == 2 )
	    continue;

    trcnrs_.addIfNew( trcinfo.trcNr() );
    } while ( res==1 );

    dp_ = new RegularSeisDataPack( SeisDataPack::categoryStr(true,true), &dc_ );
    dp_->setName( ioobj_->name() );
    DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );
    dp_->setSampling( tkzs_ );
    if ( scaler_ )
	dp_->setScaler( *scaler_ );

    if ( !addComponents(*dp_,*ioobj_,components_,msg_) )
    {
	DPM( DataPackMgr::SeisID() ).release( dp_ ); dp_ = 0;
	return false;
    }

    msg_ = uiStrings::phrReading( tr("%1 \'%2\'")
			    .arg( uiStrings::sVolDataName(true,false,false) )
			    .arg( ioobj_->uiName() ) );
    const BufferString linenm( Survey::GM().getName(geomid) );
    if ( !linenm.isEmpty() )
	msg_.append( tr("|%1" ).arg( linenm.str() ) );

    return dp_ && dp_->nrComponents() > 0;
}


void Seis::ParallelReader2D::setScaler( Scaler* newsc )
{
    delete scaler_;
    scaler_ = newsc ? newsc->clone() : 0;
}


uiString Seis::ParallelReader2D::uiNrDoneText() const
{ return tr("Traces read"); }

uiString Seis::ParallelReader2D::uiMessage() const
{ return msg_; }

od_int64 Seis::ParallelReader2D::nrIterations() const
{ return totalnr_; }


bool Seis::ParallelReader2D::doWork( od_int64 start,od_int64 stop, int )
{
    if ( stop < totalnr_ )
        trcnrs_.removeRange( mCast(int,stop+1), mCast(int,totalnr_) );
    if ( start > 0 )
        trcnrs_.removeRange( 0, mCast(int,start-1) );

    if ( trcnrs_.isEmpty() )
	return true;

    Interval<int> trcrg(trcnrs_[0], trcnrs_[trcnrs_.size()-1] );
    trcrg.sort();

    const Seis::GeomType gt = Seis::Line;
    const TrcKeySampling& tks = tkzs_.hsamp_;
    const Pos::GeomID geomid = tks.getGeomID();

    SeisTrcReader rdr( *ioobj_, geomid, &gt );
    TrcKeySampling tkschunk( tks );
    tkschunk.setTrcRange( trcrg );
    rdr.setSelData( new Seis::RangeSelData(tkschunk) );
    if ( !rdr.prepareWork() )
	{ msg_.append( rdr.errMsg(), true ); return false; }

    const Seis::ObjectSummary seissummary( *ioobj_, geomid );
    auto* databuf = new RawTrcsSequence( seissummary, 1 );
    if ( !databuf ) return false;

    TypeSet<TrcKey>* tkss = new TypeSet<TrcKey>;
    *tkss += tks.trcKeyAt( mCast(int,start) );
    databuf->setPositions( *tkss );

    if ( !seissummary.isOK() || !databuf->isOK() )
	{ delete databuf; return false; }

    const StepInterval<float>& zsamp = tkzs_.zsamp_;
    const bool samedatachar = seissummary.hasSameFormatAs( dp_->getDataDesc() );
    const bool needresampling = !zsamp.isCompatible( seissummary.zRange() );
    ObjectSet<Scaler> compscalers;
    compscalers.allowNull( true );
    const TypeSet<int>& components = components_;
    for ( int idx=0; idx<components_.size(); idx++ )
	compscalers += 0;

    ArrayFiller fillertask( *databuf, zsamp, samedatachar, needresampling,
			    components, compscalers, components, *dp_, true );

    SeisTrc trc;
    SeisTrcInfo& trcinfo = trc.info();
    int trcseqpos = 0;
    int res;
    TraceData& trcdata = databuf->getTraceData( trcseqpos );
    do
    {
	res = rdr.get( trcinfo );
	const int trcnr = trcinfo.trcNr();
	if ( res == -1 )
	    { msg_ = rdr.errMsg(); return false; }
	else if ( res == 0 )
	    break;
	else if ( res == 2 )
	    continue;

	(*tkss)[trcseqpos].setTrcNr( trcnr );
	// 2D Fetcher takes care of trace scaling
	if ( !rdr.getData(trcdata) )
	{
	    if ( !rdr.get(trc) )
		{ msg_ = rdr.errMsg(); return false; }

	    databuf->copyFrom( trc, &trcseqpos );
	}

	if ( !fillertask.execute() )
	    return false;

	addToNrDone(1);
    } while ( res == 1 );

    return true;
}


bool Seis::ParallelReader2D::doFinish( bool success )
{ return success; }


RegularSeisDataPack* Seis::ParallelReader2D::getDataPack()
{
    dpclaimed_ = true;
    return dp_;
}



Seis::SequentialReader::SequentialReader( const IOObj& ioobj,
					  const TrcKeyZSampling* tkzs,
					  const TypeSet<int>* comps )
    : Executor("Volume Reader")
    , ioobj_(ioobj.clone())
    , dp_(0)
    , scaler_(0)
    , rdr_(*new SeisTrcReader(ioobj))
    , dc_(DataCharacteristics::Auto)
    , initialized_(false)
    , trcssampling_(0)
    , trcsiterator3d_(0)
    , samedatachar_(false)
    , needresampling_(true)
    , seissummary_(0)
    , nrdone_(0)
    , totalnr_(0)
{
    compscalers_.allowNull( true );
    const SeisIOObjInfo info( ioobj );
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

    is2d_ = info.is2D();
    if ( tkzs )
	tkzs_ = *tkzs;
    else if ( is2d_ )
    {
	TypeSet<Pos::GeomID> geomids;
	info.getGeomIDs( geomids );
	if ( !geomids.isEmpty() )
	    tkzs_.hsamp_ = TrcKeySampling( geomids[0] );
    }

    seissummary_ = is2d_ ? new ObjectSummary( ioobj, tkzs_.hsamp_.getGeomID() )
			 : new ObjectSummary( ioobj );
    msg_ = uiStrings::phrReading( tr("%1 \'%2\'")
			    .arg( uiStrings::sVolDataName(is2d_,!is2d_,false) )
			    .arg( ioobj.uiName() ) );
    if ( is2d_ )
    {
	const BufferString linenm(
			   Survey::GM().getName(tkzs_.hsamp_.getGeomID()) );
	if ( !linenm.isEmpty() )
	    msg_.append( tr("|%1" ).arg( linenm.str() ) );

	totalnr_ = tkzs_.nrTrcs();
    }

    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::MultiThread,
				"SequentialReader" );
}


Seis::SequentialReader::~SequentialReader()
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    DPM( DataPackMgr::SeisID() ).release( dp_ );
    delete &rdr_;
    delete ioobj_;
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
    initialized_ = false;
    const bool success = Executor::goImpl( strm, first, last, delay );
    Threads::WorkManager::twm().emptyQueue( queueid_, success );
    deleteAndZeroPtr( seissummary_ );

    return success;
}


bool Seis::SequentialReader::setOutputComponents()
{
    if ( !dp_ || dp_->nrComponents() != components_.size() )
	return false;

    const int nrcomps = dp_->nrComponents();
    outcomponents_.setEmpty();
    for ( int idx=0; idx<nrcomps; idx++ )
	outcomponents_ += idx;

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
    msg_ = tr("Initializing reader");
    is2d_ = tkzs_.hsamp_.is2D();
    delete seissummary_;
    seissummary_ = is2d_ ? new ObjectSummary( *ioobj_, tkzs_.hsamp_.getGeomID())
			 : new ObjectSummary( *ioobj_ );
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

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
    if ( !dp_ )
    {
	if ( is2d_ )
	{
	    const Pos::GeomID geomid = tkzs_.hsamp_.getGeomID();
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

	dp_ = new RegularSeisDataPack( SeisDataPack::categoryStr(true,is2d_),
				       &dc_);
	dp_->setName( ioobj_->name() );
	DPM( DataPackMgr::SeisID() ).addAndObtain( dp_ );
	dp_->setSampling( tkzs_ );
	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( !addComponents(*dp_,*ioobj_,components_,msg_) )
	{
	    DPM( DataPackMgr::SeisID() ).release( dp_ ); dp_ = 0;
	    return false;
	}
    }

    if ( !setOutputComponents() )
    {
	DPM( DataPackMgr::SeisID() ).release( dp_ ); dp_ = 0;
	return false;
    }

    PosInfo::CubeData cubedata;
    if ( rdr_.get3DGeometryInfo(cubedata) )
	dp_->setTrcsSampling( new PosInfo::SortedCubeData(cubedata) );

    nrdone_ = 0;

    seistkzs.hsamp_.limitTo( tkzs_.hsamp_ );
    rdr_.setSelData( new Seis::RangeSelData(seistkzs) );
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
    needresampling_ = !dpzsamp_.isCompatible( seissummary_->zRange() );
    if ( needresampling_ )
    {
	if ( dpzsamp_.start < seissummary_->zRange().start )
	    dpzsamp_.start =
		dpzsamp_.snap( seissummary_->zRange().start, OD::SnapUpward );
	if ( dpzsamp_.stop > seissummary_->zRange().stop )
	    dpzsamp_.stop =
		dpzsamp_.snap( seissummary_->zRange().stop, OD::SnapDownward );
    }
    else
    {
	dpzsamp_.limitTo( seissummary_->zRange() );
    }

    initialized_ = true;
    msg_ = uiStrings::phrReading( tr("%1 \'%2\'")
			    .arg( uiStrings::sVolDataName(is2d_,!is2d_,false) )
			    .arg( ioobj_->uiName() ) );
    if ( is2d_ )
    {
	const BufferString linenm(
			   Survey::GM().getName(tkzs_.hsamp_.getGeomID()) );
	if ( !linenm.isEmpty() )
	    msg_.append( tr("|%1" ).arg( linenm.str() ) );
    }

    return true;
}


bool Seis::SequentialReader::setDataPack( RegularSeisDataPack& dp,
					  od_ostream* extstrm )
{
    initialized_ = false;
    DPM( DataPackMgr::SeisID() ).release( dp_ );
    dp_ = &dp;
    mAddNewDP(return false)

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
	DPM( DataPackMgr::SeisID() ).release( dp_ ); dp_ = 0;
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

static bool fillTrcsBuffer( SeisTrcReader& rdr, TypeSet<TrcKey>& tks,
			    RawTrcsSequence& databuf, TypeSet<float>& refnrs,
			    ObjectSet<Scaler>& trcscalers, uiString& errmsg )
{
    deepErase( trcscalers );
    SeisTrc trc;
    SeisTrcInfo& trcinfo = trc.info();
    const int nrpos = databuf.nrPositions();
    for ( int ipos=0; ipos<nrpos; ipos++ )
    {
	const int res = rdr.get( trcinfo );
	if ( res == -1 )
	    { errmsg = rdr.errMsg(); return false; }
	else if ( res == 0 )
	    return true;
	else if ( res == 2 )
	    continue;

	TrcKey& tk = tks[ipos];
	if ( tk.is2D() )
	{
	    tk.setTrcNr( trcinfo.trcNr() );
	    refnrs[ipos] = trcinfo.refnr;
	}
	else
	    tk.setPosition( trcinfo.binID() );

	trcscalers += (Scaler*)rdr.getTraceScaler();
	if ( !rdr.getData(databuf.getTraceData(ipos)) )
	{
	    if ( !rdr.get(trc) )
		return false;

	    databuf.copyFrom( trc, &ipos );
	}
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
    {
	if ( is2d_ )
	    dp_->setRefNrs( refnrs_ );

	return Finished();
    }

    if ( Threads::WorkManager::twm().queueSize(queueid_) >
	 100*Threads::WorkManager::twm().nrThreads() )
	return MoreToDo();

    int nrposperchunk = cTrcChunkSz;
    TypeSet<TrcKey>* tks = new TypeSet<TrcKey>;
    if ( !getTrcsPosForRead(nrposperchunk,*tks) )
	{ delete tks; return Finished(); }

    auto* databuf = new RawTrcsSequence( *seissummary_, tks->size() );
    ObjectSet<Scaler>* trcscalers = new ObjectSet<Scaler>;
    trcscalers->allowNull( true );
    if ( databuf ) databuf->setPositions( *tks );
    TypeSet<float> refnrs;
    if ( is2d_ )
	refnrs.setSize( tks->size(), 0.f );

    if ( !databuf || !databuf->isOK() ||
	 !fillTrcsBuffer(rdr_,*tks,*databuf,refnrs,*trcscalers,msg_) )
    {
	if ( !databuf ) delete tks;
	delete databuf; delete trcscalers;
	msg_.append( tr("Cannot allocate trace data"), true );
	return ErrorOccurred();
    }

    if ( is2d_ ) refnrs_.append( refnrs );
    ArrayFiller* task = new ArrayFiller( *databuf, dpzsamp_, samedatachar_,
				  needresampling_, components_,
				  compscalers_, outcomponents_, *dp_, is2d_ );
    task->setTrcScalers( trcscalers );
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

	tks += TrcKey( tkzs_.hsamp_.survid_, (const Pos::IdxPair&)bid );
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
