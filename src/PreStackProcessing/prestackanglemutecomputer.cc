/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kris / Bruno
 * DATE     : June 2011
-*/

static const char* rcsID = "$Id: prestackanglemutecomputer.cc,v 1.5 2012-01-06 09:09:18 cvsbruno Exp $";

#include "prestackanglemutecomputer.h"

#include "ailayer.h"
#include "ioman.h"
#include "multiid.h"
#include "prestackgather.h"
#include "prestackmute.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "raytrace1d.h"
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


bool AngleMuteComputer::doPrepare( int )
{
    errmsg_.setEmpty();
    if ( !setVelocityFunction() )
	return false;

    PtrMan<IOObj> muteioobj = IOM().get( params().outputmutemid_ );
    if ( !muteioobj )
	{ errmsg_ = "Cannot find MuteDef ID in Object Manager"; return false; }

    MuteDefTranslator::store(outputmute_,muteioobj,errmsg_);

    offsets_.erase();
    params().raypar_.get( RayTracer1D::sKeyOffset(), offsets_ );
    if ( offsets_.isEmpty() )
	offsets_ += 0;

    return errmsg_.isEmpty();
}


bool AngleMuteComputer::doWork( od_int64 start, od_int64 stop, int )
{
    BinID startbid = params().hrg_.atIndex( start );
    BinID stopbid = params().hrg_.atIndex( stop );
    HorSampling hs(false);
    hs.set( Interval<int>(startbid.inl,stopbid.inl), 
	    Interval<int>(startbid.crl,stopbid.crl) );
    HorSamplingIterator iterator( hs );

    ObjectSet<PointBasedMathFunction> mutefuncs;
    TypeSet<BinID> bids;

    RayTracer1D* raytracer = 
			RayTracer1D::createInstance(params().raypar_,errmsg_);
    if ( !raytracer ) return false;

    BinID curbid;
    while( iterator.next( curbid ) )
    {
	TypeSet<ElasticLayer> layers; SamplingData<float> sd;
	if ( !getLayers( curbid, layers, sd ) )
	    continue;

	raytracer->setModel( layers );
	raytracer->setOffsets( offsets_ );
	if ( !raytracer->execute( false ) )
	    continue;

	PointBasedMathFunction* mutefunc = new PointBasedMathFunction();

	const int nrlayers = layers.size();
	for ( int ioff=0; ioff<offsets_.size(); ioff++ )
	{
	    const float mutelayer = getOffsetMuteLayer( *raytracer, nrlayers, 
		    					ioff, true );
	    if ( !mIsUdf( mutelayer ) )
	    {
		const float zpos = sd.start + sd.step*mutelayer;
		mutefunc->add( offsets_[ioff], zpos );
	    }
	}
	mutefuncs += mutefunc;
	bids += curbid;
	addToNrDone( 1 );
    }
    delete raytracer;

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
