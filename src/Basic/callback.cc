/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "callback.h"
#include "notify.h"

#include "thread.h"


#define mOneMilliSecond 0.001

#ifndef OD_NO_QT
#include <QCoreApplication>
#include <qpointer.h>


mUseType( Threads, ThreadID );
mUseType( Threads, Lock );
mUseType( Threads, Locker );
static QEvent::Type the_qevent_type = QEvent::None;


class QCallBackEventReceiver : public QObject
{
public:
QCallBackEventReceiver( ThreadID threadid, QObject* p=nullptr )
    : QObject(p)
    , receiverlock_( true )
    , threadid_( threadid )
{
    cbers_.setNullAllowed();
}

bool event( QEvent* ev ) override
{
    if ( ev->type() != the_qevent_type )
	return QObject::event( ev );

    Locker locker( receiverlock_ );
    if ( !queue_.isEmpty() )
    {
	static int occurrences = 0;
	if ( occurrences%100 == 0 )
	{
	    BufferString msg( "Queue should be empty, size=", queue_.size() );
	    msg.add( " (occurrence " ).add( occurrences+1 ).add( ")" );
	    pErrMsg( msg );
	}
	occurrences++;
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
    Locker locker( receiverlock_ );

    if ( cbs_.isEmpty() )
    {
	QCoreApplication::postEvent( this, new QEvent(the_qevent_type) );
    }

    toremove_.addIfNew( cb.cbObj() );

    cbs_ += cb;
    cbers_ += cber;
}

void removeBy( const CallBacker* cber )
{
    Locker locker( receiverlock_ );

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
	    Threads::sleep( 10*mOneMilliSecond );
	    locker.reLock();
	}
    }

    toremove_ -= cber;
}

bool isPresent( const CallBacker* cber )
{
    Locker locker( receiverlock_ );
    return toremove_.isPresent( cber );
}


ThreadID threadID() const
{
    return threadid_;
}

private:

    TypeSet<CallBack>		queue_;

    TypeSet<CallBack>		cbs_;
    ObjectSet<CallBacker>	cbers_;

    ObjectSet<const CallBacker>	toremove_;

    Lock			receiverlock_;
    const ThreadID		threadid_;

};


static Lock cb_rcvrs_lock_;

static ObjectSet<QCallBackEventReceiver>& cbRcvrs()
{
    static ObjectSet<QCallBackEventReceiver>* cbrcvrs = nullptr;
    if ( !cbrcvrs )
	cbrcvrs = new ObjectSet<QCallBackEventReceiver>;
    return *cbrcvrs;
}

static QCallBackEventReceiver* getQCBER( ThreadID threadid )
{
    for (int idx = 0; idx < cbRcvrs().size(); idx++)
    {
	if (cbRcvrs()[idx]->threadID() == threadid )
	    return cbRcvrs()[idx];
    }

    return 0;
}


void CallBacker::createReceiverForCurrentThread()
{
    const ThreadID curthread = Threads::currentThread();

    Locker locker(cb_rcvrs_lock_);

    if ( getQCBER(curthread) )
	return;

    QCallBackEventReceiver* res = new QCallBackEventReceiver(curthread);
    cbRcvrs() += res;
}


void CallBacker::removeReceiverForCurrentThread()
{
    const ThreadID curthreadid = Threads::currentThread();

    Locker locker(cb_rcvrs_lock_);
    for (int idx = 0; idx < cbRcvrs().size(); idx++)
    {
	if ( cbRcvrs()[idx]->threadID() == curthreadid )
	    { delete cbRcvrs().removeSingle(idx); break; }
    }
}


