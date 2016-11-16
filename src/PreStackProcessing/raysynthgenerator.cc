/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/


#include "raysynthgenerator.h"

#include "prestackgather.h"
#include "prestacksyntheticdata.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "stratsynthgenparams.h"
#include "syntheticdataimpl.h"

#define mErrRet(msg, retval) { errmsg_ = msg; return retval; }
#define mpErrRet(msg, retval) { pErrMsg(msg); return retval; }
#define mErrOccRet SequentialTask::ErrorOccurred()

RaySynthGenerator::RaySynthGenerator( const TypeSet<ElasticModel>* ems,
				      const SynthGenParams& synthpars )
    : forcerefltimes_(false)
    , rtr_( 0 )
    , raytracingdone_( false )
    , aimodels_( ems )
    , overwrite_(true)
{
    if ( synthpars.synthtype_ == SynthGenParams::ZeroOffset )
    {
	SeisTrcBuf* dptrcbuf = new SeisTrcBuf( true );
	SeisTrcBufDataPack* trcbufdp =
		new SeisTrcBufDataPack( dptrcbuf, Seis::Line,
					SeisTrcInfo::TrcNr, synthpars.name_ );
	synthdata_ = new PostStackSyntheticData( synthpars, *trcbufdp);
    }
    else
    {
	ObjectSet<PreStack::Gather> gatherset;
	PreStack::GatherSetDataPack* dp =
		new PreStack::GatherSetDataPack( synthpars.name_, gatherset );
	synthdata_ = new PreStack::PreStackSyntheticData( synthpars, *dp );
    }
}


RaySynthGenerator::RaySynthGenerator( SyntheticData* sd, bool overwrite )
    : forcerefltimes_(false)
    , rtr_( 0 )
    , raytracingdone_( true )
    , aimodels_( 0 )
    , overwrite_( overwrite )
    , synthdata_( sd )
{
}


void RaySynthGenerator::reset()
{
    resetNrDone();
    message_ = uiString::emptyString();
}


RaySynthGenerator::~RaySynthGenerator()
{
    delete rtr_;
}


od_int64 RaySynthGenerator::nrIterations() const
{
    return aimodels_ ? aimodels_->size() : synthdata_->raymodels_->size();
}


const ObjectSet<RayTracer1D>& RaySynthGenerator::rayTracers() const
{ return  rtr_->rayTracers(); }


bool RaySynthGenerator::doPrepare( int )
{
    if ( aimodels_ && aimodels_->isEmpty() )
	mErrRet(tr("No AI model found"), false);

    if ( offsets_.isEmpty() )
	offsets_ += 0;

    if ( synthdata_->raymodels_->isEmpty() )
    {
	rtr_ = new RayTracerRunner( *aimodels_, raysetup_ );
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
	    SyntheticData::RayModel* rm =
			new SyntheticData::RayModel( *rt1d,offsets_.size() );
	    synthdata_->raymodels_->insertAt( rm, 0 );

	    if ( forcerefltimes_ )
		rm->forceReflTimes( forcedrefltimes_ );
	    addToNrDone( 1 );
	}
    }

    resetNrDone();
    RefMan<ReflectivityModelSet> models = new ReflectivityModelSet;
    getAllRefls( models );
    const bool zerooffset = offsets_.size() == 1 &&
			    Seis::equalOffset(offsets_[0],0);
    StepInterval<float> cursampling( outputsampling_ );
    if ( models->isEmpty() )
	mErrRet( tr("No models given to make synthetics"), false );

    if ( !Seis::SynthGenBase::isInputOK() )
	return mErrOccRet;

    if ( !SynthGenBase::getOutSamplingFromModel(models,cursampling,
						applynmo_||zerooffset) &&
	    aimodels_ )
    {
	Interval<float> modelsampling;
	ElasticModel::getTimeSampling( *aimodels_, modelsampling );
	cursampling.include( modelsampling, false );
    }

    outputsampling_.include( cursampling, false );
    outputsampling_.step = cursampling.step;

    message_ = tr("Generating synthetics");

    return true;
}


