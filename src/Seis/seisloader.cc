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
    , dp_(&dp),is2d_(is2d)
{
}


~ArrayFiller()
{
    delete &rawseq_;
}


bool execute()
{
    if ( !dp_ )
	return false;

    for ( int itrc=0; itrc<rawseq_.nrpos_; itrc++ )
    {
	if ( !doTrace(itrc) )
	    return false;
    }

    dp_ = 0;

    return true;
}

bool doTrace( int itrc )
{
    RegularSeisDataPack& dp = *dp_;
    const TrcKey& tk = (*rawseq_.tks_)[itrc];
    const int idx0 = is2d_ ? 0 : dp.sampling().hsamp_.lineIdx( tk.lineNr() );
    const int idx1 = dp.sampling().hsamp_.trcIdx( tk.trcNr() );

    const int startidx0 = dp.sampling().zsamp_.nearestIndex( zsamp_.start );
    const int stopidx0 = dp.sampling().zsamp_.nearestIndex( zsamp_.stop );
    const int nrzsamples = zsamp_.nrSteps()+1;
    const bool needsudfpaddingattop =
			dp.sampling().zsamp_.start < zsamp_.start;
    const bool needsudfpaddingatbottom =
			dp.sampling().zsamp_.stop > zsamp_.stop;
    const int trczidx0 = rawseq_.getZRange().nearestIndex( zsamp_.atIndex(0) );
    const int bytespersamp = dp.getDataDesc().nrBytes();
    const bool hastrcscaler = rawseq_.trcscalers_[itrc];

    for ( int cidx=0; cidx<outcomponents_.size(); cidx++ )
    {
	const int idcin = components_[cidx];
	const Scaler* compscaler = compscalers_[cidx];
	const int idcout = outcomponents_[cidx];
	Array3D<float>& arr = dp.data( idcout );
	ValueSeries<float>* stor = arr.getStorage();
	float* storptr = stor ? stor->arr() : 0;
	mDynamicCastGet(ConvMemValueSeries<float>*,convmemstor,stor);
	char* storarr = convmemstor ? convmemstor->storArr()
				    : (char*)storptr;
	const od_int64 offset =  storarr ? arr.info().getOffset( idx0, idx1, 0 )
					 : 0;
	char* dststartptr = storarr ? storarr + (offset+startidx0)*bytespersamp
				    : 0;
	if ( storarr && samedatachar_ && !needresampling_ &&
	     !compscaler && !hastrcscaler )
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
		const float trcval = compscaler
				   ? mCast(float,compscaler->scale(rawval) )
				   : rawval;
		if ( storptr )
		    *destptr++ = trcval;
		else if ( stor )
		    stor->setValue( valueidx++, trcval );
		else
		    arr.set( idx0, idx1, startidx++, trcval );
	    }
	}

	if ( needsudfpaddingattop )
	{
	    if ( storptr )
		OD::sysMemValueSet( storptr + offset, mUdf(float), startidx0 );
	    else
	    {
		for ( int validx=0; validx<startidx0; validx++ )
		{
		    if ( stor ) stor->setValue( offset + validx, mUdf(float) );
		    else arr.set( idx0, idx1, validx, mUdf(float) );
		}
	    }
	}
	if ( needsudfpaddingatbottom )
	{
	    if ( storptr )
		OD::sysMemValueSet( storptr + offset + stopidx0 + 1,
				    mUdf(float),
				    dp.sampling().zsamp_.nrSteps() - stopidx0);
	    else
	    {
		const int lastidx = dp.sampling().zsamp_.nrSteps();
		for ( int validx=stopidx0+1; validx<=lastidx; validx++ )
		{
		    if ( stor ) stor->setValue( offset + validx, mUdf(float) );
		    else arr.set( idx0, idx1, validx, mUdf(float) );
		}
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
    RefMan<RegularSeisDataPack>	dp_;
    bool			is2d_;
    bool			samedatachar_;
    bool			needresampling_;
};

}; // namespace Seis



