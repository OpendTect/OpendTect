/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kris / Bruno
 * DATE     : June 2011
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "prestackanglemutecomputer.h"

#include "ailayer.h"
#include "ioman.h"
#include "multiid.h"
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


AngleMuteComputer::~AngleMuteComputer()
{
    delete &outputmute_;
}


bool AngleMuteComputer::doPrepare( int nrthreads )
{
    errmsg_.setEmpty();

    deepErase( rtrunners_ );

    if ( !setVelocityFunction() )
	return false;

    PtrMan<IOObj> muteioobj = IOM().get( params().outputmutemid_ );
    if ( !muteioobj )
	{ errmsg_ = "Cannot find MuteDef ID in Object Manager"; return false; }

    MuteDefTranslator::store( outputmute_, muteioobj, errmsg_ );

    for ( int idx=0; idx<nrthreads; idx++ )
	rtrunners_ += new RayTracerRunner( params().raypar_ );

    return errmsg_.isEmpty();
}


bool AngleMuteComputer::doWork( od_int64 start, od_int64 stop, int thread )
{
    BinID startbid = params().hrg_.atIndex( start );
    BinID stopbid = params().hrg_.atIndex( stop );
    HorSampling hs(false); 
    hs.set( Interval<int>(startbid.inl,stopbid.inl), 
	    Interval<int>(startbid.crl,stopbid.crl) );
    hs.step = params().hrg_.step; 
    HorSamplingIterator iterator( hs );

    ObjectSet<PointBasedMathFunction> mutefuncs;
    TypeSet<BinID> bids;

    RayTracerRunner* rtrunner = rtrunners_[thread];
    BinID curbid = startbid;
    while ( iterator.next( curbid ) && shouldContinue() )
    {
	TypeSet<ElasticLayer> layers; SamplingData<float> sd;
	if ( !getLayers( curbid, layers, sd ) )
	    continue;

	rtrunner->addModel( layers, true );
	if ( !rtrunner->execute( false ) )
	    { errmsg_ = rtrunner->errMsg(); continue; }

	PointBasedMathFunction* mutefunc = new PointBasedMathFunction();

	const int nrlayers = layers.size();
	TypeSet<float> offsets;
	params().raypar_.get( RayTracer1D::sKeyOffset(), offsets );
	float zpos = 0;
	int lastioff =0;
	float lastvalidmutelayer = 0;
	TypeSet< Interval<float> > mutelayeritvs;
	for ( int ioff=0; ioff<offsets.size(); ioff++ )
	{
	    getOffsetMuteLayers( *rtrunner->rayTracers()[0], 
				nrlayers, ioff, true, mutelayeritvs );
	    while ( !mutelayeritvs.isEmpty() )
	    {
		Interval<float> mlitv = mutelayeritvs[0]; 
		mutelayeritvs.remove( 0 );

		float mutelayer = mlitv.start;
		const float offset = offsets[ioff];
		zpos = offset== 0 ? 0 : sd.start + sd.step*mutelayer;
		mutefunc->add( zpos, offset );
		lastvalidmutelayer = mutelayer;
		lastioff = ioff;

		mutelayer = mlitv.stop;
		if ( !mIsUdf( mutelayer ) )
		{
		    zpos = offset == 0 ? 0 : sd.start + sd.step*mutelayer;
		    mutefunc->add( zpos, offset );
		}
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
		rtrunner->rayTracers()[0]->getSinAngle(nrlayers-1,lastioff);
	    const float cosangle = sqrt(1-lastsinangle*lastsinangle);
	    const float doff = thk*lastsinangle/cosangle;
	    mutefunc->add( lastzpos, offsets[lastioff]+doff );
	}

	mutefuncs += mutefunc;
	bids += curbid;
	addToNrDone( 1 );
    }

    //add the mutes
    lock_.lock();

    for ( int idx=0; idx<mutefuncs.size(); idx++ )
	outputmute_.add( mutefuncs[idx], bids[idx] );

    lock_.unLock();

    return true;
}


bool AngleMuteComputer::doFinish( bool sucess )
{
    if ( !sucess ) 
	return false;

    PtrMan<IOObj> obj = IOM().get( params().outputmutemid_ );
    MuteDefTranslator* tr = obj ? (MuteDefTranslator*)obj->getTranslator() : 0;
    BufferString bs;
    return tr ? tr->store( outputmute_, obj, bs ) : false;
}


const char* AngleMuteComputer::errMsg() const
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }


od_int64 AngleMuteComputer::nrIterations() const
{ return params().hrg_.totalNr(); }


AngleMuteComputer::AngleMuteCompPars& AngleMuteComputer::params()
{ return static_cast<AngleMuteCompPars&>(*params_); }


const AngleMuteComputer::AngleMuteCompPars& AngleMuteComputer::params() const
{ return static_cast<AngleMuteComputer::AngleMuteCompPars&>(*params_); }

} //PreStack