bool RaySynthGenerator::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !synthdata_->raymodels_ ) return false;
    IOPar par; fillPar( par );
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() )
	    return false;

	SyntheticData::RayModel& rm = *(*synthdata_->raymodels_)[idx];
	deepErase( rm.outtrcs_ );

	Seis::MultiTraceSynthGenerator multitracegen;
	multitracegen.setModels( rm.refmodels_ );
	multitracegen.setOutSampling( outputsampling_ );
	multitracegen.usePar( par );
	if ( wavelet_ )
	    multitracegen.setWavelet( wavelet_ );

	if ( !multitracegen.execute() )
	    mErrRet( multitracegen.errMsg(), false )

	multitracegen.getResult( rm.outtrcs_ );
	multitracegen.getSampledRMs( rm.sampledrefmodels_ );
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
    const bool zerooffset = offsets_.size() == 1 &&
			    Seis::equalOffset(offsets_[0],0);
    DataPack::ID id = synthdata_->getPack().id();
    if ( zerooffset )
    {
	SeisTrcBuf* seisbuf = new SeisTrcBuf( true );
	getStackedTraces( *seisbuf );

	RefMan<DataPack> dp = DPM( synthdata_->datapackid_ ).get(id);
	mDynamicCastGet(SeisTrcBufDataPack*,postdp,dp.ptr());
	postdp->setBuffer( seisbuf, Seis::Line, SeisTrcInfo::TrcNr,0,true);
	return true;
    }

    ObjectSet<SeisTrcBuf> tbufs;
    getTraces( tbufs );
    RefObjectSet<PreStack::Gather> gatherset;
    while ( tbufs.size() )
    {
	PtrMan<SeisTrcBuf> tbuf = tbufs.removeSingle( 0 );
	RefMan<PreStack::Gather> gather = new PreStack::Gather();
	if ( !gather->setFromTrcBuf( *tbuf, 0 ) )
	    continue;

	gather->ref();
	gatherset += gather;
    }

    RefMan<DataPack> dp = DPM( synthdata_->datapackid_ ).get(id);
    mDynamicCastGet(PreStack::GatherSetDataPack*,prestkdp,dp.ptr());
    prestkdp->setGathers( gatherset );
    deepUnRef( gatherset );
    return true;
}


bool RaySynthGenerator::doFinish( bool success )
{
    if (!synthdata_->raymodels_ || synthdata_->raymodels_->isEmpty() )
	return false;


    synthdata_->updateD2TModels();
    getAllRefls( synthdata_->reflectivitymodels_ );

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
    SynthGenBase::usePar( par );
    raysetup_.merge( par );
    raysetup_.get( RayTracer1D::sKeyOffset(), offsets_ );
    if ( offsets_.isEmpty() )
	offsets_ += 0;

    return true;
}


void RaySynthGenerator::fillPar( IOPar& par ) const
{
    SynthGenBase::fillPar( par );
    par.merge( raysetup_ );
}


void RaySynthGenerator::forceReflTimes( const StepInterval<float>& si)
{
    forcedrefltimes_ = si; forcerefltimes_ = true;
}


void RaySynthGenerator::getAllRefls( RefMan<ReflectivityModelSet>& refs )
{
    if ( !synthdata_->raymodels_ || synthdata_->raymodels_->isEmpty() ) return;

    refs->setEmpty();
    for ( int imod=0; imod<synthdata_->raymodels_->size(); imod++ )
    {
	if ( !(*synthdata_->raymodels_)[imod] )
	    continue;

	RefMan<ReflectivityModelSet> curraymodel =
			(*synthdata_->raymodels_)[imod]->getRefs( false );
	refs->append( *curraymodel );
    }
}


void RaySynthGenerator::getTraces( ObjectSet<SeisTrcBuf>& seisbufs )
{
    if ( !synthdata_->raymodels_ || synthdata_->raymodels_->isEmpty() ) return;

    for ( int imdl=0; imdl<synthdata_->raymodels_->size(); imdl++ )
    {
	SeisTrcBuf* tbuf = new SeisTrcBuf( true );
	ObjectSet<SeisTrc> trcs;
	(*synthdata_->raymodels_)[imdl]->getTraces(trcs, true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = trcs[idx];
	    trc->info().trckey_ = TrcKey::getSynth( imdl + 1 );
	    trc->info().coord_ = Coord::udf();
	    tbuf->add( trc );
	}
	seisbufs += tbuf;
    }
}


void RaySynthGenerator::getStackedTraces( SeisTrcBuf& seisbuf )
{
    if ( !synthdata_->raymodels_ || synthdata_->raymodels_->isEmpty() ) return;

    seisbuf.erase();
    for ( int imdl=0; imdl<synthdata_->raymodels_->size(); imdl++ )
    {
	SeisTrc* trc =
	   const_cast<SeisTrc*> ((*synthdata_->raymodels_)[imdl]->stackedTrc());
	if ( !trc )
	    continue;

	trc->info().trckey_ = TrcKey::getSynth( imdl + 1 );
	trc->info().coord_ = Coord::udf();
	seisbuf.add( trc );
    }
}
