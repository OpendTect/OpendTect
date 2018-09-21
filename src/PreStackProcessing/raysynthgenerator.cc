/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/


#include "raysynthgenerator.h"

#include "prestackgather.h"
#include "prestacksynthdataset.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "synthseisdataset.h"

#define mErrRet(msg, retval) { errmsg_ = msg; return retval; }
#define mpErrRet(msg, retval) { pErrMsg(msg); return retval; }
#define mErrOccRet SequentialTask::ErrorOccurred()

RaySynthGenerator::RaySynthGenerator( const GenParams& gp,
					const ElasticModelSet& ems )
    : forcerefltimes_(false)
    , rtr_( 0 )
    , raytracingdone_( false )
    , elasticmodels_( &ems )
{
    createDataSet( gp );
}


RaySynthGenerator::RaySynthGenerator( const SynthSeis::GenParams& gp,
					const RayModelSet& rms )
    : forcerefltimes_(false)
    , rtr_( 0 )
    , raytracingdone_( true )
{
    createDataSet( gp );
    dataset_->setRayModels( rms );
}


void RaySynthGenerator::createDataSet( const GenParams& gp )
{
    if ( gp.type_ == SynthSeis::ZeroOffset )
    {
	auto* dp = new SeisTrcBufDataPack( "PostStack Synthetics" );
	dataset_ = new SynthSeis::PostStackDataSet( gp, *dp );
    }
    else
    {
	auto* dp = new GatherSetDataPack( "PreStack Synthetics" );
	dataset_ = new SynthSeis::PreStackDataSet( gp, *dp );
    }
    dataset_->setName( gp.name_ );
    dataset_->ref();
}


void RaySynthGenerator::reset()
{
    resetNrDone();
    message_ = uiString::empty();
}


RaySynthGenerator::~RaySynthGenerator()
{
    delete rtr_;
    dataset_->unRef();
}


const ElasticModelSet& RaySynthGenerator::elasticModels() const
{
    static ElasticModelSet emptyelms;
    return elasticmodels_ ? *elasticmodels_ : emptyelms;
}


od_int64 RaySynthGenerator::nrIterations() const
{
    return elasticmodels_ ? elasticmodels_->size()
			  : dataset_->rayModels().size();
}


const ObjectSet<RayTracer1D>& RaySynthGenerator::rayTracers() const
{ return  rtr_->rayTracers(); }


bool RaySynthGenerator::doPrepare( int )
{
    if ( elasticModels().isEmpty() )
	mErrRet(tr("No Elastic Model available"), false);

    if ( offsets_.isEmpty() )
	offsets_ += 0;

    if ( !raytracingdone_ || dataset_->rayModels().isEmpty() )
    {
	rtr_ = new RayTracerRunner( elasticModels(), raysetup_ );
	message_ = tr("Raytracing");
	if ( !rtr_->execute() )
	    mErrRet( rtr_->errMsg(), false );

	raytracingdone_ = true;

	message_ = tr("Preparing Reflectivity Model");
	const ObjectSet<RayTracer1D>& rt1ds = rtr_->rayTracers();
	resetNrDone();
	for ( int idx=rt1ds.size()-1; idx>=0; idx-- )
	{
	    const RayTracer1D* rt1d = rt1ds[idx];
	    auto* rm = new SynthSeis::RayModel( *rt1d, offsets_.size() );
	    dataset_->rayMdls().insertAt( rm, 0 );

	    if ( forcerefltimes_ )
		rm->forceReflTimes( forcedrefltimes_ );
	    addToNrDone( 1 );
	}
    }

    resetNrDone();
    const bool zerooffset = offsets_.size() == 1 &&
			    Seis::equalOffset(offsets_[0],0);
    StepInterval<float> cursampling( outputsampling_ );
    RefMan<ReflectivityModelSet> allmdls = new ReflectivityModelSet;
    const auto& raymdls = dataset_->rayMdls();
    if ( raymdls.isEmpty() || raymdls.first()->reflModels()->isEmpty() )
	mErrRet( tr("No models given to make synthetics"), false );
    *allmdls = *raymdls.first()->reflModels();
    if ( raymdls.size() > 1 )
	allmdls->append( *raymdls.last()->reflModels() );

    if ( !isInputOK() )
	return mErrOccRet;

    if ( !getOutSamplingFromModels(*allmdls,cursampling,applynmo_||zerooffset)
	    && elasticmodels_ )
	cursampling.include( elasticmodels_->getTimeRange(), false );

    outputsampling_.include( cursampling, false );
    outputsampling_.step = cursampling.step;

    message_ = tr("Generating synthetics");

    return true;
}