Seis::Loader::Loader( const IOObj& ioobj, const TrcKeyZSampling* tkzs,
		      const TypeSet<int>* components )
    : dpismine_(false)
    , ioobj_(ioobj.clone())
    , tkzs_(false)
    , dc_(OD::AutoFPRep)
    , outcomponents_(0)
    , scaler_(0)
    , line2ddata_(0)
    , trcssampling_(0)
    , queueid_(Threads::WorkManager::cDefaultQueueID())
    , udftraceswritefinished_(true)
{
    compscalers_.setNullAllowed( true );
    const SeisIOObjInfo info( ioobj );
    const bool is2d = info.is2D();
    if ( tkzs )
	tkzs_ = *tkzs;
    else if ( is2d )
    {
	TypeSet<Pos::GeomID> geomids;
	info.getGeomIDs( geomids );
	if ( !geomids.isEmpty() )
	    tkzs_.hsamp_ = TrcKeySampling( geomids[0] );
    }

    if ( components )
	setComponents( *components );

    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( ioobj.key(), &uirv );
    if ( prov ) prov->setSelData( new Seis::RangeSelData(tkzs_) );
    if ( prov && uirv.isOK() )
	setTrcsSamplingFromProv( *prov );

    delete prov;
    const Pos::GeomID geomid = is2d ? tkzs_.hsamp_.getGeomID() : mUdfGeomID;
    seissummary_ = new ObjectSummary( ioobj, geomid );
    msg_ = uiStrings::phrReading( tr("%1 \'%2\'")
			    .arg( uiStrings::sVolDataName(is2d,!is2d,false) )
			    .arg( ioobj_->uiName() ) );
    if ( is2d )
    {
	const BufferString linenm(
			    Survey::GM().getName(tkzs_.hsamp_.getGeomID()) );
	if ( !linenm.isEmpty() )
	    msg_.constructWord( tr("|%1" ).arg( linenm.str() ) );
    }
}


Seis::Loader::~Loader()
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    delete ioobj_;
    deepErase( compscalers_ );
    delete outcomponents_;
    delete scaler_;
    delete seissummary_;
    delete line2ddata_;
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


bool Seis::Loader::setTrcsSamplingFromProv( const Provider& prov )
{
    const TrcKeySampling& tks = tkzs_.hsamp_;
    const bool is2d = tks.is2D();
    delete trcssampling_;
    trcssampling_ = new PosInfo::CubeData();
    if ( is2d )
    {
	mDynamicCastGet(const Provider2D*,prov2d,&prov)
	if ( !prov2d ) return false;
	PosInfo::Line2DData* line2ddata = new PosInfo::Line2DData();
	const Pos::GeomID geomid = tks.getGeomID();
	prov2d->getGeometryInfo( prov2d->lineNr(geomid), *line2ddata );
	line2ddata->limitTo( tks.trcRange() );
	delete line2ddata_;
	line2ddata_ = line2ddata;
	PosInfo::LineData* linedata = new PosInfo::LineData( geomid );
	line2ddata_->getSegments( *linedata );
	trcssampling_->add( linedata );
    }
    else
    {
	mDynamicCastGet(const Provider3D*,prov3d,&prov)
	if ( !prov3d ) return false;
	prov3d->getGeometryInfo( *trcssampling_ );
    }

    trcssampling_->limitTo( tks );
    totalnr_ = is2d ? line2ddata_->size() : trcssampling_->totalSize();
    if ( dp_ )
	dp_->setTrcsSampling( new PosInfo::SortedCubeData(*trcssampling_) );

    return true;
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


void Seis::Loader::udfTracesWrittenCB( CallBacker* )
{
    udftraceswritefinished_ = true;
}


void Seis::Loader::submitUdfWriterTasks()
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

    CallBack cb = mCB( this, Seis::Loader, udfTracesWrittenCB );
    udftraceswritefinished_ = false;
    Threads::WorkManager::twm().addWork(
	    Threads::Work(*udfwriters,true),&cb,queueid_,false,false,true);
}


void Seis::Loader::releaseDP()
{
    while ( !udftraceswritefinished_ )
    {
	Threads::sleep( 0.1 );
    }

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
					      const TrcKeyZSampling& tkzs )
    : Seis::Loader(ioobj,&tkzs,0)
{
    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::SingleThread,
				"ParallelFSLoader3D" );
}


Seis::ParallelFSLoader3D::~ParallelFSLoader3D()
{
    deepErase( tks_ );
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
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

    const SeisIOObjInfo& seisinfo = seissummary_->getFullInformation();
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
    if ( !dp_ )
    {
	dp_ = new RegularSeisDataPack( VolumeDataPack::categoryStr(true,false),
				       &dc_ );
	dp_->setName( ioobj_->name() );
	dp_->setSampling( tkzs_ );
	if ( trcssampling_ )
	    dp_->setTrcsSampling( new PosInfo::SortedCubeData(*trcssampling_) );

	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( addComponents(*dp_,*ioobj_,components_,msg_) )
	    dpismine_ = true;
	else
	    { releaseDP(); return false; }
    }

    submitUdfWriterTasks();

    deepErase( tks_ );
    for ( int idx=0; idx<nrthreads; idx++ )
    {
	tks_.add( new TrcKeySampling(
				  tkzs_.hsamp_.getLineChunk(nrthreads,idx) ) );
    }

    return true;
}