static bool isPresent(const CallBacker* cber)
{
    Locker locker(cb_rcvrs_lock_);
    for (int idx = 0; idx < cbRcvrs().size(); idx++)
    {
	if (cbRcvrs()[idx]->isPresent(cber))
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


void CallBacker::detachAllNotifiers() const
{
    CallBacker* self = const_cast<CallBacker*>( this );
    CallBack::removeFromThreadCalls( self );

    /*Avoid deadlocks (will happen if one thread deletes the notifier while
     the other thread deletes the callbacker at the same time) by using
     try-locks and retry after releasing own lock. */

    Locker lckr( attachednotifierslock_ );

    while ( attachednotifiers_.size() )
    {
	for ( int idx=attachednotifiers_.size()-1; idx>=0; idx-- )
	{
	    NotifierAccess* na = self->attachednotifiers_[idx];
	    if ( na->removeWith( self, false ) &&
		na->removeShutdownSubscription( self, false ) )
		self->attachednotifiers_.removeSingle( idx );
	}

	if ( attachednotifiers_.size() )
	{
	    lckr.unlockNow();
	    Threads::sleep( mOneMilliSecond );
	    lckr.reLock();
	}
    }
}


bool CallBacker::attachCB( const NotifierAccess* notif, const CallBack& cb,
			    bool onlyifnew ) const
{
    return notif
	? attachCB(*notif,cb,onlyifnew)
	: false;
}


bool CallBacker::attachCB( const NotifierAccess& notif, const CallBack& cb,
			  bool onlyifnew ) const
{
    CallBacker* cbobj = const_cast<CallBacker*>( cb.cbObj() );
    if ( cbobj!=this )
    {
	pErrMsg("You can only attach a callback to yourself. "
		"Use triggered.notify( obj )" );
	return false;
    }

    NotifierAccess& worknotif = const_cast<NotifierAccess&>( notif );
    CallBacker* self = const_cast<CallBacker*>( this );
    if ( onlyifnew )
    {
	if ( !worknotif.notifyIfNotNotified( cb ) )
	    return false;
    }
    else
    {
	worknotif.notify( cb );
    }

    //If the notifier is belonging to me, it will only be messy if
    // we subscribe to the shutdown messages.
    if ( notif.cber_!=this )
	worknotif.addShutdownSubscription( self );

    Locker lckr( attachednotifierslock_ );
    if ( !attachednotifiers_.isPresent( &notif ) )
	self->attachednotifiers_ += &worknotif;

    return true;
}


void CallBacker::detachCB( const NotifierAccess& notif,
			   const CallBack& cb ) const
{
    if ( cb.cbObj()!=this )
    {
	pErrMsg( "You can only detach a callback to yourself" );
	return;
    }

    Locker lckr( attachednotifierslock_ );
    if ( !attachednotifiers_.isPresent( &notif ) )
    {
	//It may be deleted. Don't touch it
	return;
    }

    NotifierAccess& enotif = const_cast<NotifierAccess&>( notif );
    enotif.remove( cb );
    CallBacker* self = const_cast<CallBacker*>( this );

    if ( !notif.willCall( this ) )
    {
	while ( attachednotifiers_.isPresent( &notif ) )
	{
	    if ( notif.removeShutdownSubscription( this, false ) )
		self->attachednotifiers_ -= &enotif;
	    else
	    {
		lckr.unlockNow();
		Threads::sleep( mOneMilliSecond );
		lckr.reLock();
	    }
	}
    }
}


bool CallBacker::isNotifierAttached( const NotifierAccess* na ) const
{
    Locker lckr( attachednotifierslock_ );
    return attachednotifiers_.isPresent( na );
}


bool CallBacker::notifyShutdown( const NotifierAccess* na, bool wait ) const
{
    Locker lckr( attachednotifierslock_,
		wait ? Locker::WaitIfLocked : Locker::DontWaitForLock );
    if ( !lckr.isLocked() )
	return false;

    const_cast<CallBacker*>(this)->attachednotifiers_
		    -= const_cast<NotifierAccess*>(na);
    return true;
}


//---- CallBack
ThreadID CallBack::mainthread_ = 0;


bool CallBack::operator==( const CallBack& c ) const
{
    return cberobj_ == c.cberobj_ && fn_ == c.fn_ && sfn_ == c.sfn_;
}


bool CallBack::operator!=( const CallBack& oth ) const
{ return !(*this==oth); }


bool CallBack::willCall() const
{
    return disablecount_ == 0 && ((cberobj_ && fn_) || sfn_);
}


void CallBack::initClass()
{
#ifndef OD_NO_QT
    mainthread_ = Threads::currentThread();
    the_qevent_type = (QEvent::Type)QEvent::registerEventType();
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


bool CallBack::addToThread( ThreadID threadid, const CallBack& cb,
			    CallBacker* cber )
{
    Locker locker(cb_rcvrs_lock_);
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


void CallBack::removeFromThreadCalls( const CallBacker* cber )
{
#ifndef OD_NO_QT
    Locker locker(cb_rcvrs_lock_);
    for (int idx = 0; idx < cbRcvrs().size(); idx++)
	cbRcvrs()[idx]->removeBy(cber);
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
    Locker lckr( cbs.lock_ );
    TypeSet<CallBack>::operator=( cbs );
    return *this;
}


void CallBackSet::doCall( CallBacker* obj )
{
    Locker lckr( lock_ );
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
    Locker lckr( lock_ );
    for ( int idx=0; idx<size(); idx++ )
	(*this)[idx].disable( yn );
}


bool CallBackSet::hasAnyDisabled() const
{
    Locker lckr( lock_ );
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx].isDisabled() )
	    return true;
    return false;
}


void CallBackSet::removeWith( const CallBacker* cbrm )
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


void CallBackSet::transferTo( CallBackSet& to, const CallBacker* only_for,
			      const CallBacker* not_for )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const CallBack& cb = (*this)[idx];
	const CallBacker* cbobj = cb.cbObj();

	if ( only_for && cbobj != only_for )
	    continue;
	if ( not_for && cbobj == not_for )
	    continue;

	if ( !to.isPresent(cb) )
	    to += cb;
	removeSingle( idx );
	idx--;
    }
}


