/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackeventtracker.h"

#include "ioman.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "uistrings.h"

namespace PreStack
{

// EventTracker

EventTracker::EventTracker()
{
}


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
    myid.setUdf();

    PreStack::MuteDef* mutedef = 0;

    if ( !mid.isUdf() )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	    mErrRet( tr("Cannot find MuteDef ID in Object Manager") );
	mutedef = new PreStack::MuteDef;

	if ( !mutedef )
	    mErrRet( uiStrings::phrCannotCreate(tr("new mute definition")) );

	if ( !MuteDefTranslator::retrieve( *mutedef, ioobj.ptr(), errmsg_ ) )
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
	mErrRet( uiStrings::phrCannotRead( uiStrings::sMute(mPlural)) );
    }

    return setMute( true, imid ) && setMute( false, omid );
}


} // namespace PreStack