bool RaySynthGenerator::doWork( od_int64 start, od_int64 stop, int )
{
    if ( dataset_->rayModels().isEmpty() )
	return false;

    IOPar par; fillPar( par );
    const auto nrsamples = outputsampling_.nrSteps() + 1;
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() )
	    return false;

	auto& raymod = *dataset_->rayMdls().get( idx );
	auto& outtrcs = raymod.outtrcs_;
	deepErase( outtrcs );

	SynthSeis::MultiTraceGenerator multitracegen;
	multitracegen.setModels( *raymod.reflmodels_ );
	multitracegen.setOutSampling( outputsampling_ );
	multitracegen.usePar( par );
	if ( wavelet_ )
	    multitracegen.setWavelet( wavelet_ );

	if ( !multitracegen.execute() )
	    mErrRet( multitracegen.errMsg(), false )

	multitracegen.getResult( outtrcs );
	raymod.sampledreflmodels_ = multitracegen.getSampledRMs();
	for ( int ioffs=0; ioffs<offsets_.size(); ioffs++ )
	{
	    SeisTrc* outtrc = 0;
	    if ( ioffs < outtrcs.size() )
		outtrc = outtrcs[ioffs];
	    else
	    {
		pErrMsg( "Got no out trc. Probably not OK?" );
		outtrc = new SeisTrc( nrsamples );
		outtrcs += outtrc;
		outtrc->info().sampling_ = outputsampling_;
		outtrc->setAll( mUdf(float), 0 );
	    }
	    outtrc->info().offset_ = offsets_[ioffs];
	    outtrc->info().trckey_ = TrcKey::getSynth( idx + 1 );
	}
    }

    return true;
}


bool RaySynthGenerator::updateDataPack()
{
    DataPack& dp = dataset_->dataPack();
    if ( !dataset_->isPS() )
    {
	mDynamicCastGet( SeisTrcBufDataPack*, tbdp, &dp );
	auto* tbuf = new SeisTrcBuf( true );
	getStackedTraces( *tbuf );
	tbdp->setBuffer( tbuf, Seis::Line, SeisTrcInfo::TrcNr, 0, true );
	return true;
    }

    ObjectSet<SeisTrcBuf> tbufs;
    getTraces( tbufs );
    RefObjectSet<Gather> gatherset;
    while ( tbufs.size() )
    {
	PtrMan<SeisTrcBuf> tbuf = tbufs.removeSingle( 0 );
	auto* gather = new Gather();
	if ( !gather->setFromTrcBuf( *tbuf, 0 ) )
	    { pErrMsg("Cannot set gather from trcbuf?"); }
	else
	{
	    gather->ref();
	    gatherset += gather;
	}
    }

    mDynamicCastGet( GatherSetDataPack*, prestkdp, &dp );
    prestkdp->setGathers( gatherset );
    deepUnRef( gatherset );
    return true;
}


bool RaySynthGenerator::doFinish( bool success )
{
    if ( dataset_->rayModels().isEmpty() )
	return false;

    dataset_->updateD2TModels();
    return updateDataPack();
}


#define mDoingRayTracing() (!raytracingdone_ && rtr_)

od_int64 RaySynthGenerator::totalNr() const
{
    return mDoingRayTracing() ? rtr_->totalNr() : nrIterations();
}


od_int64 RaySynthGenerator::nrDone() const
{
    return mDoingRayTracing() ? rtr_->nrDone() : ParallelTask::nrDone();
}



uiString RaySynthGenerator::nrDoneText() const
{
    return mDoingRayTracing() ? tr("Layers done") : tr("Models done");
}


bool RaySynthGenerator::usePar( const IOPar& par )
{
    GenBase::usePar( par );
    raysetup_.merge( par );
    raysetup_.get( RayTracer1D::sKeyOffset(), offsets_ );
    if ( offsets_.isEmpty() )
	offsets_ += 0;

    return true;
}


void RaySynthGenerator::fillPar( IOPar& par ) const
{
    GenBase::fillPar( par );
    par.merge( raysetup_ );
}


void RaySynthGenerator::forceReflTimes( const StepInterval<float>& si )
{
    forcedrefltimes_ = si; forcerefltimes_ = true;
}


static void setTrcInfo2Synth( SeisTrc* trc, int imdl, int nrstep )
{
    trc->info().trckey_ = TrcKey::getSynth( imdl*nrstep + 1 );
    trc->info().coord_ = Coord::udf();
}


void RaySynthGenerator::getTraces( ObjectSet<SeisTrcBuf>& tbufs )
{
    if ( dataset_->rayModels().isEmpty() )
	return;

    auto& raymdls = dataset_->rayMdls();
    for ( int imdl=0; imdl<raymdls.size(); imdl++ )
    {
	SeisTrcBuf* tbuf = new SeisTrcBuf( true );
	ObjectSet<SeisTrc> trcs;
	raymdls[imdl]->getTraces( trcs, true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = trcs[idx];
	    setTrcInfo2Synth( trc, imdl, nrstep_ );
	    tbuf->add( trc );
	}
	tbufs += tbuf;
    }
}


void RaySynthGenerator::getStackedTraces( SeisTrcBuf& tbuf )
{
    const auto& raymdls = dataset_->rayModels();
    if ( raymdls.isEmpty() )
	{ pErrMsg("Empty ray models"); return; }

    tbuf.erase();
    for ( int imdl=0; imdl<raymdls.size(); imdl++ )
    {
	auto* trc = raymdls.get(imdl)->stackedTrc();
	if ( !trc )
	    trc = new SeisTrc( outputsampling_.nrSteps()+1 );
	setTrcInfo2Synth( trc, imdl, nrstep_ );
	tbuf.add( trc );
    }
}
