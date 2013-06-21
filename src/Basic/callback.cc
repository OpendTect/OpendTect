/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "callback.h"

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
    
    Threads::SpinLockLocker lock( attachednotifierslock_ );
    
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
	    lock.unLock();
	    Threads::sleep( mOneMilliSecond );
	    lock.lock();
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
    
    Threads::SpinLockLocker lock( attachednotifierslock_ );
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
	Threads::SpinLockLocker lock( attachednotifierslock_ );
	attachednotifiers_ -= &notif;
    }
}


bool CallBacker::isNotifierAttached( NotifierAccess* na ) const
{
    Threads::SpinLockLocker lock( attachednotifierslock_ );
    return attachednotifiers_.isPresent( na );
}


#define mLock( thelock ) \
    if ( wait ) \
	thelock.lock(); \
    else if ( !thelock.tryLock() ) \
	return false

#define mUnlockRet(thelock) \
    thelock.unLock(); \
    return true


bool CallBacker::notifyShutdown( NotifierAccess* na, bool wait )
{
    mLock( attachednotifierslock_ );

    attachednotifiers_ -= na;

    mUnlockRet( attachednotifierslock_ );
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

    lock_.lock();
    TypeSet<CallBack> cbscopy = *this;
    lock_.unLock();
    
    for ( int idx=0; idx<cbscopy.size(); idx++ )
    {
	CallBack& cb = cbscopy[idx];
	lock_.lock();
	if ( !isPresent(cb) )
	{
	    lock_.unLock();
	    continue;
	}
	
	lock_.unLock();

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
    Threads::SpinLockLocker lock( na.cbs_.lock_ );
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
    
    Threads::SpinLockLocker lock( shutdownsubscriberlock_ );
    while ( shutdownsubscribers_.size() )
    {
	for ( int idx=shutdownsubscribers_.size()-1; idx>=0; idx-- )
	{
	    if ( shutdownsubscribers_[idx]->notifyShutdown( this, false ) )
		shutdownsubscribers_.removeSingle( idx );
	}
	
	if ( shutdownsubscribers_.size() )
	{
	    lock.unLock();
	    Threads::sleep( mOneMilliSecond );
	    lock.lock();
	}
    }
}


void NotifierAccess::addShutdownSubscription( CallBacker* cber )
{
    Threads::SpinLockLocker lock( shutdownsubscriberlock_ );
    shutdownsubscribers_.addIfNew( cber );
}


bool NotifierAccess::isShutdownSubscribed( CallBacker* cber ) const
{    
    Threads::SpinLockLocker lock( shutdownsubscriberlock_ );
    return shutdownsubscribers_.isPresent( cber );
}


bool NotifierAccess::removeShutdownSubscription( CallBacker* cber, bool wait )
{
    mLock( shutdownsubscriberlock_ );

    shutdownsubscribers_ -= cber;
    mUnlockRet( shutdownsubscriberlock_ );
}


void NotifierAccess::notify( const CallBack& cb, bool first )
{
    Threads::SpinLockLocker lock( cbs_.lock_ );
    
    if ( first ) 
	cbs_.insert(0,cb); 
    else
	cbs_ += cb; 
}


void NotifierAccess::notifyIfNotNotified( const CallBack& cb )
{
    Threads::SpinLockLocker lock( cbs_.lock_ );
    cbs_.addIfNew( cb );
}


void NotifierAccess::remove( const CallBack& cb )
{
    Threads::SpinLockLocker lock( cbs_.lock_ );

    cbs_ -= cb;
}


bool NotifierAccess::removeWith( CallBacker* cber, bool wait )
{
    mLock( cbs_.lock_ );

    if ( cber_ == cber )
	{ cbs_.erase(); cber_ = 0; return true; }

    cbs_.removeWith( cber );

    mUnlockRet( cbs_.lock_ );
}


bool NotifierAccess::willCall( CallBacker* cber ) const
{
    Threads::SpinLockLocker locker( cbs_.lock_ );
    
    for ( int idx=0; idx<cbs_.size(); idx++ )
    {
        if ( cbs_[idx].cbObj()==cber )
            return true;
    }
    
    return false;
}
