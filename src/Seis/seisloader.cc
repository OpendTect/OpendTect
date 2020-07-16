/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/


#include "seisloader.h"

#include "arrayndalgo.h"
#include "binnedvalueset.h"
#include "cbvsreadmgr.h"
#include "convmemvalseries.h"
#include "datapackbase.h"
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
#include "seisrawtrcsseq.h"
#include "seisrangeseldata.h"
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
    const SeisIOObjInfo ioobjinf( ioobj ); BufferStringSet compnms;
    ioobjinf.getComponentNames( compnms );

    const int nrcomp = selcomponents.size();
    od_int64 totmem, freemem;
    OD::getSystemMemory( totmem, freemem );
    const od_int64 reqsz = ((od_int64)nrcomp) * dp.subSel().totalSize() *
			    dp.getDataDesc().nrBytes();
			    // dp.nrKBytes() cannot be used before allocation

    if ( reqsz >= freemem )
	{ msg = uiStrings::phrCannotAllocateMemory( reqsz ); return false; }

    msg = uiStrings::phrAllocating( reqsz );
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	const int cidx = selcomponents[idx];
	const char* cnm = compnms.size()>1 && compnms.validIdx(cidx) ?
			  compnms.get(cidx).buf() : "";

	// assembles composite "<attribute>|<component>" name
	const StringPair compstr( ioobj.name().str(), cnm );
	if ( !dp.addComponent(compstr.getCompString(),false) )
	    return false;
    }

    return true;
}



