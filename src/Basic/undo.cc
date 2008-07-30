/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: undo.cc,v 1.3 2008-07-30 22:55:09 cvskris Exp $
________________________________________________________________________

-*/

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
{
    descs_.allowNull( true );
}


Undo::~Undo()
{
    deepErase( events_ );
    deepErase( descs_ );
}


void Undo::removeAll()
{
    deepErase( events_ );
    deepErase( descs_ );
    userinteractionends_.erase();
    userendscount_ = 0;

    currenteventid_ = -1;
    firsteventid_ = 0;

    changenotifier.trigger();
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
    return idx<0 ? 0 : userinteractionends_[idx];
}


#define mUpdateUserEndsCount( idx, newboolval ) \
    if ( userinteractionends_[idx] != newboolval ) \
	userendscount_ += newboolval ? 1 : -1;


void Undo::setUserInteractionEnd( int eventid, bool yn )
{
    const int idx = indexOf( eventid );
    if ( idx<0 ) return;
    mUpdateUserEndsCount(idx,yn); 
    userinteractionends_[idx] = yn;
}


int Undo::getNextUserInteractionEnd( int startid ) const
{
    int idx = indexOf( startid );
    if ( idx<0 ) return -1;

    while ( idx<userinteractionends_.size() && !userinteractionends_[idx] )
	idx++;

    if ( idx>=userinteractionends_.size() )
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

	delete events_.remove(idx);
	delete descs_.remove(idx);
	mUpdateUserEndsCount(idx,false);
	userinteractionends_.remove(idx);
    }

    changenotifier.trigger();
}


BufferString Undo::getDesc( int eventid ) const
{
    const int idx = indexOf( eventid );
    if ( idx<0 ) return BufferString("");
    if ( descs_[idx] ) return *descs_[idx];

    return events_[idx]->getStandardDesc();
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
    if ( descs_[idx] ) delete descs_[idx];

    descs_.replace( idx, d ? new BufferString( d ) : 0 );
    changenotifier.trigger();
}


int Undo::addEvent( UndoEvent* event, const char* description )
{
    if ( canReDo() )
    {
	NotifyStopper stop( changenotifier );
	removeAllAfterCurrentEvent();
    }
	
    events_ += event;
    descs_ += description ? new BufferString( description ) : 0;
    userinteractionends_ += false;
    currenteventid_++;

    removeOldEvents();
    changenotifier.trigger();
    return currenteventid_;
}


bool Undo::canUnDo() const
{
    return currenteventid_ >= firsteventid_;
}


bool Undo::unDo( int nrtimes, bool userinteraction )
{
    bool change = false;
    while ( nrtimes && canUnDo() )
    {
	const int idx = indexOf( currenteventid_ );
	if ( idx<0 )
	    return false;

	if ( !events_[idx]->unDo() )
	{
	    removeAllBeforeCurrentEvent();
	    return false;
	}

	currenteventid_--;
	change = true;

	if ( !userinteraction || (idx && userinteractionends_[idx-1]) )
	    nrtimes--;
    }

    if ( change )
	changenotifier.trigger();

    return true;
}


bool Undo::canReDo() const
{
    return currenteventid_ < lastEventID();
}


bool Undo::reDo( int nrtimes, bool userinteraction )
{
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

	if ( !userinteraction || userinteractionends_[idx] )
	    nrtimes--;
    }

    if ( change )
	changenotifier.trigger();

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
    while ( firsteventid_<=eventid )
    {
	delete events_.remove(0);
	delete descs_.remove(0);
	mUpdateUserEndsCount(0,false);
	userinteractionends_.remove(0);
	firsteventid_++;
    }

    if ( currenteventid_<firsteventid_ )
	currenteventid_ = firsteventid_-1;

    changenotifier.trigger();
}


void Undo::removeOldEvents()
{
    if ( userendscount_<=maxsize_ || maxsize_==-1 ) return;

    int firstkepteventidx = indexOf( currentEventID() );
    for ( int nrkept=0; nrkept<maxsize_; nrkept++ )
    {
	firstkepteventidx--;
	while (firstkepteventidx>=0 && !userinteractionends_[firstkepteventidx])
	    firstkepteventidx--;

	if ( firstkepteventidx<0 )
	    return;
    }

    removeStartToAndIncluding( firstkepteventidx+firsteventid_-1 );
}


const BinID& BinIDUndoEvent::getBinID() const
{
    static BinID res( mUdf(int), mUdf(int) );
    return res;
}


