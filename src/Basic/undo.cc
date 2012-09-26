/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "undo.h"

#include "errh.h"
#include "iopar.h"
#include "position.h"


Undo::Undo()
    : currenteventid_(-1)
    , firsteventid_(0)
    , maxsize_( 1000 )
    , userendscount_(0)
    , changenotifier(this)
    , undoredochange(this)
{ }


Undo::~Undo()
{
    deepErase( events_ );
}


#define mStoreUndoRedoState() \
    const bool oldcanundostate = canUnDo(); \
    const bool oldcanredostate = canReDo(); \
    BufferString oldundodesc = unDoDesc(); \
    BufferString oldredodesc = reDoDesc();

#define mTriggerIfUndoRedoChange() \
    if ( oldcanundostate!=canUnDo() || oldundodesc!=unDoDesc() || \
	 oldcanredostate!=canReDo() || oldredodesc!=reDoDesc() ) \
	undoredochange.trigger();


void Undo::removeAll()
{
    mStoreUndoRedoState();
    deepErase( events_ );
    userendscount_ = 0;

    currenteventid_ = -1;
    firsteventid_ = 0;

    changenotifier.trigger();
    mTriggerIfUndoRedoChange();
}


int Undo::maxLength() const
{ return maxsize_; }


void Undo::setMaxLength( int maxsize )
{
    maxsize_ = maxsize;
    removeOldEvents();
    changenotifier.trigger();
}


bool Undo::isUserInteractionEnd( int eventid ) const
{
    const int idx = indexOf( eventid );
    return idx<0 ? 0 : events_[idx]->isUserInteractionEnd();
}


#define mUpdateUserEndsCount( idx, newboolval ) \
    if ( events_[idx]->isUserInteractionEnd() != newboolval ) \
	userendscount_ += newboolval ? 1 : -1;


void Undo::setUserInteractionEnd( int eventid, bool yn )
{
    const int idx = indexOf( eventid );
    if ( idx<0 ) return;
    mUpdateUserEndsCount(idx,yn); 
    events_[idx]->setUserInteractionEnd( yn );
}


int Undo::getNextUserInteractionEnd( int startid ) const
{
    int idx = indexOf( startid );
    if ( idx<0 ) return -1;

    while ( idx<events_.size() && !events_[idx]->isUserInteractionEnd() )
	idx++;

    if ( idx>=events_.size() )
	return -1;

    return idx+firsteventid_;
}


int Undo::currentEventID() const
{ return currenteventid_; }


int Undo::firstEventID() const
{ return firsteventid_; }


void Undo::removeAllBeforeCurrentEvent()
{
    removeStartToAndIncluding( currentEventID()-1 );
}


int Undo::lastEventID() const
{ return firsteventid_+events_.size()-1; }


void Undo::removeAllAfterCurrentEvent()
{
    for ( int event=lastEventID(); event>currentEventID(); event-- )
    {
	const int idx = indexOf(event);

	mUpdateUserEndsCount(idx,false);
	delete events_.remove(idx);
    }

    changenotifier.trigger();
}


BufferString Undo::getDesc( int eventid ) const
{
    const int idx = indexOf( eventid );
    if ( idx<0 ) return BufferString("");
    return events_[idx]->getDesc();
}


BufferString Undo::unDoDesc() const
{
    return getDesc( currentEventID() );
}


BufferString Undo::reDoDesc() const
{
    const int eventid = getNextUserInteractionEnd( currentEventID()+1 );
    return getDesc( eventid );
}


void Undo::setDesc( int eventid, const char* d )
{
    const int idx = indexOf( eventid );
    events_[idx]->setDesc( d );
    changenotifier.trigger();
}


int Undo::addEvent( UndoEvent* event, const char* description )
{
    mStoreUndoRedoState();
    if ( canReDo() )
    {
	NotifyStopper stop( changenotifier );
	removeAllAfterCurrentEvent();
    }

    event->setDesc( description );    
    events_ += event;
    currenteventid_++;

    removeOldEvents();
    changenotifier.trigger();
    mTriggerIfUndoRedoChange();
    return currenteventid_;
}


