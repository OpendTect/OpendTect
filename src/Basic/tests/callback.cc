/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2013
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "callback.h"
#include "commandlineparser.h"
#include "keystrs.h"
#include "signal.h"
#include "debug.h"

#include <iostream>
#include <time.h>


class ClassWithNotifier : public CallBacker
{
public:
				ClassWithNotifier() : notifier( this ) {} 
    Notifier<ClassWithNotifier>	notifier;
};


class NotifiedClass : public CallBacker
{
public:
			NotifiedClass(NotifierAccess* a = 0 ) : nrhits_( 0 )
			{
			    if ( a ) mAttachCB( *a, NotifiedClass::callbackA);
			}
			~NotifiedClass() { detachAllNotifiers(); }
    void 		callbackA(CallBacker*)
			{
			    nrhits_++;
			}
    void 		callbackB(CallBacker*)
			{
			    nrhits_--;
			}
    
    Threads::Atomic<int>	nrhits_;
};

#define mCheckTest( testname, condition ) \
if ( !(condition) ) \
{ \
    std::cout << testname << ": Failed\n"; \
    return false; \
} \
else if ( !quiet ) \
{ \
    std::cout << testname << ": Pass\n"; \
}


bool testNormalOp( bool quiet )
{
    NotifiedClass notified;
    ClassWithNotifier notifier;
    notifier.notifier.notify( mCB(&notified, NotifiedClass,callbackA) );
    
    notifier.notifier.trigger();
    mCheckTest( "Normal callback", notified.nrhits_==1 );
    
    mCheckTest( "Return value of disable call", notifier.notifier.disable() );
    mCheckTest( "Return value of disable call on disabled notifier",
	       	!notifier.notifier.disable() );
    notifier.notifier.trigger();
    mCheckTest( "Trigger disabled notifier", notified.nrhits_==1 );
    
    mCheckTest( "Return value of enable call on disabled notifier",
	       !notifier.notifier.enable() );
    
    NotifyStopper* stopper = new NotifyStopper( notifier.notifier );
    notifier.notifier.trigger();
    mCheckTest( "Notify-stopper on enabled notifier", notified.nrhits_==1 );
    delete stopper;
    
    notifier.notifier.trigger();
    mCheckTest( "Removed notify-stopper on enabled notifier",
	        notified.nrhits_==2 );
    
    notifier.notifier.disable();
    
    stopper = new NotifyStopper( notifier.notifier );
    notifier.notifier.trigger();
    mCheckTest( "Notify-stopper on disabled notifier", notified.nrhits_==2 );
    delete stopper;
    
    notifier.notifier.trigger();
    mCheckTest( "Removed notify-stopper on disabled notifier",
	       notified.nrhits_==2 );
    
    return true;
}


bool testAttach( bool quiet )
{
    ClassWithNotifier* notifier = new ClassWithNotifier;
    NotifierAccess* naccess = &notifier->notifier;
    NotifiedClass* notified = new NotifiedClass( naccess );
    
    notifier->notifier.trigger();
    mCheckTest( "Normal attached callback", notified->nrhits_==1);
    
    delete notified;
    
    mCheckTest( "Notifier shutdown subscription removal",
	       !notifier->notifier.isShutdownSubscribed(notified) );
    
    mCheckTest( "Notifier notification removal",
	       !naccess->willCall(notified) );
    
    notified = new NotifiedClass( naccess );
    
    notifier->notifier.trigger();
    
    mCheckTest( "Normal attached callback 2", notified->nrhits_==1);
    
    delete notifier;
    
    mCheckTest( "Callbacker notifier removal",
	       !notified->isNotifierAttached(naccess) );
    
    delete notified;
    
    return true;
}


bool testDetach( bool quiet )
{
    ClassWithNotifier* notifier = new ClassWithNotifier;
    NotifierAccess* naccess = &notifier->notifier;
    NotifiedClass* notified = new NotifiedClass( naccess );

    notified->detachCB( *naccess, mCB(notified,NotifiedClass,callbackA));
    notifier->notifier.trigger();

    mCheckTest( "Detached callback", notified->nrhits_==0);
    mCheckTest( "WillCall of detached callback",
	       !naccess->willCall(notified) );
    mCheckTest( "Notifier removal from attached list",
	       !notified->isNotifierAttached(naccess) );

    delete notifier;
    delete notified;
    
    return true;
}


#define mNrNotifiers 100

class NotifierOwner : public CallBacker
{
public:
    NotifierOwner()
	: stopflag_( false ), seed_( 5323 )
    {
	thread_ = new Threads::Thread( mCB(this,NotifierOwner,modifyNotifiers));
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

    bool				stopflag_;
    ObjectSet<ClassWithNotifier>	notifierclasses_;
    Threads::SpinLock			lock_;

private:

    unsigned int pseudoRandom( int max )
    {
	seed_ = (8253729 * seed_ + 2396403);
	return seed_ % max;
    }

    unsigned int			seed_;
    Threads::Thread*			thread_;
};


class ReceiversOwner : public CallBacker
{
public:
    ReceiversOwner( NotifierOwner& no )
	: stopflag_( false ), seed_( 1234 ), notifierowner_( no )
    {
	thread_ = new Threads::Thread(mCB(this,ReceiversOwner,modifyRecievers));
    }

    ~ReceiversOwner()
    {
	stop();
    }

    void stop()
    {
	lock_.lock();
	stopflag_ = true;
	lock_.unLock();
	if ( thread_ ) thread_->waitForFinish();
	delete thread_;
	thread_ = 0;
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

    NotifiedClass*			createReceiver()
    {
	NotifiedClass* res = new NotifiedClass;

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
    bool				stopflag_;
    ObjectSet<NotifiedClass>		receivers_;
    Threads::SpinLock			lock_;


private:

    unsigned int pseudoRandom( int max )
    {
	seed_ = (8253729 * seed_ + 2396403);
	return seed_ % max;
    }

    unsigned int			seed_;
    Threads::Thread*			thread_;
};


bool crashed = false;

void handler(int sig)
{
    std::cout << "Program crashed\n";
    exit( 1 );
}


bool testMulthThreadChaos( bool quiet )
{
    std::cout << "Multithreaded chaos:";
    std::cout.flush();

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

    std::cout << " Pass\n";
    return true;
}


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );

    CommandLineParser clparser;
    const bool quiet = clparser.hasKey( sKey::Quiet() );
    
    if ( !testNormalOp( quiet ) || !testAttach( quiet ) ||
	 !testDetach( quiet ) || !testMulthThreadChaos( quiet ) )
	ExitProgram( 1 );
    
    ExitProgram( 0 );
}
