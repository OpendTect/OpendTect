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
    if ( elasticmodels_ && elasticmodels_->isEmpty() )
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
    RflMdlSetRef allmdls = new ReflectivityModelSet;
    const auto& raymdls = dataset_->rayMdls();
    if ( raymdls.isEmpty() || raymdls.first()->reflModels()->isEmpty() )
	mErrRet( tr("No models given to make synthetics"), false );
    *allmdls = *raymdls.first()->reflModels();
    if ( raymdls.size() > 1 )
	allmdls->append( *raymdls.last()->reflModels() );

    if ( !isInputOK() )
	return mErrOccRet;

    if ( !getOutSamplingFromModels(allmdls,cursampling,applynmo_||zerooffset)
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
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() )
	    return false;

	auto& rm = *dataset_->rayMdls().get( idx );
	deepErase( rm.outtrcs_ );

	SynthSeis::MultiTraceGenerator multitracegen;
	multitracegen.setModels( rm.reflmodels_ );
	multitracegen.setOutSampling( outputsampling_ );
	multitracegen.usePar( par );
	if ( wavelet_ )
	    multitracegen.setWavelet( wavelet_ );

	if ( !multitracegen.execute() )
	    mErrRet( multitracegen.errMsg(), false )

	multitracegen.getResult( rm.outtrcs_ );
	multitracegen.getSampledRMs( rm.sampledreflmodels_ );
	for ( int idoff=0; idoff<offsets_.size(); idoff++ )
	{
	    if ( !rm.outtrcs_.validIdx( idoff ) )
	    {
		rm.outtrcs_ += new SeisTrc( outputsampling_.nrSteps() + 1 );
		rm.outtrcs_[idoff]->info().sampling_ = outputsampling_;
		for ( int idz=0; idz<rm.outtrcs_[idoff]->size(); idz++ )
		    rm.outtrcs_[idoff]->set( idz, mUdf(float), 0 );
	    }
	    rm.outtrcs_[idoff]->info().offset_ = offsets_[idoff];
	    rm.outtrcs_[idoff]->info().trckey_ = TrcKey::getSynth( idx + 1 );
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
	RefMan<Gather> gather = new Gather();
	if ( !gather->setFromTrcBuf( *tbuf, 0 ) )
	    continue;

	gather->ref();
	gatherset += gather;
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


od_int64 RaySynthGenerator::totalNr() const
{
    return !raytracingdone_ && rtr_ ? rtr_->totalNr() : nrIterations();
}


od_int64 RaySynthGenerator::nrDone() const
{
    return !raytracingdone_ && rtr_ ? rtr_->nrDone() : ParallelTask::nrDone();
}



uiString RaySynthGenerator::nrDoneText() const
{
    return !raytracingdone_ && rtr_ ? tr("Layers done") : tr("Models done");
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


void RaySynthGenerator::forceReflTimes( const StepInterval<float>& si)
{
    forcedrefltimes_ = si; forcerefltimes_ = true;
}


static void setTrcInfo( SeisTrc* trc, int imdl, int nrstep )
{
    trc->info().trckey_ = TrcKey::getSynth( imdl*nrstep + 1 );
    trc->info().coord_ = Coord::udf();
}


void RaySynthGenerator::getTraces( ObjectSet<SeisTrcBuf>& tbufs )
{
    if ( dataset_->rayModels().isEmpty() )
	return;

    for ( int imdl=0; imdl<dataset_->rayModels().size(); imdl++ )
    {
	SeisTrcBuf* tbuf = new SeisTrcBuf( true );
	ObjectSet<SeisTrc> trcs;
	dataset_->rayMdls()[imdl]->getTraces( trcs, true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = trcs[idx];
	    setTrcInfo( trc, imdl, nrstep_ );
	    tbuf->add( trc );
	}
	tbufs += tbuf;
    }
}


void RaySynthGenerator::getStackedTraces( SeisTrcBuf& tbuf )
{
    if ( dataset_->rayModels().isEmpty() )
	return;

    tbuf.erase();
    for ( int imdl=0; imdl<dataset_->rayModels().size(); imdl++ )
    {
	auto* trc = dataset_->rayModels().get(imdl)->stackedTrc();
	if ( !trc )
	    trc = new SeisTrc( outputsampling_.nrSteps()+1 );
	setTrcInfo( trc, imdl, nrstep_ );
	tbuf.add( trc );
    }
}