//---- NotifierAccess

NotifierAccess::NotifierAccess( const NotifierAccess& na )
    : cbs_(*new CallBackSet(na.cbs_) )
    , cber_( na.cber_ )
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

    Locker lckr( shutdownsubscriberlock_ );
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


void NotifierAccess::addShutdownSubscription( const CallBacker* cber ) const
{
    Locker lckr( shutdownsubscriberlock_ );
    shutdownsubscribers_.addIfNew( cber );
}


bool NotifierAccess::isShutdownSubscribed( const CallBacker* cber ) const
{
    Locker lckr( shutdownsubscriberlock_ );
    return shutdownsubscribers_.isPresent( cber );
}


bool NotifierAccess::removeShutdownSubscription( const CallBacker* cber,
						 bool wait ) const
{
    Locker lckr( shutdownsubscriberlock_,
		wait ? Locker::WaitIfLocked : Locker::DontWaitForLock );
    if ( !lckr.isLocked() )
	return false;

    shutdownsubscribers_ -= cber;
    return true;
}


void NotifierAccess::notify( const CallBack& cb, bool first ) const
{
    Locker lckr( cbs_.lock_ );

    if ( first )
	cbs_.insert(0,cb);
    else
	cbs_ += cb;
}


bool NotifierAccess::notifyIfNotNotified( const CallBack& cb ) const
{
    Locker lckr( cbs_.lock_ );
    return cbs_.addIfNew( cb );
}


void NotifierAccess::remove( const CallBack& cb ) const
{
    Locker lckr( cbs_.lock_ );

    cbs_ -= cb;
}


bool NotifierAccess::removeWith( const CallBacker* cber, bool wait ) const
{
    Locker lckr( cbs_.lock_,
		wait ? Locker::WaitIfLocked : Locker::DontWaitForLock );
    if ( !lckr.isLocked() )
	return false;

    if ( cber_ == cber )
    {
	cbs_.erase();
	const_cast<NotifierAccess*>(this)->cber_ = 0;
	return true;
    }
    cbs_.removeWith( cber );
    return true;
}


void NotifierAccess::transferCBSTo( const NotifierAccess& oth,
				    const CallBacker* only_for,
				    const CallBacker* not_for ) const
{
    Locker mycbslocker( cbs_.lock_ );
    Locker tocbslocker( oth.cbs_.lock_ );
    const_cast<NotifierAccess*>(this)->cbs_.transferTo(
	    const_cast<NotifierAccess&>(oth).cbs_, only_for, not_for );
}


bool NotifierAccess::willCall( const CallBacker* cber ) const
{
    Locker lckr( cbs_.lock_ );

    for ( int idx=0; idx<cbs_.size(); idx++ )
    {
        if ( cbs_[idx].cbObj()==cber )
            return true;
    }

    return false;
}


void NotifierAccess::doTrigger( CallBackSet& cbs, const CallBacker* cber )
{
    cbs.ref();
    cbs.doCall( const_cast<CallBacker*>(cber) );
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
    Locker locker( thenotif_.cbs_.lock_ );
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
