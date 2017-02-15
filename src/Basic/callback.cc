/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/


#include "notify.h"
#include "thread.h"
#include "ptrman.h"
#include "manobjectset.h"


#define mOneMilliSecond 0.001

#ifndef OD_NO_QT
#include <QCoreApplication>
#include <qpointer.h>

static QEvent::Type qevent_type_ = QEvent::None;


class QCallBackEventReceiver : public QObject
{
public:
    QCallBackEventReceiver( Threads::ThreadID threadid)
	: receiverlock_( true )
	, threadid_( threadid )
    { cbers_.allowNull( true ); }

    Threads::ThreadID threadID() const
    { return threadid_; }

    bool event( QEvent* ev )
    {
	if (ev->type() != qevent_type_ )
	    return false;

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
	    queue_[idx].doCall( cbers[idx] );

	for ( int idx=cbers.size()-1; idx>=0; idx-- )
	{
	    if ( cbers[idx] && cbers[idx]->isCapsule() )
		delete cbers.removeSingle( idx );
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
	    QCoreApplication::postEvent( this, new QEvent(qevent_type_) );
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

    ObjectSet<const CallBacker>	toremove_;

    Threads::Lock		receiverlock_;
    const Threads::ThreadID	threadid_;
};


static Threads::Lock callbackeventreceiverslock;
static ObjectSet<QCallBackEventReceiver> callbackeventreceivers;

static QCallBackEventReceiver* getQCBER( Threads::ThreadID thread )
{
    for (int idx = 0; idx < callbackeventreceivers.size(); idx++)
    {
	if (callbackeventreceivers[idx]->threadID() == thread)
	    return callbackeventreceivers[idx];
    }

    return 0;
}


void CallBacker::createReceiverForCurrentThread()
{
    const Threads::ThreadID curthread = Threads::currentThread();

    Threads::Locker locker(callbackeventreceiverslock);

    if ( getQCBER(curthread) )
	return;

    QCallBackEventReceiver* res = new QCallBackEventReceiver(curthread);
    callbackeventreceivers += res;
}


void CallBacker::removeReceiverForCurrentThread()
{
    const Threads::ThreadID curthread = Threads::currentThread();

    Threads::Locker locker(callbackeventreceiverslock);
    for (int idx = 0; idx < callbackeventreceivers.size(); idx++)
    {
	if (callbackeventreceivers[idx]->threadID() == curthread)
	{
	    delete callbackeventreceivers.removeSingle(idx);
	    break;
	}
    }
}


static bool isPresent(const CallBacker* cber)
{
    Threads::Locker locker(callbackeventreceiverslock);
    for (int idx = 0; idx < callbackeventreceivers.size(); idx++)
    {
	if (callbackeventreceivers[idx]->isPresent(cber))
	    return true;
    }
    return false;
}


#endif // OD_NO_QT


//---- CallBacker

CallBacker::CallBacker()
{
}


CallBacker::CallBacker( const CallBacker& )
{
}


CallBacker::~CallBacker()
{
#ifndef OD_NO_QT
    if ( attachednotifiers_.size() || isPresent(this) )
    {
	pErrMsg("Notifiers not disconnected.");
	/* Notifiers should be removed in the class where they were attached,
	   normally by calling detachAllNotifiers in the destructor of that
	   class.
	   If not done, they may still get callbacks after the destuctor
	   is called, but before this destructor is called.

	   If you end up here, go up in the debugger and find the destructor
	   where you should insert detachAllNotifiers().
	 */

	//Remove them now.
	detachAllNotifiers();
    }
# endif
}


void CallBacker::detachAllNotifiers()
{
    CallBack::removeFromThreadCalls( this );

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


bool CallBacker::attachCB(NotifierAccess* notif,const CallBack& cb,
	bool onlyifnew)
{
    return notif
	? attachCB(*notif,cb,onlyifnew)
	: false;
}


bool CallBacker::attachCB(NotifierAccess& notif, const CallBack& cb,
			  bool onlyifnew )
{
    CallBacker* cbobj = const_cast<CallBacker*>( cb.cbObj() );
    if ( cbobj!=this )
    {
	pErrMsg("You can only attach a callback to yourself" );
	return false;
    }

    if ( onlyifnew )
    {
	if ( !notif.notifyIfNotNotified( cb ) )
	    return false;
    }
    else
    {
	notif.notify( cb );
    }

    //If the notifier is belonging to me, it will only be messy if
    // we subscribe to the shutdown messages.
    if ( notif.cber_!=this )
	notif.addShutdownSubscription( this );

    Threads::Locker lckr( attachednotifierslock_ );
    if ( !attachednotifiers_.isPresent( &notif ) )
	attachednotifiers_ += &notif;

    return true;
}


void CallBacker::detachCB( NotifierAccess& notif, const CallBack& cb )
{
    if ( cb.cbObj()!=this )
    {
	pErrMsg( "You can only detach a callback to yourself" );
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


//---- CallBack
Threads::ThreadID CallBack::mainthread_ = 0;


bool CallBack::operator==( const CallBack& c ) const
{
    return cberobj_ == c.cberobj_ && fn_ == c.fn_ && sfn_ == c.sfn_;
}


void CallBack::initClass()
{
#ifndef OD_NO_QT
    mainthread_ = Threads::currentThread();
    qevent_type_ = (QEvent::Type)QEvent::registerEventType();
    CallBacker::createReceiverForCurrentThread(); //Force creation
#endif
}


void CallBack::doCall( CallBacker* cber ) const
{
    if ( !disablecount_ )
    {
	if ( cberobj_ && fn_ )
	    (cberobj_->*fn_)( cber );
	else if ( sfn_ )
	    sfn_( cber );
    }
}


void CallBack::disable( bool yn ) const
{
    if ( yn )
	disablecount_++;
    else if ( disablecount_ > 0 )
	disablecount_--;
}


bool CallBack::addToMainThread( const CallBack& cb, CallBacker* cber )
{
#ifdef OD_NO_QT
    return false;
#else
    return addToThread( mainthread_, cb, cber);
#endif
}


bool CallBack::addToThread( Threads::ThreadID threadid, const CallBack& cb,
			    CallBacker* cber)
{
    Threads::Locker locker(callbackeventreceiverslock);
    QCallBackEventReceiver* rec = getQCBER(threadid);

    if (!rec)
    {
	pFreeFnErrMsg("Thread does not have a receiver. Create in the thread "
		  "by calling CallBacker::createReceiverForCurrentThread()");
	return false;
    }

    rec->add( cb, cber );
    return true;
}


bool CallBack::queueIfNotInMainThread( CallBack cb, CallBacker* cber )
{
#ifndef OD_NO_QT
    if ( mainthread_!=Threads::currentThread() )
    {
	if (!addToThread(mainthread_, cb, cber))
	{
	    pFreeFnErrMsg("Main thread not initialized.");
	}

	return true;
    }
#endif

    return false;
}


bool CallBack::callInMainThread( const CallBack& cb, CallBacker* cber )
{
    if ( addToMainThread( cb, cber ) )
	return false;

    cb.doCall( cber );
    return true;
}


void CallBack::removeFromThreadCalls(const CallBacker* cber)
{
#ifndef OD_NO_QT
    Threads::Locker locker(callbackeventreceiverslock);
    for (int idx = 0; idx < callbackeventreceivers.size(); idx++)
    {
	callbackeventreceivers[idx]->removeBy(cber);
    }
#endif
}


//---- CallBackSet

CallBackSet::CallBackSet()
    : lock_(true)
{
}


CallBackSet::CallBackSet( const CallBackSet& cbs )
    : TypeSet<CallBack>( cbs )
    , lock_( true )
{
}


CallBackSet::~CallBackSet()
{
}


CallBackSet& CallBackSet::operator=( const CallBackSet& cbs )
{
    Threads::Locker lckr( cbs.lock_ );
    TypeSet<CallBack>::operator=( cbs );
    return *this;
}


void CallBackSet::doCall( CallBacker* obj )
{
    Threads::Locker lckr( lock_ );
    TypeSet<CallBack> cbscopy = *this;
    lckr.unlockNow();

    for ( int idx=0; idx<cbscopy.size(); idx++ )
    {
	CallBack cb = cbscopy[idx];
	lckr.reLock();
	if ( !isPresent(cb) )
	    { lckr.unlockNow(); continue; }

	lckr.unlockNow();
	cb.doCall( obj );
    }
}


void CallBackSet::disableAll( bool yn )
{
    Threads::Locker lckr( lock_ );
    for ( int idx=0; idx<size(); idx++ )
	(*this)[idx].disable( yn );
}


bool CallBackSet::hasAnyDisabled() const
{
    Threads::Locker lckr( lock_ );
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx].isDisabled() )
	    return true;
    return false;
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


void CallBackSet::transferTo( CallBackSet& to, const CallBacker* onlyfor )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const CallBack& cb = (*this)[idx];
	if ( !onlyfor || cb.cbObj() == onlyfor )
	{
	    if ( !to.isPresent(cb) )
		to += cb;
	    removeSingle( idx );
	    idx--;
	}
    }
}


//---- NotifierAccess

NotifierAccess::NotifierAccess( const NotifierAccess& na )
    : cber_( na.cber_ )
    , cbs_(*new CallBackSet(na.cbs_) )
{
    cbs_.ref();
}


NotifierAccess::NotifierAccess()
    : cbs_(*new CallBackSet )
{
    cbs_.ref();
}


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

    cbs_.unRef();
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


void NotifierAccess::doTrigger( CallBackSet& cbs, CallBacker* cber )
{
    cbs.ref();
    cbs.doCall( cber );
    cbs.unRef();
}



//---- NotifyStopper

NotifyStopper::NotifyStopper( NotifierAccess& na, const CallBacker* only_for )
    : thenotif_(na)
    , isdisabled_(false)
    , onlyfor_(only_for)
{
    disableNotification();
}


NotifyStopper::~NotifyStopper()
{
    enableNotification();
}


void NotifyStopper::setDisabled( bool yn )
{
    Threads::Locker locker( thenotif_.cbs_.lock_ );
    for ( int idx=0; idx<thenotif_.cbs_.size(); idx++ )
    {
	CallBack& cb = thenotif_.cbs_[idx];
	if ( !onlyfor_ || cb.cbObj() == onlyfor_ )
	    cb.disable( yn );
    }
    isdisabled_ = yn;
}


void NotifyStopper::enableNotification()
{
    if ( isdisabled_ )
	setDisabled( false );
}


void NotifyStopper::disableNotification()
{
    if ( !isdisabled_ )
	setDisabled( true );
}