class ArrayFiller : public Task
{ mODTextTranslationClass(ArrayFiller)
public:
ArrayFiller( const RawTrcsSequence& rawseq, const StepInterval<float>& zsamp,
	     bool samedatachar, bool needresampling,
	     const TypeSet<int>& components,
	     const ObjectSet<Scaler>& compscalers,
	     const TypeSet<int>& outcomponents,
	     RegularSeisDataPack& dp, bool is2d )
    : Task("Datapack Array Filler")
    , rawseq_(rawseq)
    , zsamp_(zsamp)
    , samedatachar_(samedatachar)
    , needresampling_(needresampling)
    , components_(components)
    , compscalers_(compscalers)
    , outcomponents_(outcomponents)
    , dp_(&dp),is2d_(is2d)
{
    const auto dpzrg = dp.zSubSel().zRange();
    startidx0_ = dpzrg.nearestIndex( zsamp_.start );
    stopidx0_ = dpzrg.nearestIndex( zsamp_.stop );
    nrzsamples_ = zsamp_.nrSteps() + 1;
    dpnrzsamples_ = dpzrg.nrSteps() + 1;
    needsudfpaddingattop_ = dpzrg.start < zsamp_.start;
    needsudfpaddingatbottom_ = dpzrg.stop > zsamp_.stop;
    nrpadtail_ = needsudfpaddingatbottom_ ? dpnrzsamples_-stopidx0_-1 : 0;
    trczidx0_ = rawseq_.getZRange().nearestIndex( zsamp_.atIndex(0) );
    bytespersamp_ = dp.getDataDesc().nrBytes();
    nrbytes_ = nrzsamples_ * bytespersamp_;
}


~ArrayFiller()
{
    delete &rawseq_;
}


virtual uiString message() const
{ return msg_; }


bool execute()
{
    if ( !dp_ || dp_->nrComponents() != outcomponents_.size() )
	{ msg_ = mINTERNAL( "Filler incorrectly setup" ); return false; }

    for ( int itrc=0; itrc<rawseq_.nrpos_; itrc++ )
    {
	if ( !doTrace(itrc) )
	    return false;
    }

    dp_ = 0;

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
    RegularSeisDataPack& dp = *dp_;
    const TrcKey& tk = rawseq_.tks_->get( itrc );
    const int idx0 = is2d_ ? 0 : dp.horSubSel().idx4LineNr( tk.lineNr() );
    const int idx1 = dp.horSubSel().idx4TrcNr( tk.trcNr() );
    const bool hastrcscaler = rawseq_.trcscalers_[itrc];

    for ( int cidx=0; cidx<outcomponents_.size(); cidx++ )
    {
	const int idcin = components_[cidx];
	const Scaler* compscaler = compscalers_[cidx];
	const int idcout = outcomponents_[cidx];
#ifdef __debug__
	if ( !dp.validComp(idcout) )
	    mpErrRet("Array Filler is incorrectly setup")
#endif
	Array3D<float>& arr = dp.data( idcout );
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
	     !compscaler && !hastrcscaler )
	{
	    const unsigned char* srcptr = rawseq_.getData( itrc, idcin,
							   trczidx0_ );
#ifdef __debug__
	    const char* storstartptr = storarr;
	    const char* storstopptr  = storarr +
				       arr.totalSize() * bytespersamp_;
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
	    const float* storstopptr  = storptr + arr.totalSize();
#endif
	    for ( int zidx=0; zidx<nrzsamples_; zidx++ )
	    {
		const float rawval = needresampling_
				   ? rawseq_.getValue( zval, itrc, idcin )
				   : rawseq_.get( trczidx++, itrc, idcin );
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
	    const float* storstopptr  = storptr + arr.totalSize();
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

private:

    const RawTrcsSequence&	rawseq_;
    const StepInterval<float>&	zsamp_;
    const TypeSet<int>&		components_;
    const ObjectSet<Scaler>&	compscalers_;
    const TypeSet<int>&		outcomponents_;
    RefMan<RegularSeisDataPack>	dp_;
    bool			is2d_;
    bool			samedatachar_;
    bool			needresampling_;
    uiString			msg_;

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



Seis::Loader::Loader( const IOObj& ioobj, const GeomSubSel* gss,
		      const TypeSet<int>* components )
    : dpismine_(false)
    , ioobj_(ioobj.clone())
    , dc_(OD::AutoDataRep)
    , queueid_(Threads::WorkManager::cDefaultQueueID())
    , udftraceswritefinished_(true)
{
    compscalers_.setNullAllowed( true );
    const SeisIOObjInfo info( ioobj );
    is2d_ = info.is2D();
    if ( gss )
    {
	if ( gss->is2D() != is2d_ )
	    { pErrMsg("2D/3D err"); }
	else
	    reqss_ = gss->duplicate();
    }
    if ( !reqss_ )
    {
	if ( is2d_ )
	{
	    GeomIDSet geomids;
	    info.getGeomIDs( geomids );
	    reqss_ = new LineSubSel( geomids.isEmpty() ? Pos::GeomID()
		: geomids.first() );
	}
	else
	    reqss_ = new CubeSubSel;
    }

    if ( components )
	setComponents( *components );

    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( ioobj, &uirv );
    if ( prov )
	prov->setSelData( new Seis::RangeSelData(*reqss_) );
    if ( prov && uirv.isOK() )
	setTracePositionsFromProv( *prov );

    delete prov;
    const Pos::GeomID geomid = reqss_->geomID();
    seissummary_ = new ObjectSummary( ioobj, geomid );
    msg_ = uiStrings::phrReading( toUiString("%1 '%2'")
			    .arg( uiStrings::sSeisObjName(is2d_,!is2d_,false) )
			    .arg( ioobj_->name() ) );
    if ( is2d_ )
    {
	const BufferString linenm( geomid.name() );
	if ( !linenm.isEmpty() )
	    msg_.constructWordWith( toUiString("|%1" ).arg( linenm.str() ) );
    }
}


Seis::Loader::~Loader()
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    delete ioobj_;
    delete reqss_;
    deepErase( compscalers_ );
    delete scaler_;
    delete seissummary_;
    delete trcposns_;
}


void Seis::Loader::setDataChar( DataCharacteristics::UserType type )
{
    dc_ = DataCharacteristics(type);
}


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


bool Seis::Loader::setOutputComponents()
{
    if ( !dp_ || dp_->nrComponents() != components_.size() )
	return false;

    const int nrcomps = dp_->nrComponents();
    outcomponents_.setEmpty();
    for ( int idx=0; idx<nrcomps; idx++ )
	outcomponents_.add( idx );

    return true;
}


void Seis::Loader::setScaler( const Scaler* newsc )
{
    delete scaler_;
    scaler_ = newsc ? newsc->clone() : 0;
}


uiString Seis::Loader::nrDoneText() const
{
    return tr("%1 read", "Traces read(past tense)")
		.arg( uiStrings::sTrace(mPlural) );
}


void Seis::Loader::setTracePositionsFromProv( const Provider& prov )
{
    delete trcposns_;
    trcposns_ = prov.possiblePositions().clone();
    trcposns_->limitTo( reqss_->horSubSel() );
    totalnr_ = trcposns_->totalSize();
    if ( dp_ )
	dp_->setTracePositions( trcposns_->clone() );
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


void Seis::Loader::arrayFillerCB( CallBacker* cb )
{
    const uiString msg = Threads::WorkManager::twm().message( cb );
    const bool res = msg.isEmpty();
    if ( res )
	return;

    arrayfillererror_ = !res;
    msg_ = msg;
}


void Seis::Loader::udfTracesWrittenCB( CallBacker* )
{
    udftraceswritefinished_ = true;
}


void Seis::Loader::submitUdfWriterTasks()
{
    if ( !trcposns_ || trcposns_->isLinesData() )
	return;
    const Survey::HorSubSel& reqhss = reqss_->horSubSel();
    const PosInfo::CubeData& cubeposns = *trcposns_->asCubeData();
    if ( cubeposns.totalSizeInside(*reqhss.asCubeHorSubSel()) >=
	 reqhss.totalSize() )
	return;

    TaskGroup* udfwriters = new TaskGroup;
    for ( int idx=0; idx<dp_->nrComponents(); idx++ )
    {
	udfwriters->addTask(
		new Array3DUdfTrcRestorer<float>( *trcposns_, reqhss,
						  dp_->data(idx) ) );
    }

    CallBack cb = mCB( this, Seis::Loader, udfTracesWrittenCB );
    udftraceswritefinished_ = false;
    Threads::WorkManager::twm().addWork(
	    Threads::Work(*udfwriters,true),&cb,queueid_,false,false,true);
}


void Seis::Loader::releaseDP()
{
    while ( !udftraceswritefinished_ )
	Threads::sleep( 0.1 );

    //Release external DP: This task no longer needs it
    if ( !dpismine_ )
	dp_ = 0;
}


ConstRefMan<RegularSeisDataPack> Seis::Loader::getDataPack()
{
    if ( !dp_ )
	return 0;

    ConstRefMan<RegularSeisDataPack> dpman( dp_ );
    dp_ = 0;
    dpismine_ = false;

    return dpman;
}



Seis::ParallelFSLoader3D::ParallelFSLoader3D( const IOObj& ioobj,
					      const CubeSubSel& css )
    : Seis::Loader(ioobj,&css,0)
{
    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::SingleThread,
				"ParallelFSLoader3D" );
}


Seis::ParallelFSLoader3D::~ParallelFSLoader3D()
{
    deepErase( csss_ );
}


bool Seis::ParallelFSLoader3D::executeParallel( bool yn )
{
    const bool success = ParallelTask::executeParallel( yn );
    Threads::WorkManager::twm().emptyQueue( queueid_, success );
    releaseDP();

    return success;
}


void Seis::ParallelFSLoader3D::setDataPack( RegularSeisDataPack* dp )
{
    dp_ = dp;
    dpismine_ = false;
}


uiString Seis::ParallelFSLoader3D::nrDoneText() const
{ return Seis::Loader::nrDoneText(); }

uiString Seis::ParallelFSLoader3D::message() const
{ return Seis::Loader::message(); }


bool Seis::ParallelFSLoader3D::doPrepare( int nrthreads )
{
    arrayfillererror_ = false;
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

    const SeisIOObjInfo& seisinfo = seissummary_->ioObjInfo();
    const DataCharacteristics datasetdc( seissummary_->dataChar() );
    if ( dc_.userType() == OD::AutoDataRep )
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
    if ( !dp_ )
    {
	dp_ = new RegularSeisDataPack( VolumeDataPack::categoryStr(true,false),
				       &dc_ );
	dp_->setName( ioobj_->name() );
	dp_->setSubSel( *reqss_ );
	if ( trcposns_ && !trcposns_->isEmpty() )
	    dp_->setTracePositions( trcposns_->clone() );

	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( addComponents(*dp_,*ioobj_,components_,msg_) )
	    dpismine_ = true;
	else
	    { releaseDP(); return false; }
    }

    if ( !setOutputComponents() )
	{ releaseDP(); return false; }

    submitUdfWriterTasks();
    deepErase( csss_ );
    const auto& css = *reqss_->asCubeSubSel();
    for ( int idx=0; idx<nrthreads; idx++ )
	csss_.add( new CubeSubSel( css, nrthreads, idx ) );

    return true;
}


bool Seis::ParallelFSLoader3D::doWork( od_int64 start, od_int64 stop,
				       int threadid )
{
    if ( !csss_.validIdx(threadid) || !trcposns_ || trcposns_->isEmpty() )
	return false;
    const CubeSubSel& css = *csss_.get( threadid );

    uiRetVal uirv;
    PtrMan<Seis::Provider> prov = Seis::Provider::create( *ioobj_, &uirv );
    if ( !prov || !uirv.isOK() )
	{ msg_ = uirv; return false; }
    if ( prov )
	prov->setSelData( new Seis::RangeSelData(css) );

    const Seis::ObjectSummary seissummary( *ioobj_ );
    RawTrcsSequence* rawseq = new RawTrcsSequence( seissummary, 1 );
    if ( !rawseq )
	return false;
    TypeSet<TrcKey>* tkss = new TypeSet<TrcKey>;
    *tkss += TrcKey( css.cubeHorSubSel().atGlobIdx(0) );
    rawseq->setPositions( *tkss );
    if ( !seissummary.isOK() || !rawseq->isOK() )
	{ delete rawseq; return false; }

    PosInfo::CubeData cubedata( *trcposns_ );
    cubedata.limitTo( css.cubeHorSubSel() );
    PosInfo::CubeDataIterator trcsiterator( cubedata );

    const auto zsamp = reqss_->zRange();
    const bool samedatachar = seissummary.hasSameFormatAs( dp_->getDataDesc() );
    const bool needresampling = !zsamp.isCompatible( seissummary.zRange() );
    ObjectSet<Scaler> compscalers;
    compscalers.setNullAllowed( true );
    for ( int idx=0; idx<components_.size(); idx++ )
	compscalers += 0;

    ArrayFiller fillertask( *rawseq, zsamp, samedatachar, needresampling,
			    components_, compscalers, outcomponents_, *dp_,
			    false );
    int trcseqpos = 0;
    TrcKey& tk = (*tkss)[trcseqpos];
    int currentinl = tk.inl();
    int nrdone = 0;
    BinID bid;
    while( trcsiterator.next(bid) )
    {
	tk.setPos( bid );
	if ( bid.lineNr() > currentinl )
	{
	    addToNrDone( nrdone ); nrdone = 0;
	    currentinl = bid.lineNr();
	}

	uirv = prov->getNextSequence( *rawseq );
	nrdone++;
	if ( !uirv.isOK() )
	{
	    if ( isFinished(uirv) )
		return true;

	    msg_ = uirv;
	    return false;
	}

	if ( !fillertask.doTrace(trcseqpos) )
	    return false;
    }

    return true;
}



// ParallelFSLoader2D (probably replace by a SequentialFSLoader)
Seis::ParallelFSLoader2D::ParallelFSLoader2D( const IOObj& ioobj,
					      const LineSubSel& lss,
					      const TypeSet<int>* comps )
    : Seis::Loader(ioobj,&lss,comps)
{
    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::SingleThread,
				"ParallelFSLoader2D" );
}


Seis::ParallelFSLoader2D::~ParallelFSLoader2D()
{
}


bool Seis::ParallelFSLoader2D::executeParallel( bool yn )
{
    const bool success = ParallelTask::executeParallel( yn );
    Threads::WorkManager::twm().emptyQueue( queueid_, success );
    releaseDP();

    return success;
}


uiString Seis::ParallelFSLoader2D::nrDoneText() const
{ return Seis::Loader::nrDoneText(); }

uiString Seis::ParallelFSLoader2D::message() const
{ return Seis::Loader::message(); }


bool Seis::ParallelFSLoader2D::doPrepare( int nrthreads )
{
    arrayfillererror_ = false;
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

    const SeisIOObjInfo& seisinfo = seissummary_->ioObjInfo();
    const DataCharacteristics datasetdc( seissummary_->dataChar() );
    if ( dc_.userType() == OD::AutoDataRep )
	setDataChar( datasetdc.userType() );

    const auto& lss = *reqss_->asLineSubSel();
    const Pos::GeomID geomid( lss.geomID() );
    if ( components_.isEmpty() )
    {
	const int nrcomponents = seisinfo.nrComponents( geomid );
	TypeSet<int> components;
	for ( int idx=0; idx<nrcomponents; idx++ )
	    components += idx;

	setComponents( components );
    }

    if ( !dp_ )
    {
	dp_ = new RegularSeisDataPack( VolumeDataPack::categoryStr(true,true),
				       &dc_ );
	dp_->setName( ioobj_->name() );
	dp_->setSubSel( *reqss_ );
	if ( trcposns_ && !trcposns_->isEmpty() )
	    dp_->setTracePositions( trcposns_->clone() );

	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( addComponents(*dp_,*ioobj_,components_,msg_) )
	    dpismine_ = true;
	else
	    { releaseDP(); return false; }
    }

    if ( !setOutputComponents() )
	{ releaseDP(); return false; }

    submitUdfWriterTasks();
    trcnrs_.setEmpty();
    LineCollDataIterator trcsiterator( *trcposns_ );
    while ( trcsiterator.next() )
	trcnrs_.addIfNew( trcsiterator.trcNr() );

    return true;
}


bool Seis::ParallelFSLoader2D::doWork(od_int64 start,od_int64 stop,int threadid)
{
    TypeSet<int> trcnrs( trcnrs_ );
    if ( stop < totalnr_ )
	trcnrs.removeRange( (int)(stop+1), (int)totalnr_ );
    if ( start > 0 )
	trcnrs.removeRange( 0, (int)start-1 );

    if ( trcnrs.isEmpty() )
	return true;

    Interval<int> trcrg( trcnrs[0], trcnrs[trcnrs.size()-1] );
    trcrg.sort();

    const auto& lss = *reqss_->asLineSubSel();
    const auto& lhss = lss.lineHorSubSel();
    uiRetVal uirv;
    PtrMan<Seis::Provider> prov = Seis::Provider::create( *ioobj_, &uirv );
    if ( prov )
	prov->setSelData( new Seis::RangeSelData(lhss) );
    if ( !prov || !uirv.isOK() )
	{ msg_ = uirv; return false; }

    const Seis::ObjectSummary seissummary( *ioobj_, lhss.geomID() );
    RawTrcsSequence* rawseq = new RawTrcsSequence( seissummary, 1 );
    if ( !rawseq )
	return false;
    TypeSet<TrcKey>* tkss = new TypeSet<TrcKey>;
    *tkss += TrcKey( lhss.atGlobIdx(0) ); // Only for GeomSystem
    rawseq->setPositions( *tkss );
    if ( !seissummary.isOK() || !rawseq->isOK() )
	{ delete rawseq; return false; }

    const auto zsamp = lss.zRange();
    const bool samedatachar = seissummary.hasSameFormatAs( dp_->getDataDesc() );
    const bool needresampling = !zsamp.isCompatible( seissummary.zRange() );
    ObjectSet<Scaler> compscalers;
    compscalers.setNullAllowed( true );
    for ( int idx=0; idx<components_.size(); idx++ )
	compscalers += 0;

    ArrayFiller fillertask( *rawseq, zsamp, samedatachar, needresampling,
			    components_, compscalers, outcomponents_,*dp_,true);
    int trcseqpos = 0;
    TrcKey& tk = (*tkss)[trcseqpos];
    for ( int idx=0; idx<trcnrs.size(); idx++ )
    {
	tk.setTrcNr( trcnrs[idx] );
	uirv = prov->getNextSequence( *rawseq );
	addToNrDone(1);
	if ( !uirv.isOK() )
	{
	    if ( isFinished(uirv) )
		return true;

	    msg_ = uirv;
	    return false;
	}

	if ( !fillertask.doTrace(trcseqpos) )
	    return false;
    }

    return true;
}



Seis::SequentialFSLoader::SequentialFSLoader( const IOObj& ioobj,
					      const GeomSubSel* gss,
					      const TypeSet<int>* comps )
    : Seis::Loader(ioobj,gss,comps)
    , Executor("Volume Reader")
    , prov_(0)
    , initialized_(false)
    , nrdone_(0)
    , trcsiterator_(0)
    , samedatachar_(false)
    , needresampling_(true)
{
    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::MultiThread,
				"SequentialFSLoader" );
}


Seis::SequentialFSLoader::~SequentialFSLoader()
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    delete prov_;
    delete trcsiterator_;
}


