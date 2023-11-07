/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackanglemutecomputer.h"

#include "ailayer.h"
#include "ioman.h"
#include "multiid.h"
#include "prestackgather.h"
#include "prestackmute.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "raytracerrunner.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "velocityfunctionvolume.h"


namespace PreStack
{

// PreStack::AngleMuteComputer::AngleMuteCompPars

AngleMuteComputer::AngleMuteCompPars::AngleMuteCompPars()
{
}


AngleMuteComputer::AngleMuteCompPars::~AngleMuteCompPars()
{
}


// PreStack::AngleMuteComputer

AngleMuteComputer::AngleMuteComputer()
    : outputmute_(*new MuteDef)
{
    params_ = new AngleMuteCompPars();
}


AngleMuteComputer::~AngleMuteComputer()
{
    delete &outputmute_;
}


uiString AngleMuteComputer::uiMessage() const
{ return tr("Computing mute"); }


uiString AngleMuteComputer::uiNrDoneText() const
{ return tr("Computed"); }


void AngleMuteComputer::fillPar( IOPar& par ) const
{
    AngleMuteBase::fillPar( par );
    par.set( sKeyMuteDefID(), params().outputmutemid_ );
}


bool AngleMuteComputer::usePar( const IOPar& par )
{
    par.get( sKeyMuteDefID(), params().outputmutemid_ );
    if ( !AngleMuteBase::usePar(par) )
	return false;

    bool offsetsinfeet = outputmute_.isOffsetInFeet();
    if ( params().raypar_.getYN(RayTracer1D::sKeyOffsetInFeet(),offsetsinfeet) )
	outputmute_.setOffsetType( offsetsinfeet ? Seis::OffsetFeet
						 : Seis::OffsetMeter );

    return true;
}


bool AngleMuteComputer::doPrepare( int nrthreads )
{
    errmsg_.setEmpty();

    deepErase( rtrunners_ );

    if ( !setVelocityFunction() )
	return false;

    PtrMan<IOObj> muteioobj = IOM().get( params().outputmutemid_ );
    if ( !muteioobj )
    {
	errmsg_ = tr("Cannot find MuteDef ID in Object Manager");
	return false;
    }

    MuteDefTranslator::store( outputmute_, muteioobj, errmsg_ );

    params().raypar_.get( RayTracer1D::sKeyOffset(), offsets_ );
    bool offsetsinfeet = outputmute_.isOffsetInFeet();
    if ( params().raypar_.getYN(RayTracer1D::sKeyOffsetInFeet(),offsetsinfeet) )
	outputmute_.setOffsetType( offsetsinfeet ? Seis::OffsetFeet
						 : Seis::OffsetMeter );

    for ( int idx=0; idx<nrthreads; idx++ )
	rtrunners_ += new RayTracerRunner( params().raypar_ );

    return errmsg_.isEmpty();
}


bool AngleMuteComputer::doWork( od_int64 start, od_int64 stop, int thread )
{
    if ( !rtrunners_.validIdx(thread) )
	return false;

    RayTracerRunner* rtrunner = rtrunners_[thread];

    const TrcKeySampling& tks = params().tks_;
    ObjectSet<PointBasedMathFunction> mutefuncs;
    TypeSet<BinID> bids;

    const bool zistime = outputmute_.zIsTime();
    const TypeSet<float>& offsets = offsets_;

    const RayTracer1D::Setup& rtsu = params().rtsu_;
    const float cutoffsin = (float) sin( params().mutecutoff_ * M_PIf / 180.f );

    ElasticModelSet emodels;
    auto* layers = new ElasticModel();
    emodels.add( layers );
    bool nonemuted, allmuted;
    for ( od_int64 pidx=start; pidx<=stop && shouldContinue(); pidx++ )
    {
	const TrcKey tk = tks.trcKeyAt( pidx );
	layers->setEmpty();
	if ( !getLayers(tk,*layers,errmsg_) )
	    continue;

	rtrunner->setModel( emodels, &rtsu );
	if ( !rtrunner->executeParallel(false) )
	    { errmsg_ = rtrunner->uiMessage(); continue; }

	ConstRefMan<ReflectivityModelSet> refmodels = rtrunner->getRefModels();
	const ReflectivityModelBase* refmodel = refmodels ? refmodels->get(0)
							  : nullptr;
	if ( !refmodel || !refmodel->hasAngles() )
	    return false;

	const TimeDepthModel& tdmodel = refmodel->getDefaultModel();
	auto* mutefunc = new PointBasedMathFunction();

	const bool innermute = false;
	float zpos = 0.f;
	int lastioff = 0;
	for ( int ioff=0; ioff<offsets.size(); ioff++ )
	{
	    const float offset = offsets[ioff];
	    TypeSet< Interval<float> > mutelayeritvs;
	    const float mutelayer = getOffsetMuteLayer( *refmodel, ioff,
						innermute, nonemuted,
						allmuted, mutelayeritvs );
	    if ( !mIsUdf(mutelayer) )
	    {
		zpos = getfMutePos( tdmodel, zistime, mutelayer, offset );
		mutefunc->add( offset, zpos );
		lastioff = ioff;
		continue;
	    }

	    for ( auto& itvml : mutelayeritvs )
	    {
		if ( mIsUdf(itvml.start) )
		    continue;

		zpos = getfMutePos( tdmodel, zistime, itvml.start, offset );
		mutefunc->add( offset, zpos );
		lastioff = ioff;
		break;
/* These points should be added too to the function:
		if ( mIsUdf(itvml.stop) )
		    continue;

		zpos = getfMutePos( tdmodel, zistime, itvml.stop, offset );
		mutefunc->add( offset, zpos ); //Should be added too
 */
	    }
	}

	const int nrlayers = layers->size();
	if ( lastioff != offsets.size()-1 )
	{
	    zpos = zistime ? tdmodel.getTime( nrlayers )
			   : tdmodel.getDepth( nrlayers );
	    const float prevangle = refmodel->getSinAngle( lastioff,
							   nrlayers-1 );
	    const float nextangle = refmodel->getSinAngle( lastioff+1,
							   nrlayers-1 );
	    const float relpos = ( cutoffsin - prevangle ) /
				 ( nextangle - prevangle );
	    if ( !mIsUdf(prevangle) && !mIsUdf(nextangle) )
	    {
		const float doff = mIsEqual(nextangle,prevangle,1e-6f)
				 ? 0.f
			 : ( (offsets[lastioff+1] - offsets[lastioff])*relpos);
		mutefunc->add( offsets[lastioff]+doff, zpos );
	    }
	}

	mutefuncs += mutefunc;
	bids += tk.position();
	addToNrDone( 1 );
    }

    Threads::Locker lckr( lock_ );
    //add the mutes
    for ( int idx=0; idx<mutefuncs.size(); idx++ )
	outputmute_.add( mutefuncs[idx], bids[idx] );

    return true;
}


bool AngleMuteComputer::doFinish( bool sucess )
{
    if ( !sucess )
	return false;

    PtrMan<IOObj> obj = IOM().get( params().outputmutemid_ );
    PtrMan<MuteDefTranslator> mdtrl = obj
	? (MuteDefTranslator*)obj->createTranslator()
	: nullptr;

    uiString msg;
    return mdtrl ? mdtrl->store( outputmute_, obj, msg ) : false;
}


od_int64 AngleMuteComputer::nrIterations() const
{ return params().tks_.totalNr(); }


AngleMuteComputer::AngleMuteCompPars& AngleMuteComputer::params()
{ return static_cast<AngleMuteCompPars&>(*params_); }


const AngleMuteComputer::AngleMuteCompPars& AngleMuteComputer::params() const
{ return static_cast<AngleMuteComputer::AngleMuteCompPars&>(*params_); }

} // namespace PreStack
