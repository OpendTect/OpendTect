/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "notify.h"

#include "applicationdata.h"
#include "atomic.h"
#include "ptrman.h"
#include "signal.h"
#include "testprog.h"
#include "thread.h"
#include "threadwork.h"
#include "timer.h"

#include <time.h>


class ClassWithNotifier : public CallBacker
{
public:
				ClassWithNotifier()
				    : notifier( this )
				{}

    Notifier<ClassWithNotifier>	notifier;
};


class NotifiedClass : public CallBacker
{
public:
    NotifiedClass( NotifierAccess* a=nullptr )
	: timer_("starter")
    {
	if ( a )
	    mAttachCB( *a, NotifiedClass::callbackA );
    }

    ~NotifiedClass()
    {
	detachAllNotifiers();
	CallBack::removeFromThreadCalls( this );
    }

    void setTimer()
    {
	retval_ = 1;
	mAttachCB( timer_.tick, NotifiedClass::timerHit );
	timer_.start( 100, true );
    }

    void callbackA( CallBacker* )
    {
	nrhits_++;
    }

    void callbackB( CallBacker* )
    {
	nrhits_--;
    }

    void timerHit( CallBacker* )
    {
	tstStream() << "[OK] Timer hit!" << od_endl;
	retval_ = 0;
	CallBack::addToMainThread( mCB(this,NotifiedClass,closeTesterCB) );
    }

    void closeTesterCB( CallBacker* )
    {
	tstStream() << "[OK] Closing in main thread" << od_endl;
	ApplicationData::exit( retval_ );
    }

    Threads::Atomic<int> nrhits_ = 0;
    Timer	timer_;
    int		retval_ = -1;
};


bool testNormalOp()
{
    NotifiedClass notified;
    ClassWithNotifier notifier;

    notifier.notifier.notifyIfNotNotified(
				mCB(&notified, NotifiedClass,callbackA) );
    notifier.notifier.trigger();
    mRunStandardTest(notified.nrhits_==1,
		     "Normal callback after notifyIfNotNotified" );

    notifier.notifier.notifyIfNotNotified(
			  mCB(&notified, NotifiedClass,callbackA) );
    notifier.notifier.notify( mCB(&notified, NotifiedClass,callbackA) );

    notifier.notifier.trigger();
    mRunStandardTest( notified.nrhits_==3, "Normal callback" );

    notifier.notifier.disable();
    notifier.notifier.trigger();
    mRunStandardTest( notified.nrhits_==3 , "Trigger disabled notifier" );

    notifier.notifier.enable();

    auto* stopper = new NotifyStopper( notifier.notifier );
    notifier.notifier.trigger();
    mRunStandardTest( notified.nrhits_==3,
		     "Notify-stopper on enabled notifier" );
    delete stopper;

    notifier.notifier.trigger();
    mRunStandardTest( notified.nrhits_==5,
		     "Removed notify-stopper on enabled notifier" );

    notifier.notifier.disable();

    stopper = new NotifyStopper( notifier.notifier );
    notifier.notifier.trigger();
    mRunStandardTest( notified.nrhits_==5,
		     "Notify-stopper on disabled notifier" );
    delete stopper;

    notifier.notifier.trigger();
    mRunStandardTest( notified.nrhits_==5,
		     "Removed notify-stopper on disabled notifier" );

    notifier.notifier.enable();
    mRunStandardTest( notifier.notifier.isEnabled(),
		     "No longer disabled notifier" );

    return true;
}


bool testAttach()
{
    {
	auto* notifier = new ClassWithNotifier;
	NotifierAccess* naccess = &notifier->notifier;
	auto* notified = new NotifiedClass( naccess );

	notifier->notifier.trigger();
	mRunStandardTest( notified->nrhits_==1, "Normal attached callback" );

	delete notified;

	mRunStandardTest( !notifier->notifier.isShutdownSubscribed(notified),
			 "Notifier shutdown subscription removal" );

	mRunStandardTest( !naccess->willCall(notified),
			 "Notifier notification removal" );

	notified = new NotifiedClass( naccess );

	notifier->notifier.trigger();

	mRunStandardTest( notified->nrhits_==1, "Normal attached callback 2");

	delete notifier;

	mRunStandardTest( !notified->isNotifierAttached(naccess),
			 "Callbacker notifier removal" );

	delete notified;
    }
    {
	PtrMan<ClassWithNotifier> notifier = new ClassWithNotifier;
	NotifierAccess* naccess = &notifier->notifier;
	PtrMan<NotifiedClass> notified = new NotifiedClass( naccess );

	notified->attachCB( *naccess, mCB(notified,NotifiedClass,callbackA));

	notifier->notifier.trigger();
	mRunStandardTest(notified->nrhits_==2, "Double notifications");

	notified->detachCB( *naccess, mCB(notified,NotifiedClass,callbackA));
	notifier->notifier.trigger();

	mRunStandardTest(notified->nrhits_==3, "Detachement");


	notified->attachCB( *naccess,
			    mCB(notified,NotifiedClass,callbackA), true );

	notifier->notifier.trigger();

	mRunStandardTest(notified->nrhits_==4, "Attach if not yet attached");
    }

    return true;
}


