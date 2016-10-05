/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : SynthSeis
-*/


#include "raysynthgenerator.h"

#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seistrcprop.h"

#define mErrRet(msg, retval) { errmsg_ = msg; return retval; }
#define mpErrRet(msg, retval) { pErrMsg(msg); return retval; }
#define mErrOccRet SequentialTask::ErrorOccurred()

RaySynthGenerator::RaySynthGenerator( const TypeSet<ElasticModel>* ems,
				      bool ownrms )
    : forcerefltimes_(false)
    , rtr_( 0 )
    , raytracingdone_( false )
    , ownraymodels_( ownrms )
    , raymodels_( 0 )
    , aimodels_( ems )
{}


RaySynthGenerator::RaySynthGenerator( ObjectSet<RayModel>* rms )
    : forcerefltimes_(false)
    , rtr_( 0 )
    , raytracingdone_( true )
    , ownraymodels_( false )
    , aimodels_( 0 )
    , raymodels_( rms )
{}


void RaySynthGenerator::reset()
{
    resetNrDone();
    message_ = uiString::emptyString();
}


RaySynthGenerator::~RaySynthGenerator()
{
    delete rtr_;
    if ( ownraymodels_ && raymodels_ )
	deepErase( *raymodels_ );
}


od_int64 RaySynthGenerator::nrIterations() const
{
    return aimodels_ ? aimodels_->size() : raymodels_->size();
}


const ObjectSet<RayTracer1D>& RaySynthGenerator::rayTracers() const
{ return  rtr_->rayTracers(); }


bool RaySynthGenerator::doPrepare( int )
{
    if ( ownraymodels_ && raymodels_ )
	deepErase( *raymodels_ );

    if ( aimodels_ && aimodels_->isEmpty() )
	mErrRet(tr("No AI model found"), false);

    if ( offsets_.isEmpty() )
	offsets_ += 0;

    if ( !raymodels_ )
    {
	rtr_ = new RayTracerRunner( *aimodels_, raysetup_ );
	message_ = tr("Raytracing");
	if ( !rtr_->execute() )
	    mErrRet( rtr_->errMsg(), false );

	raytracingdone_ = true;

	message_ = tr("Preparing Reflectivity Model");
	raymodels_ = new ObjectSet<RayModel>();
	const ObjectSet<RayTracer1D>& rt1ds = rtr_->rayTracers();
	resetNrDone();
	for ( int idx=rt1ds.size()-1; idx>=0; idx-- )
	{
	    const RayTracer1D* rt1d = rt1ds[idx];
	    RayModel* rm = new RayModel( *rt1d, offsets_.size() );
	    raymodels_->insertAt( rm, 0 );

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
    if ( !raymodels_ ) return false;
    IOPar par; fillPar( par );
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() )
	    return false;

	RayModel& rm = *(*raymodels_)[idx];
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


od_int64 RaySynthGenerator::totalNr() const
{
    return !raytracingdone_ && rtr_ ? rtr_->totalNr() : nrIterations();
}


od_int64 RaySynthGenerator::nrDone() const
{
    return !raytracingdone_ && rtr_ ? rtr_->nrDone() : ParallelTask::nrDone();
}



uiString RaySynthGenerator::uiNrDoneText() const
{
    return !raytracingdone_ && rtr_ ? tr("Layers done") : tr("Models done");
}


RaySynthGenerator::RayModel::RayModel( const RayTracer1D& rt1d, int nroffsets )
    : zerooffset2dmodel_(0)
    , refmodels_(new ReflectivityModelSet)
    , sampledrefmodels_(new ReflectivityModelSet)
{
    for ( int idx=0; idx<nroffsets; idx++ )
    {
	ReflectivityModel* refmodel = new ReflectivityModel();
	rt1d.getReflectivity( idx, *refmodel );

	TimeDepthModel* t2dm = new TimeDepthModel();
	rt1d.getTDModel( idx, *t2dm );

	refmodels_->add( refmodel );
	t2dmodels_ += t2dm;
	if ( !idx )
	{
	    zerooffset2dmodel_ = new TimeDepthModel();
	    rt1d.getZeroOffsTDModel( *zerooffset2dmodel_ );
	}
    }
}


RaySynthGenerator::RayModel::~RayModel()
{
    deepErase( outtrcs_ );
    deepErase( t2dmodels_ );
    delete zerooffset2dmodel_;
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


void RaySynthGenerator::getAllRefls( RefMan<ReflectivityModelSet>& refs,
				     bool sampled )
{
    if ( !raymodels_ || raymodels_->isEmpty() ) return;

    refs->setEmpty();
    for ( int imod=0; imod<raymodels_->size(); imod++ )
    {
	if ( !(*raymodels_)[imod] )
	    continue;

	RefMan<ReflectivityModelSet> curraymodel =
			(*raymodels_)[imod]->getRefs( false );
	refs->append( *curraymodel );
    }
}

#define mGet( inpset, outpset, steal )\
{\
    outpset.copy( inpset );\
    if ( steal )\
	inpset.erase();\
}

void RaySynthGenerator::RayModel::getTraces(
		    ObjectSet<SeisTrc>& trcs, bool steal )
{
    mGet( outtrcs_, trcs, steal );
}


RefMan<ReflectivityModelSet>& RaySynthGenerator::RayModel::getRefs(
								bool sampled )
{
    return sampled ? sampledrefmodels_ : refmodels_;
}


void RaySynthGenerator::RayModel::getZeroOffsetD2T( TimeDepthModel& tdms )
{
    tdms = *zerooffset2dmodel_;
}


void RaySynthGenerator::RayModel::getD2T(
			ObjectSet<TimeDepthModel>& tdmodels, bool steal )
{
    mGet( t2dmodels_, tdmodels, steal );
}


void RaySynthGenerator::RayModel::forceReflTimes( const StepInterval<float>& si)
{
    for ( int idx=0; idx<refmodels_->size(); idx++ )
    {
	ReflectivityModel& refmodel =
			const_cast<ReflectivityModel&>(*refmodels_->get(idx));
	for ( int iref=0; iref<refmodel.size(); iref++ )
	{
	    refmodel[iref].time_ = si.atIndex(iref);
	    refmodel[iref].correctedtime_ = si.atIndex(iref);
	}
    }
}


const SeisTrc* RaySynthGenerator::RayModel::stackedTrc() const
{
    if ( outtrcs_.isEmpty() )
	return 0;

    SeisTrc* trc = new SeisTrc( *outtrcs_[0] );
    SeisTrcPropChg stckr( *trc );
    for ( int idx=1; idx<outtrcs_.size(); idx++ )
	stckr.stack( *outtrcs_[idx], false, mCast(float,idx) );

    return trc;
}


void RaySynthGenerator::getTraces( ObjectSet<SeisTrcBuf>& seisbufs )
{
    if ( !raymodels_ || raymodels_->isEmpty() ) return;

    for ( int imdl=0; imdl<raymodels_->size(); imdl++ )
    {
	SeisTrcBuf* tbuf = new SeisTrcBuf( true );
	ObjectSet<SeisTrc> trcs; (*raymodels_)[imdl]->getTraces( trcs, true );
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
    if ( !raymodels_ || raymodels_->isEmpty() ) return;

    seisbuf.erase();
    for ( int imdl=0; imdl<raymodels_->size(); imdl++ )
    {
	SeisTrc* trc = const_cast<SeisTrc*> ((*raymodels_)[imdl]->stackedTrc());
	trc->info().trckey_ = TrcKey::getSynth( imdl + 1 );
	trc->info().coord_ = Coord::udf();
	seisbuf.add( trc );
    }
}