uiString Seis::SequentialFSLoader::nrDoneText() const
{ return Seis::Loader::nrDoneText(); }

uiString Seis::SequentialFSLoader::message() const
{ return Seis::Loader::message(); }


bool Seis::SequentialFSLoader::goImpl( od_ostream* strm, bool first, bool last,
				       int delay )
{
    initialized_ = false;
    const bool success = Executor::goImpl( strm, first, last, delay );
    Threads::WorkManager::twm().emptyQueue( queueid_, success );
    releaseDP();
    deleteAndZeroPtr( seissummary_ );

    return success;
}


bool Seis::SequentialFSLoader::init()
{
    arrayfillererror_ = false;
    msg_ = tr("Initializing reader");
    const auto geomid = reqss_->geomID();
    delete seissummary_;
    seissummary_ = new ObjectSummary( *ioobj_, geomid );
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

    const SeisIOObjInfo& seisinfo = seissummary_->ioObjInfo();
    PtrMan<Survey::GeomSubSel> usess = reqss_->duplicate();
    PtrMan<Survey::GeomSubSel> avss = seisinfo.getGeomSubSel( geomid );
    if ( !usess || !avss )
    {
	pErrMsg("Invalid sub-selection");
	return false;
    }
    usess->limitTo( *avss );

    const DataCharacteristics datasetdc( seissummary_->dataChar() );
    if ( dc_.userType() == OD::AutoDataRep )
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
    if ( !dp_ )
    {
	dp_ = new RegularSeisDataPack( VolumeDataPack::categoryStr(true,is2d_),
					&dc_);
	dp_->setName( ioobj_->name() );
	dp_->setSubSel( *usess );
	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( addComponents(*dp_,*ioobj_,components_,msg_) )
	    dpismine_ = true;
	else
	    { releaseDP(); return false; }
    }

    if ( !setOutputComponents() )
	{ releaseDP(); return false; }

    uiRetVal uirv;
    delete prov_;
    prov_ = Seis::Provider::create( *ioobj_, &uirv );
    if ( !prov_ || !uirv.isOK() )
    {
	deleteAndZeroPtr( prov_ );
	msg_ = uirv;
	return false;
    }

    prov_->setSelData( new Seis::RangeSelData(*usess) );
    setTracePositionsFromProv( *prov_ );
    delete trcsiterator_;
    trcsiterator_ = new PosInfo::LineCollDataIterator( *trcposns_ );
    nrdone_ = 0;

    samedatachar_ = seissummary_->hasSameFormatAs( dp_->getDataDesc() );
    dpzsamp_ = dp_->zSubSel().zRange();

    needresampling_ = !dpzsamp_.isCompatible( seissummary_->zRange() );
    if ( !needresampling_ )
	dpzsamp_.limitTo( seissummary_->zRange() );
    else
    {
	if ( dpzsamp_.start < seissummary_->zRange().start )
	    dpzsamp_.start = dpzsamp_.snap( seissummary_->zRange().start,
					    OD::SnapUpward );
	if ( dpzsamp_.stop > seissummary_->zRange().stop )
	    dpzsamp_.stop = dpzsamp_.snap( seissummary_->zRange().stop,
					    OD::SnapDownward );
    }

    initialized_ = true;
    msg_ = uiStrings::phrReading( toUiString("%1 '%2'")
			    .arg( uiStrings::sSeisObjName(is2d_,!is2d_,false) )
			    .arg( ioobj_->name() ) );
    if ( is2d_ )
    {
	const BufferString linenm( geomid.name() );
	if ( !linenm.isEmpty() )
	    msg_.constructWordWith( toUiString("|%1" ).arg( linenm.str() ) );
    }

    return true;
}


