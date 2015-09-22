/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "callback.h"
#include "thread.h"

#define mOneMilliSecond 0.001

#ifndef OD_NO_QT
#include <QCoreApplication>

class CallBackEventReceiver : public QObject
{
public:
    CallBackEventReceiver()
	: receiverlock_( true )
    { cbers_.allowNull( true ); }

    bool event( QEvent* )
    {
	Threads::Locker locker( receiverlock_ );
	if ( queue_.size() )
	{
	    pErrMsg("Queue should be empty");
	}

	queue_ = cbs_;
	ObjectSet<CallBacker> cbers( cbers_ );

	cbs_.erase();
	cbers_.erase();
	locker.unlockNow();

	for ( int idx=0; idx<queue_.size(); idx++ )
	{
	    queue_[idx].doCall( cbers[idx] );
	}

	locker.reLock();
	queue_.erase();

	return true;
    }

    void add( const CallBack& cb, CallBacker* cber )
    {
	Threads::Locker locker( receiverlock_ );

	if ( cbs_.isEmpty() )
	{
	    QCoreApplication::postEvent( this, new QEvent(QEvent::None) );
	}

	toremove_.addIfNew( cb.cbObj() );

	cbs_ += cb;
	cbers_ += cber;
    }

    void removeBy( const CallBacker* cber )
    {
	Threads::Locker locker( receiverlock_ );

	for ( int idx=cbs_.size()-1; idx>=0; idx-- )
	{
	    if ( cbs_[idx].cbObj()==cber )
	    {
		cbs_.removeSingle( idx );
		cbers_.removeSingle( idx );
	    }
	}

	//Check that it is not presently running
	bool found = true;
	while ( found )
	{
	    found = false;

	    for ( int idx=queue_.size()-1; idx>=0; idx-- )
	    {
		if ( queue_[idx].cbObj()==cber )
		{
		    found = true;
		    break;
		}
	    }

	    if ( found )
	    {
		locker.unlockNow();
		Threads::sleep( 0.01 );
		locker.reLock();
	    }
	}

	toremove_ -= cber;
    }

    bool isPresent( const CallBacker* cber )
    {
	Threads::Locker locker( receiverlock_ );
	return toremove_.isPresent( cber );
    }

private:

    TypeSet<CallBack>		queue_;

    TypeSet<CallBack>		cbs_;
    ObjectSet<CallBacker>	cbers_;

    ObjectSet<const CallBacker> toremove_;

    Threads::Lock		receiverlock_;
};


static PtrMan<CallBackEventReceiver> currentreceiver = 0;

static CallBackEventReceiver* getQELR()
{
    if ( !currentreceiver )
    {
	CallBackEventReceiver* rec  = new CallBackEventReceiver;
	if ( !currentreceiver.setIfNull( rec ) )
	    delete rec;
    }

    return currentreceiver;
}

#endif





CallBacker::CallBacker()
{}


CallBacker::CallBacker( const CallBacker& )
{}


CallBacker::~CallBacker()
{
#ifndef OD_NO_QT
    CallBackEventReceiver* rec = getQELR();
    if ( attachednotifiers_.size() || rec->isPresent(this) )
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
#endif
}


void CallBacker::detachAllNotifiers()
{
    /*Avoid deadlocks (will happen if one thread deletes the notifier while
     the other thread deletes the callbacker at the same time) by using
     try-locks and retry after releasing own lock. */

    CallBack::removeFromMainThread( this );
    
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


bool CallBacker::attachCB(NotifierAccess& notif, const CallBack& cb,
			  bool onlyifnew )
{
    if ( onlyifnew )
    {
	if ( !notif.notifyIfNotNotified( cb ) )
	    return false;
    }
    else
    {
	notif.notify( cb );
    }
 
    if ( cb.cbObj()!=this )
	return true;
    
    if ( notif.cber_==this )
	return true;

    notif.addShutdownSubscription( this );
    
    Threads::Locker lckr( attachednotifierslock_ );
    if ( !attachednotifiers_.isPresent( &notif ) )
	attachednotifiers_ += &notif;

    return true;
}

 
void CallBacker::detachCB( NotifierAccess& notif, const CallBack& cb )
{
    if ( cb.cbObj()!=this || notif.cber_==this )
    {
	//Lets hope notif is still alive
	notif.remove( cb );
	return;
    }
    
    Threads::Locker lckr( attachednotifierslock_ );
    if ( !attachednotifiers_.isPresent( &notif ) )
    {
	//It may be deleted. Don't touch it
	return;
    }

    notif.remove( cb );

    if ( !notif.willCall( this ) )
    {
    	while ( attachednotifiers_.isPresent( &notif ) )
    	{
            if ( notif.removeShutdownSubscription( this, false ) )
            {
                attachednotifiers_ -= &notif;
            }
            else
            {
                lckr.unlockNow();
                Threads::sleep( mOneMilliSecond );
                lckr.reLock();
            }
	}
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


void CallBack::initClass()
{
#ifndef OD_NO_QT
    getQELR(); //Force creation
#endif
}


void CallBack::doCall( CallBacker* cber )
{
    if ( obj_ && fn_ )
	(obj_->*fn_)( cber );
    else if ( sfn_ )
	sfn_( cber );
}


bool CallBack::addToMainThread( CallBack cb, CallBacker* cber )
{
#ifndef OD_NO_QT
    CallBackEventReceiver* rec = getQELR();
    rec->add( cb, cber );
    return true;
#endif

    return false;
}


bool CallBack::queueIfNotInMainThread( CallBack cb, CallBacker* cber )
{
#ifndef OD_NO_QT
    QCoreApplication* instance = QCoreApplication::instance();
    if ( instance && instance->thread()!=Threads::currentThread() )
    {
	CallBackEventReceiver* rec = getQELR();
	rec->add( cb, cber );
	return true;
    }
#endif

    return false;
}


bool CallBack::callInMainThread( CallBack cb, CallBacker* cber )
{
    if ( addToMainThread( cb, cber ) )
	return false;

    cb.doCall( cber );
    return true;
}


void CallBack::removeFromMainThread( const CallBacker* cber )
{
#ifdef OD_NO_QT
    return false;
#else
    CallBackEventReceiver* rec = getQELR();
    rec->removeBy( cber );
#endif
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
    , enabled_( rcbs_->enabled_ )
    , rcbs_( new RefCountCallBackSet )
    , cbs_( *rcbs_ )
{
    Threads::Locker lckr( na.cbs_.lock_ );
    cbs_ = na.cbs_;
}


NotifierAccess::NotifierAccess()
    : rcbs_( new RefCountCallBackSet )
    , enabled_( rcbs_->enabled_ )
    , cbs_( *rcbs_ )
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


bool NotifierAccess::notifyIfNotNotified( const CallBack& cb )
{
    Threads::Locker lckr( cbs_.lock_ );
    return cbs_.addIfNew( cb );
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


void NotifierAccess::doTrigger( RefCountCallBackSet& cbs, CallBacker* c,
				CallBacker* exclude )
{
    cbs.ref();
    cbs.doCall( c, &cbs.enabled_, exclude );
    cbs.unRef();
}