/*!Start an application thread and ask a working thread to call callBackFuncCB.
   This function is protected, so the call should be redirected to main
   thread. Check that the call is done from main thread. If so, success. */

class InMainThreadTester : public CallBacker
{
public:

    static bool	test( InMainThreadTester& tester )
    {
	mRunStandardTest(
	     CallBack::addToMainThread(
				mCB( &tester,InMainThreadTester,eventLoopCB)),
	     "Callback: add to main thread" );;

        return true;
    }

    void eventLoopCB( CallBacker* )
    {
        if ( callingthread_ )
        {
	    tstStream(false) << "Should not be reached" << od_endl;
            ApplicationData::exit( 1 );
	    return;
        }
        else
        {
	    tstStream() << "[OK] Callback: add worker from main thread"
			<< od_endl;
	    mainthread_ = Threads::currentThread();
            Threads::WorkManager::twm().addWork(
                Threads::Work( mCB(this,InMainThreadTester,callBackFuncCB) ),
		nullptr, Threads::WorkManager::cDefaultQueueID(), false, false,
		true );
	}
    }

    ~InMainThreadTester()
    {
        CallBack::removeFromThreadCalls( this );
    }

    void callBackFuncCB(CallBacker*)
    {
	if ( Threads::currentThread() != mainthread_ )
	{
	    tstStream() << "[OK] Callback: initially called from work thread"
			<< od_endl;
	}

        mEnsureExecutedInMainThread( InMainThreadTester::callBackFuncCB );
        if ( callingthread_ )
	{
	    tstStream( false ) << "Should not be reached" << od_endl;
            ApplicationData::exit( 1 );
            return;
	}

	tstStream() << "[OK] Callback: Impl executed in main thread"
		    << od_endl;
        callingthread_ = Threads::currentThread();
    }


    Threads::Atomic<Threads::ThreadID>	mainthread_;
    Threads::Atomic<Threads::ThreadID>	callingthread_;
};


bool testEarlyDetach()
{
    auto* notifier = new ClassWithNotifier;
    NotifierAccess* naccess = &notifier->notifier;
    auto* notified = new NotifiedClass( naccess );

    notified->detachCB( *naccess, mCB(notified,NotifiedClass,callbackA));
    notifier->notifier.trigger();

    mRunStandardTest( notified->nrhits_==0, "Detached callback" );
    mRunStandardTest( !naccess->willCall(notified),
		     "WillCall of detached callback" );
    mRunStandardTest( !notified->isNotifierAttached(naccess),
		     "Notifier removal from attached list" );

    delete notifier;
    delete notified;

    return true;
}


bool testLateDetach()
{
    auto* notifier = new ClassWithNotifier;
    NotifierAccess* naccess = &notifier->notifier;
    auto* notified = new NotifiedClass( naccess );

    delete notifier;
    notified->detachCB( *naccess, mCB(notified,NotifiedClass,callbackA));
    delete notified;

    mRunStandardTest( true, "Detaching deleted notifier" );

    return true;
}


bool testDetachBeforeRemoval()
{
    auto* notifier = new ClassWithNotifier;
    NotifierAccess* naccess = &notifier->notifier;
    auto* notified = new NotifiedClass( naccess );

    notified->detachCB( *naccess, mCB(notified,NotifiedClass,callbackA));
    delete notified;
    delete notifier;

    mRunStandardTest( true, "Detach before removal" );

    return true;
}


#define mNrNotifiers 100

class NotifierOwner : public CallBacker
{
public:

    NotifierOwner()
    {
	thread_ = new Threads::Thread( mCB(this,NotifierOwner,modifyNotifiers),
					"NotifierOwner" );
    }

    ~NotifierOwner()
    {
	lock_.lock();
	stopflag_ = true;
	lock_.unLock();

	if ( thread_ ) thread_->waitForFinish();
	delete thread_;
	deepErase( notifierclasses_ );
    }

