/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2007
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "prestackeventtracker.h"

#include "ioman.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"

namespace PreStack
{


EventTracker::EventTracker()
    : ownsinnermute_( false )
    , ownsoutermute_( false )
    , innermute_( 0 )
    , outermute_( 0 )
{}


EventTracker::~EventTracker()
{
    removeMutes();
}


void EventTracker::reInit()
{ removeMutes(); }


void EventTracker::setMute( bool inner, MuteDef* mutedef,
			    OD::PtrPolicy ptrpolicy )
{
    MuteDef*& mymute = inner ? innermute_ : outermute_;
    bool& myownership = inner ? ownsinnermute_ : ownsoutermute_;

    if ( mymute && myownership )
	delete mymute;

    mymute = 0;
    if ( !mutedef )
	return;

    if ( ptrpolicy==OD::UsePtr )
    {
	mymute = mutedef;
	myownership = false;
    }
    else if ( ptrpolicy==OD::CopyPtr )
    {
	mymute = new MuteDef(*mutedef);
	myownership = true;
    }
    else
    {
	mymute = mutedef;
	myownership = true;
    }
}

#define mErrRet( msg ) { errmsg_ = msg; return false; }


bool EventTracker::setMute( bool inner, const MultiID& mid )
{
    MultiID& myid = inner ? innermuteid_ : outermuteid_;
    myid.setEmpty();

    PreStack::MuteDef* mutedef = 0;

    if ( !mid.isEmpty() )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	    mErrRet("Cannot find MuteDef ID in Object Manager");
	mutedef = new PreStack::MuteDef;

	if ( !mutedef )
	    mErrRet("Cannot create new mute definition");
				     
	if ( !MuteDefTranslator::retrieve( *mutedef, ioobj, errmsg_ ) )
	{
	    delete mutedef;
	    mErrRet(errmsg_);
	}
    }

    setMute( inner, mutedef, OD::TakeOverPtr );
    myid = mid;

    return true;
}



void EventTracker::removeMutes()
{
    if ( ownsinnermute_ ) delete innermute_;
    if ( ownsoutermute_ ) delete outermute_;

    ownsinnermute_ = 0;
    ownsoutermute_ = 0;
};


void EventTracker::fillPar( IOPar& par ) const
{
    par.set( sKeyInnerMute(), innermuteid_ );
    par.set( sKeyOuterMute(), outermuteid_ );
}


bool EventTracker::usePar( const IOPar& par )
{
    MultiID imid, omid;
    if ( !par.get( sKeyInnerMute(), imid ) ||
	 !par.get( sKeyOuterMute(), omid ) )
    {
	mErrRet( "Cannot parse mutes" );
    }

    return setMute( true, imid ) && setMute( false, omid );
}


} //namespace