bool Seis::ParallelFSLoader3D::doWork( od_int64 start, od_int64 stop,
				       int threadid )
{
    if ( !tks_.validIdx(threadid) || !trcssampling_ )
	return false;

    const TrcKeySampling& tks = *tks_.get( threadid );
    PosInfo::CubeData cubedata( *trcssampling_ );
    cubedata.limitTo( tks );
    PosInfo::CubeDataIterator trcsiterator3d( cubedata );

    uiRetVal uirv;
    PtrMan<Seis::Provider> prov = Seis::Provider::create( ioobj_->key(), &uirv);
    if ( prov ) prov->setSelData( new Seis::RangeSelData(tks) );
    if ( !prov || !uirv.isOK() )
	{ msg_ = uirv; return false; }

    const Seis::ObjectSummary seissummary( *ioobj_ );
    RawTrcsSequence* rawseq = new RawTrcsSequence( seissummary, 1 );
    if ( !rawseq ) return false;
    TypeSet<TrcKey>* tkss = new TypeSet<TrcKey>;
    const TrcKey tkstart = tks.trcKeyAt( 0 );
    *tkss += tkstart; // Only for SurvID
    rawseq->setPositions( *tkss );
    if ( !seissummary.isOK() || !rawseq->isOK() )
	{ delete rawseq; return false; }

    const StepInterval<float>& zsamp = tkzs_.zsamp_;
    const bool samedatachar = seissummary.hasSameFormatAs( dp_->getDataDesc() );
    const bool needresampling = !zsamp.isCompatible( seissummary.zRange() );
    ObjectSet<Scaler> compscalers;
    compscalers.setNullAllowed( true );
    const TypeSet<int>& outcompnrs =
				outcomponents_ && !outcomponents_->isEmpty()
				   ? *outcomponents_ : components_;
    for ( int idx=0; idx<components_.size(); idx++ )
	compscalers += 0;

    ArrayFiller fillertask( *rawseq, zsamp, samedatachar, needresampling,
			    components_, compscalers, outcompnrs, *dp_, false );
    int trcseqpos = 0;
    TrcKey& tk = (*tkss)[trcseqpos];
    int currentinl = tk.inl();
    int nrdone = 0;
    BinID bid;
    while( trcsiterator3d.next(bid) )
    {
	tk.setPosition( bid );
	if ( bid.lineNr() > currentinl )
	{
	    addToNrDone( nrdone ); nrdone = 0;
	    currentinl = bid.lineNr();
	}

	uirv = prov->getSequence( *rawseq );
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
					      const TrcKeyZSampling& tkzs,
					      const TypeSet<int>* comps )
    : Seis::Loader(ioobj,&tkzs,comps)
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
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

    const SeisIOObjInfo& seisinfo = seissummary_->getFullInformation();
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

    if ( !dp_ )
    {
	dp_ = new RegularSeisDataPack( VolumeDataPack::categoryStr(true,true),
				       &dc_ );
	dp_->setName( ioobj_->name() );
	dp_->setSampling( tkzs_ );
	if ( trcssampling_ )
	    dp_->setTrcsSampling( new PosInfo::SortedCubeData(*trcssampling_) );

	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( addComponents(*dp_,*ioobj_,components_,msg_) )
	    dpismine_ = true;
	else
	    { releaseDP(); return false; }
    }

    submitUdfWriterTasks();
    trcnrs_.setEmpty();
    PosInfo::Line2DDataIterator trcsiterator2d( *line2ddata_ );
    while ( trcsiterator2d.next() )
	trcnrs_.addIfNew( trcsiterator2d.trcNr() );

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

    const TrcKeySampling& tks = tkzs_.hsamp_;

    uiRetVal uirv;
    PtrMan<Seis::Provider> prov = Seis::Provider::create( ioobj_->key(), &uirv);
    if ( prov ) prov->setSelData( new Seis::RangeSelData(tks) );
    if ( !prov || !uirv.isOK() )
	{ msg_ = uirv; return false; }

    const Seis::ObjectSummary seissummary( *ioobj_, tks.getGeomID() );
    RawTrcsSequence* rawseq = new RawTrcsSequence( seissummary, 1 );
    if ( !rawseq ) return false;
    TypeSet<TrcKey>* tkss = new TypeSet<TrcKey>;
    const TrcKey tkstart = tks.trcKeyAt( start );
    *tkss += tkstart; // Only for SurvID
    rawseq->setPositions( *tkss );
    if ( !seissummary.isOK() || !rawseq->isOK() )
	{ delete rawseq; return false; }

    const StepInterval<float>& zsamp = tkzs_.zsamp_;
    const bool samedatachar = seissummary.hasSameFormatAs( dp_->getDataDesc() );
    const bool needresampling = !zsamp.isCompatible( seissummary.zRange() );
    ObjectSet<Scaler> compscalers;
    compscalers.setNullAllowed( true );
    const TypeSet<int>& outcompnrs =
				outcomponents_ && !outcomponents_->isEmpty()
				   ? *outcomponents_ : components_;
    for ( int idx=0; idx<components_.size(); idx++ )
	compscalers += 0;

    ArrayFiller fillertask( *rawseq, zsamp, samedatachar, needresampling,
			    components_, compscalers, outcompnrs, *dp_, true );
    int trcseqpos = 0;
    TrcKey& tk = (*tkss)[trcseqpos];
    for ( int idx=0; idx<trcnrs.size(); idx++ )
    {
	tk.setTrcNr( trcnrs[idx] );
	uirv = prov->getSequence( *rawseq );
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
					      const TrcKeyZSampling* tkzs,
					      const TypeSet<int>* comps )
    : Seis::Loader(ioobj,tkzs,comps)
    , Executor("Volume Reader")
    , prov_(0)
    , initialized_(false)
    , nrdone_(0)
    , trcsiterator2d_(0)
    , trcsiterator3d_(0)
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
    delete trcsiterator2d_;
    delete trcsiterator3d_;
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
    releaseDP();

    return success;
}


