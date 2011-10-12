/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kris / Bruno
 * DATE     : June 2011
-*/

static const char* rcsID = "$Id: prestackanglemutecomputer.cc,v 1.4 2011-10-12 11:32:33 cvsbruno Exp $";

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
    , raytracer_(0)
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
    delete raytracer_;
    delete &outputmute_;
}


bool AngleMuteComputer::doPrepare( int )
{
    if ( !setVelocityFunction() )
	return false;

    PtrMan<IOObj> muteioobj = IOM().get( params().outputmutemid_ );
    if ( !muteioobj )
	{ errmsg_ = "Cannot find MuteDef ID in Object Manager"; return false; }

    MuteDefTranslator::retrieve(outputmute_,muteioobj,errmsg_);

    raytracer_ = RayTracer1D::createInstance( params().raypar_, errmsg_ );
    if ( !raytracer_ ) return false;

    offsets_.erase();
    for ( int idx=0; idx<params().offsetrg_.nrSteps(); idx++ )
	offsets_ += params().offsetrg_.atIndex( idx );

    return errmsg_.isEmpty();
}


bool AngleMuteComputer::doWork( od_int64 start, od_int64 stop, int )
{
    HorSamplingIterator iterator( params().hrg_ );
    BinID curbid = params().hrg_.atIndex( start );

    ObjectSet<PointBasedMathFunction> mutefuncs;
    TypeSet<BinID> bids;

    while( iterator.next( curbid ) )
    {
	TypeSet<ElasticLayer> layers; SamplingData<float> sd;
	if ( !getLayers( curbid, layers, sd ) )
	    continue;

	raytracer_->setModel( layers );
	raytracer_->setOffsets( offsets_ );
	if ( !raytracer_->execute( false ) )
	    continue;

	PointBasedMathFunction* mutefunc = new PointBasedMathFunction();

	const int nrlayers = layers.size();
	for ( int ioff=0; ioff<offsets_.size(); ioff++ )
	{
	    const float mutelayer = getOffsetMuteLayer( *raytracer_, nrlayers, 
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
