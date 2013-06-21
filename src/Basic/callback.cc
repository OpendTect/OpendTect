/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "callback.h"
#include "thread.h"
#include "errh.h"

#define mOneMilliSecond 0.001


CallBacker::CallBacker()
{}


CallBacker::CallBacker( const CallBacker& )
{}


CallBacker::~CallBacker()
{
    if ( attachednotifiers_.size() )
    {
	pErrMsg("Notifiers not detached.");
	/* Notifiers should be removed in the class where they were attached,
	   normally by calling detachAllNotifiers in the destructor of that
	   class.
	   If not done, they may still get callbacks after the destuctor
	   is called, but before this destructor is called.
	 */
	
	//Remove them now.
	detachAllNotifiers();
    }
}


void CallBacker::detachAllNotifiers()
{
    /*Avoid deadlocks (will happen if one thread deletes the notifier while
     the other thread deletes the callbacker at the same time) by using
     try-locks and retry after releasing own lock. */
    
    Threads::Locker lckr( attachednotifierslock_ );
    
    while ( attachednotifiers_.size() )
    {
	for ( int idx=attachednotifiers_.size()-1; idx>=0; idx-- )
	{
	    NotifierAccess* na = attachednotifiers_[idx];
	    if ( na->removeWith( this, false ) &&
		na->removeShutdownSubscription( this, false ) )
		attachednotifiers_.removeSingle( idx );
	    
	}
	
	if ( attachednotifiers_.size() )
	{
	    lckr.unlockNow();
	    Threads::sleep( mOneMilliSecond );
	    lckr.reLock();
	}
    }
}


void CallBacker::attachCB(NotifierAccess& notif, const CallBack& cb )
{
    notif.notify( cb );
 
    if ( cb.cbObj()!=this )
	return;
    
    if ( notif.cber_==this )
	return;

    notif.addShutdownSubscription( this );
    
    Threads::Locker lckr( attachednotifierslock_ );
    if ( !attachednotifiers_.isPresent( &notif ) )
	attachednotifiers_ += &notif;
}

 
void CallBacker::detachCB( NotifierAccess& notif, const CallBack& cb )
{
    notif.remove( cb );
    
    if ( cb.cbObj()!=this )
	return;
    
    if ( notif.cber_==this )
	return;
    
    if ( !notif.willCall( this ) )
    {
	Threads::Locker lckr( attachednotifierslock_ );
	attachednotifiers_ -= &notif;
    }
}


bool CallBacker::isNotifierAttached( NotifierAccess* na ) const
{
    Threads::Locker lckr( attachednotifierslock_ );
    return attachednotifiers_.isPresent( na );
}


#define mGetLocker( thelock, wait ) \
    Threads::Locker lckr( thelock, wait ? Threads::Locker::WaitIfLocked \
	    				: Threads::Locker::DontWaitForLock ); \
    if ( !lckr.isLocked() ) return false


bool CallBacker::notifyShutdown( NotifierAccess* na, bool wait )
{
    mGetLocker( attachednotifierslock_, wait );
    attachednotifiers_ -= na;
    return true;
}


void CallBack::doCall( CallBacker* cber )
{
    if ( obj_ && fn_ )
	(obj_->*fn_)( cber );
    else if ( sfn_ )
	sfn_( cber );
}


void CallBackSet::doCall( CallBacker* obj, const bool* enabledflag,
			  CallBacker* exclude )
{
    const bool enab = true;
    const bool& enabled = enabledflag ? *enabledflag : enab;
    if ( !enabled ) return;

    Threads::Locker lckr( lock_ );
    TypeSet<CallBack> cbscopy = *this;
    lckr.unlockNow();
    
    for ( int idx=0; idx<cbscopy.size(); idx++ )
    {
	CallBack& cb = cbscopy[idx];
	lckr.reLock();
	if ( !isPresent(cb) )
	    { lckr.unlockNow(); continue; }
	
	lckr.unlockNow();
	if ( !exclude || cb.cbObj()!=exclude )
	    cb.doCall( obj );
    }
}


void CallBackSet::removeWith( CallBacker* cbrm )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.cbObj() == cbrm )
	    { removeSingle( idx ); idx--; }
    }
}


void CallBackSet::removeWith( CallBackFunction cbfn )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.cbFn() == cbfn )
	    { removeSingle( idx ); idx--; }
    }
}


void CallBackSet::removeWith( StaticCallBackFunction cbfn )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.scbFn() == cbfn )
	    { removeSingle( idx ); idx--; }
    }
}


NotifierAccess::NotifierAccess( const NotifierAccess& na )
    : cber_( na.cber_ )
    , enabled_( na.enabled_ )
{
    Threads::Locker lckr( na.cbs_.lock_ );
    cbs_ = na.cbs_;
}


NotifierAccess::NotifierAccess()
    : enabled_(true)		
{}


NotifierAccess::~NotifierAccess()
{
    /*Avoid deadlocks (will happen if one thread deletes the notifier while
     the other thread deletes the callbacker at the same time) by using
     try-locks and retry after releasing own lock. */
    
    Threads::Locker lckr( shutdownsubscriberlock_ );
    while ( shutdownsubscribers_.size() )
    {
	for ( int idx=shutdownsubscribers_.size()-1; idx>=0; idx-- )
	{
	    if ( shutdownsubscribers_[idx]->notifyShutdown( this, false ) )
		shutdownsubscribers_.removeSingle( idx );
	}
	
	if ( shutdownsubscribers_.size() )
	{
	    lckr.unlockNow();
	    Threads::sleep( mOneMilliSecond );
	    lckr.reLock();
	}
    }
}


void NotifierAccess::addShutdownSubscription( CallBacker* cber )
{
    Threads::Locker lckr( shutdownsubscriberlock_ );
    shutdownsubscribers_.addIfNew( cber );
}


bool NotifierAccess::isShutdownSubscribed( CallBacker* cber ) const
{    
    Threads::Locker lckr( shutdownsubscriberlock_ );
    return shutdownsubscribers_.isPresent( cber );
}


bool NotifierAccess::removeShutdownSubscription( CallBacker* cber, bool wait )
{
    mGetLocker( shutdownsubscriberlock_, wait );
    shutdownsubscribers_ -= cber;
    return true;
}


void NotifierAccess::notify( const CallBack& cb, bool first )
{
    Threads::Locker lckr( cbs_.lock_ );
    
    if ( first ) 
	cbs_.insert(0,cb); 
    else
	cbs_ += cb; 
}


void NotifierAccess::notifyIfNotNotified( const CallBack& cb )
{
    Threads::Locker lckr( cbs_.lock_ );
    cbs_.addIfNew( cb );
}


void NotifierAccess::remove( const CallBack& cb )
{
    Threads::Locker lckr( cbs_.lock_ );

    cbs_ -= cb;
}


bool NotifierAccess::removeWith( CallBacker* cber, bool wait )
{
    mGetLocker( cbs_.lock_, wait );
    if ( cber_ == cber )
	{ cbs_.erase(); cber_ = 0; return true; }
    cbs_.removeWith( cber );
    return true;
}


bool NotifierAccess::willCall( CallBacker* cber ) const
{
    Threads::Locker lckr( cbs_.lock_ );
    
    for ( int idx=0; idx<cbs_.size(); idx++ )
    {
        if ( cbs_[idx].cbObj()==cber )
            return true;
    }
    
    return false;
}