    void changeOne()
    {
	lock_.lock();
	const unsigned int idx = pseudoRandom( notifierclasses_.size()-1 );
	delete notifierclasses_.removeSingle( idx );
	notifierclasses_ += new ClassWithNotifier;
	lock_.unLock();
    }

    void trigger()
    {
	lock_.lock();

	for ( int idx=0; idx<notifierclasses_.size(); idx++ )
	    notifierclasses_[idx]->notifier.trigger();

	lock_.unLock();
    }

    void modifyNotifiers(CallBacker*)
    {
	lock_.lock();
	for ( int idx=0; idx<mNrNotifiers; idx++ )
	{
	    notifierclasses_ += new ClassWithNotifier;
	}

	while ( !stopflag_ )
	{
	    lock_.unLock();
	    changeOne();
	    lock_.lock();
	}

	lock_.unLock();
    };

    bool				stopflag_ = false;
    ObjectSet<ClassWithNotifier>	notifierclasses_;
    Threads::SpinLock			lock_;

private:

    unsigned int pseudoRandom( int max )
    {
	seed_ = (8253729 * seed_ + 2396403);
	return seed_ % max;
    }

    unsigned int			seed_ = 5323;
    Threads::Thread*			thread_;
};


class ReceiversOwner : public CallBacker
{
public:
    ReceiversOwner( NotifierOwner& no )
	: notifierowner_(no)
    {
	thread_ = new Threads::Thread(mCB(this,ReceiversOwner,modifyRecievers),
					"ReceiversOwner" );
    }

    ~ReceiversOwner()
    {
	lock_.lock();
	stopflag_ = true;
	lock_.unLock();
	if ( thread_ ) thread_->waitForFinish();
	delete thread_;
	deepErase( receivers_ );
    }

    void stop()
    {
	lock_.lock();
	stopflag_ = true;
	lock_.unLock();
	if ( thread_ ) thread_->waitForFinish();
	deleteAndNullPtr( thread_ );
	deepErase( receivers_ );
    }

    void changeOne()
    {
	lock_.lock();
	const unsigned int idx = pseudoRandom( receivers_.size()-1 );
	delete receivers_.removeSingle( idx );
	receivers_ += createReceiver();
	lock_.unLock();
    }

    void modifyRecievers(CallBacker*)
    {
	lock_.lock();
	for ( int idx=0; idx<mNrNotifiers; idx++ )
	{
	    receivers_ += createReceiver();
	}

	while ( !stopflag_ )
	{
	    lock_.unLock();
	    changeOne();
	    lock_.lock();
	}

	lock_.unLock();
    };

    NotifiedClass* createReceiver()
    {
	auto* res = new NotifiedClass;

	notifierowner_.lock_.lock();

	for ( int idx=0; idx<notifierowner_.notifierclasses_.size(); idx++ )
	{
	    res->attachCB( notifierowner_.notifierclasses_[idx]->notifier,
		           mCB( res, NotifiedClass, callbackA ) );
	    res->attachCB( notifierowner_.notifierclasses_[idx]->notifier,
		           mCB( res, NotifiedClass, callbackB ) );
	}

	notifierowner_.lock_.unLock();

	return res;
    }

    NotifierOwner&			notifierowner_;
    bool				stopflag_ = false;
    ObjectSet<NotifiedClass>		receivers_;
    Threads::SpinLock			lock_;

private:

    unsigned int pseudoRandom( int max )
    {
	seed_ = (8253729 * seed_ + 2396403);
	return seed_ % max;
    }

    unsigned int			seed_ = 1234;
    Threads::Thread*			thread_;
};


bool crashed = false;

void handler(int sig)
{
    od_cout() << "Program crashed\n";
    exit( 1 );
}


bool testMulthThreadChaos()
{
    mRunStandardTest( true, "Multithreaded chaos start" );

    {
	NotifierOwner notifierlist;
	ReceiversOwner receiverslist( notifierlist );

	Threads::sleep( 1 );
	for ( od_int64 idx=0; idx<1000; idx++ )
	{
	    notifierlist.trigger();
	}

	Threads::sleep( 1 );

	receiverslist.stop();
    } //All variables out of scope here

    mRunStandardTest( true, "Multithreaded chaos finished" );
    return true;
}



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testNormalOp()
      || !testAttach()
      || !testLateDetach()
      || !testEarlyDetach()
      || !testDetachBeforeRemoval()
      || !testMulthThreadChaos() )
	return 1;

    ApplicationData ad;
    InMainThreadTester tester;
    if ( !InMainThreadTester::test(tester) )
	return 1;

    NotifiedClass rcvr;
    rcvr.setTimer();
    return ad.exec();
}