bool Seis::SequentialFSLoader::setDataPack( RegularSeisDataPack& dp,
					    od_ostream* extstrm )
{
    initialized_ = false;
    dp_ = &dp;
    dpismine_ = false;
    setDataChar( DataCharacteristics( dp.getDataDesc() ).userType() );
    setScaler( dp.getScaler() && !dp.getScaler()->isEmpty()
	       ? dp.getScaler() : 0 );
    //scaler_ won't be used with external dp, but setting it for consistency

    delete reqss_;
    reqss_ = dp.subSel().duplicate();

    if ( dp_->nrComponents() < components_.size() &&
	 !addComponents(*dp_,*ioobj_,components_,msg_) )
    {
	if ( extstrm )
	    *extstrm << toString(msg_) << od_endl;

	return false;
    }

    return init(); // New datapack, hence re-init of provider
}


int Seis::SequentialFSLoader::nextStep()
{
    if ( !initialized_ && !init() )
	{ releaseDP(); return ErrorOccurred(); }

    if ( nrdone_ == 0 )
	submitUdfWriterTasks();

    if ( arrayfillererror_ )
	return ErrorOccurred();

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
	if ( !rawseq )
	    delete tks;
	delete rawseq;
	msg_ = uiStrings::phrCannotAllocateMemory();
	releaseDP();
	return ErrorOccurred();
    }

    const uiRetVal uirv = prov_->getNextSequence( *rawseq );
    if ( !uirv.isOK() )
    {
	delete rawseq;
	if ( isFinished(uirv) )
	    return Finished();

	msg_ = uirv;
	releaseDP();
	return ErrorOccurred();
    }

    Task* task = new ArrayFiller( *rawseq, dpzsamp_, samedatachar_,
				  needresampling_, components_, compscalers_,
				  outcomponents_, *dp_, (*tks)[0].is2D() );
    nrdone_ += rawseq->nrPositions();
    CallBack finishedcb( mCB(this,Seis::Loader,arrayFillerCB) );
    Threads::WorkManager::twm().addWork(
	Threads::Work(*task,true), &finishedcb, queueid_, false, false, true );

    return MoreToDo();
}


#define cTrcChunkSz	1000

bool Seis::SequentialFSLoader::getTrcsPosForRead( TypeSet<TrcKey>& tks ) const
{
    tks.setEmpty();
    BinID bid; Bin2D b2d;
    for ( int idx=0; idx<cTrcChunkSz; idx++ )
    {
	if ( (is2d_ && !trcsiterator_->next(b2d)) ||
	    (!is2d_ && !trcsiterator_->next(bid)) )
	    break;

	tks += is2d_ ? TrcKey( b2d ) : TrcKey( bid );
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
    prov_ = Seis::Provider::create( ioobj, &uirv );
    if ( !prov_ )
	msg_ = uirv;

    if ( !prov_->is2D() && linerg )
    {
	const SeisIOObjInfo info( ioobj );
	PtrMan<Survey::GeomSubSel> avss = info.getGeomSubSel( geomid );
	Seis::RangeSelData* seldata = new Seis::RangeSelData( *avss );
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
    return tr("%1 read", "Traces read(past tense)")
		.arg(uiStrings::sTrace(mPlural));
}


bool Seis::SequentialPSLoader::init()
{
    gatherdp_ = new GatherSetDataPack();
    const StringPair strpair( prov_->name(), geomid_.name());
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