bool Seis::SequentialFSLoader::init()
{
    if ( initialized_ )
	return true;

    msg_ = tr("Initializing reader");
    const bool is2d = tkzs_.hsamp_.is2D();
    const Pos::GeomID geomid = is2d ? tkzs_.hsamp_.getGeomID() : mUdfGeomID;
    delete seissummary_;
    seissummary_ = new ObjectSummary( *ioobj_, geomid );
    if ( !seissummary_ || !seissummary_->isOK() )
	{ deleteAndZeroPtr(seissummary_); return false; }

    const SeisIOObjInfo& seisinfo = seissummary_->getFullInformation();
    TrcKeyZSampling seistkzs( tkzs_ );
    seisinfo.getRanges( seistkzs );
    seistkzs.hsamp_.limitTo( tkzs_.hsamp_ );
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
    if ( !dp_ )
    {
	if ( is2d )
	{
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

	dp_ = new RegularSeisDataPack( VolumeDataPack::categoryStr(true,is2d),
					&dc_);
	dp_->setName( ioobj_->name() );
	dp_->setSampling( tkzs_ );
	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( addComponents(*dp_,*ioobj_,components_,msg_) )
	    dpismine_ = true;
	else
	    { releaseDP(); return false; }
    }

    uiRetVal uirv;
    delete prov_;
    prov_ = Seis::Provider::create( ioobj_->key(), &uirv );
    if ( !prov_ || !uirv.isOK() )
    {
	deleteAndZeroPtr( prov_ );
	msg_ = uirv;
	return false;
    }

    prov_->setSelData( new Seis::RangeSelData(seistkzs) );
    setTrcsSamplingFromProv( *prov_ );
    if ( is2d )
    {
	delete trcsiterator2d_;
	trcsiterator2d_ = new PosInfo::Line2DDataIterator( *line2ddata_ );
    }
    else
    {
	delete trcsiterator3d_;
	trcsiterator3d_ = new PosInfo::CubeDataIterator( *trcssampling_ );
    }

    nrdone_ = 0;

    samedatachar_ = seissummary_->hasSameFormatAs( dp_->getDataDesc() );
    dpzsamp_ = dp_->sampling().zsamp_;
    dpzsamp_.limitTo( seissummary_->zRange() );
    needresampling_ = !dpzsamp_.isCompatible( seissummary_->zRange() );

    initialized_ = true;
    msg_ = uiStrings::phrReading( tr("%1 \'%2\'")
			    .arg( uiStrings::sVolDataName(is2d,!is2d,false) )
			    .arg( ioobj_->uiName() ) );
    if ( is2d )
    {
	const BufferString linenm( Survey::GM().getName(geomid) );
	if ( !linenm.isEmpty() )
	    msg_.constructWord( tr("|%1" ).arg( linenm.str() ) );
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

    if ( dp.sampling().isDefined() )
	tkzs_ = dp.sampling();
    else
	dp_->setSampling( tkzs_ );

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
    if ( !init() )
	{ releaseDP(); return ErrorOccurred(); }

    if ( nrdone_ == 0 )
	submitUdfWriterTasks();

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
	releaseDP();
	return ErrorOccurred();
    }

    const uiRetVal uirv = prov_->getSequence( *rawseq );
    if ( !uirv.isOK() )
    {
	delete rawseq;
	if ( isFinished(uirv) )
	    return Finished();

	msg_ = uirv;
	releaseDP();
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

