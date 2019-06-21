/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kris / Bruno
 * DATE     : June 2011
-*/


#include "prestackanglemutecomputer.h"

#include "dbkey.h"
#include "elasticmodel.h"
#include "prestackgather.h"
#include "prestackmute.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "raytrace1d.h"
#include "raytracerrunner.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "velocityfunctionvolume.h"


namespace PreStack
{

AngleMuteComputer::AngleMuteComputer()
    : outputmute_(*new MuteDef)
{
    params_ = new AngleMuteCompPars();
    msg_ = tr("Calculating mute");
}


AngleMuteComputer::~AngleMuteComputer()
{
    delete &outputmute_;
}


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
    if ( !setVelocityFunction() )
	return false;

    PtrMan<IOObj> muteioobj = params().outputmutemid_.getIOObj();
    if ( !muteioobj )
    {
	msg_ = tr("Cannot find MuteDef ID in Object Manager");
	return false;
    }

    MuteDefTranslator::store( outputmute_, muteioobj, msg_ );
    if ( !msg_.isEmpty() )
	return false;

    for ( int idx=0; idx<nrthreads; idx++ )
	rtrunners_ += new RayTracerRunner( params().raypar_ );

    return true;
}


bool AngleMuteComputer::doWork( od_int64 start, od_int64 stop, int thread )
{
    if ( !rtrunners_.validIdx(thread) )
	return false;

    const TrcKeySampling& hrg = params().tks_;
    ObjectSet<PointBasedMathFunction> mutefuncs;
    TypeSet<BinID> bids;

    RayTracerRunner& rtrunner = *rtrunners_[thread];
    rtrunner.setParallel( false );
    ElasticModelSet models; const int modelidx = 0;
    models += new ElasticModel;
    ElasticModel& layers = *models.get( modelidx );
    rtrunner.setModel( models );
    BinID curbid;
    for ( od_int64 pidx=start; pidx<=stop && shouldContinue(); pidx++ )
    {
	curbid = hrg.atIndex( pidx );
	SamplingData<float> sd;
	layers.setEmpty();
	if ( !getLayers(curbid,layers,sd) )
	    continue;

	if ( !rtrunner.execute() )
	    { msg_ = rtrunner.errMsg(); continue; }

	PointBasedMathFunction* mutefunc = new PointBasedMathFunction();

	const int nrlayers = layers.size();
	TypeSet<float> offsets;
	params().raypar_.get( RayTracer1D::sKeyOffset(), offsets );
	float zpos = 0;
	int lastioff =0;
	float lastvalidmutelayer = 0;
	for ( int ioff=0; ioff<offsets.size(); ioff++ )
	{
	    ConstRefMan<RayTracerData> raytracedata =
				       rtrunner.results().get(modelidx);
	    const float mutelayer =
		getOffsetMuteLayer( *raytracedata.ptr(), nrlayers, ioff, true );
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
		    zdpt += layers.get(idx).thickness_;
	    }
	    float lastdepth = 0;
	    for ( int idx=0; idx<nrlayers; idx++ )
		lastdepth += layers.get(idx).thickness_;

	    float thk = lastdepth - zdpt;
	    const float lastzpos = sd.start + sd.step*(nrlayers-1);
	    ConstRefMan<RayTracerData> raytracedata =
					    rtrunner.results().get( modelidx );
	    const float lastsinangle =
				raytracedata->getSinAngle(nrlayers-1,lastioff);

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
    deepErase( rtrunners_ );
    if ( !sucess )
	return false;

    PtrMan<IOObj> obj = params().outputmutemid_.getIOObj();
    PtrMan<MuteDefTranslator> mdtrl = obj
	? (MuteDefTranslator*)obj->createTranslator()
	: 0;

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