bool Undo::canUnDo() const
{
    return currenteventid_ >= firsteventid_;
}


bool Undo::unDo( int nrtimes, bool userinteraction )
{
    mStoreUndoRedoState();
    bool change = false;
    while ( nrtimes && canUnDo() )
    {
	const int idx = indexOf( currenteventid_ );
	if ( idx<0 )
	    return false;

	const bool prevend = idx && events_[idx-1]->isUserInteractionEnd();
	if ( !events_[idx]->unDo() )
	{
	    if ( prevend )
    		removeAllBeforeCurrentEvent();
	    else
		removeStartToAndIncluding( currenteventid_ );
	    
	    return false;
	}

	currenteventid_--;
	change = true;

	if ( !userinteraction || prevend )
	    nrtimes--;
    }

    if ( change )
    {
	changenotifier.trigger();
    }

    mTriggerIfUndoRedoChange();
    return true;
}


bool Undo::canReDo() const
{
    return currenteventid_ < lastEventID();
}


bool Undo::reDo( int nrtimes, bool userinteraction )
{
    mStoreUndoRedoState();
    bool change = false;
    while ( nrtimes && canReDo() )
    {
	const int idx = indexOf( currenteventid_+1 );
	if ( idx<0 )
	    return false;

	if ( !events_[idx]->reDo() )
	{
	    removeAllAfterCurrentEvent();
	    return false;
	}

	currenteventid_++;
	change = true;

	if ( !userinteraction || events_[idx]->isUserInteractionEnd() )
	    nrtimes--;
    }

    if ( change )
    {
	changenotifier.trigger();
    }

    mTriggerIfUndoRedoChange();
    return true;
}


int Undo::indexOf( int eventid ) const
{
    const int relpos = eventid-firsteventid_;
    if ( relpos<0 || relpos>=events_.size() )
	return -1;

    return relpos;
}


void Undo::removeStartToAndIncluding( int eventid )
{
    mStoreUndoRedoState();
    while ( firsteventid_<=eventid )
    {
	mUpdateUserEndsCount(0,false);
	delete events_.remove(0);
	firsteventid_++;
    }

    if ( currenteventid_<firsteventid_ )
	currenteventid_ = firsteventid_-1;

    changenotifier.trigger();
    mTriggerIfUndoRedoChange();
}


void Undo::removeOldEvents()
{
    if ( userendscount_<=maxsize_ || maxsize_==-1 ) return;

    int firstkepteventidx = indexOf( currentEventID() );
    for ( int nrkept=0; nrkept<maxsize_; nrkept++ )
    {
	firstkepteventidx--;
	while ( firstkepteventidx>=0 &&
		!events_[firstkepteventidx]->isUserInteractionEnd() )
	    firstkepteventidx--;

	if ( firstkepteventidx<0 )
	    return;
    }

    removeStartToAndIncluding( firstkepteventidx+firsteventid_-1 );
}


UndoEvent::UndoEvent() 
    : desc_( 0 ) 
    , isuserinteractionend_( false ) 
{} 


UndoEvent::~UndoEvent() { delete desc_; } 


void UndoEvent::setUserInteractionEnd( bool yn )
{ isuserinteractionend_=yn; }


bool UndoEvent::isUserInteractionEnd() const
{ return isuserinteractionend_; }


void UndoEvent::setDesc( const char* d )
{
    if ( d && !desc_ )
    {
	mTryAlloc( desc_, BufferString( d ) );
    }
    else if ( !d && desc_ )
    {
	delete desc_;
	desc_ = 0;
    }
    else if ( d && desc_ )
	*desc_ = d;
}


BufferString UndoEvent::getDesc() const
{
    if ( desc_ ) return *desc_;
    return getStandardDesc();
}


const BinID& BinIDUndoEvent::getBinID() const
{
    static BinID res( mUdf(int), mUdf(int) );
    return res;
}


