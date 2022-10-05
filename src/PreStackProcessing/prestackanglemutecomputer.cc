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


// PreStack::AngleMuteComputer::

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


void AngleMuteComputer::fillPar( IOPar& par ) const
{
    AngleMuteBase::fillPar( par );
    par.set( sKeyMuteDefID(), params().outputmutemid_ );
}


bool AngleMuteComputer::usePar( const IOPar& par )
{
    par.get( sKeyMuteDefID(), params().outputmutemid_ );
    return AngleMuteBase::usePar( par );
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

    for ( int idx=0; idx<nrthreads; idx++ )
	rtrunners_ += new RayTracerRunner( params().raypar_ );

    return errmsg_.isEmpty();
}


bool AngleMuteComputer::doWork( od_int64 start, od_int64 stop, int thread )
{
    const TrcKeySampling& hrg = params().tks_;
    ObjectSet<PointBasedMathFunction> mutefuncs;
    TypeSet<BinID> bids;

    RayTracerRunner* rtrunner = rtrunners_[thread];
    TypeSet<ElasticModel> emodels;
    ElasticModel layers;
    emodels += layers;
    rtrunner->setModel( emodels );

    BinID curbid;
    for ( od_int64 pidx=start; pidx<=stop && shouldContinue(); pidx++ )
    {
	curbid = hrg.atIndex( pidx );
	SamplingData<float> sd;
	if ( !getLayers(curbid,layers,sd) )
	    continue;

	if ( !rtrunner->executeParallel(false) )
	    { errmsg_ = rtrunner->uiMessage(); continue; }

	ConstRefMan<ReflectivityModelSet> refmodels = rtrunner->getRefModels();
	const ReflectivityModelBase* refmodel = refmodels ? refmodels->get(0)
							  : nullptr;
	if ( !refmodel || !refmodel->hasAngles() )
	    return false;

	auto* mutefunc = new PointBasedMathFunction();

	const int nrlayers = layers.size();
	TypeSet<float> offsets;
	params().raypar_.get( RayTracer1D::sKeyOffset(), offsets );
	float zpos = 0;
	int lastioff =0;
	float lastvalidmutelayer = 0;
	for ( int ioff=0; ioff<offsets.size(); ioff++ )
	{
	    const float mutelayer =
			getOffsetMuteLayer( *refmodel, nrlayers, ioff, true );
	    if ( !mIsUdf( mutelayer ) )
	    {
		zpos = offsets[ioff] == 0 ? 0 : sd.start + sd.step*mutelayer;
		lastvalidmutelayer = mutelayer;
		mutefunc->add( offsets[ioff], zpos );
		lastioff = ioff;
	    }
	}

	if ( lastioff != offsets.size()-1 )
	{
	    float zdpt = 0;
	    for ( int idx=0; idx<(int)lastvalidmutelayer+1 ; idx++ )
	    {
		if ( idx < lastvalidmutelayer+1 )
		    zdpt += layers[idx].thickness_;
	    }
	    float lastdepth = 0;
	    for ( int idx=0; idx<nrlayers; idx++ )
		lastdepth += layers[idx].thickness_;

	    float thk = lastdepth - zdpt;
	    const float lastzpos = sd.start + sd.step*(nrlayers-1);
	    const float lastsinangle =
				refmodel->getSinAngle( lastioff, nrlayers-1 );

	    const float cosangle = Math::Sqrt(1-lastsinangle*lastsinangle);
	    if ( cosangle > 0 )
	    {
		const float doff = thk*lastsinangle/cosangle;
		mutefunc->add( lastzpos, offsets[lastioff]+doff );
	    }
	}

	mutefuncs += mutefunc;
	bids += curbid;
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
