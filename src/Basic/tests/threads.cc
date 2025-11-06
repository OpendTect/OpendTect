/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "atomic.h"
#include "callback.h"
#include "limits.h"
#include "thread.h"




#define mSetDesc( finalval , func ) \
    desc.set( "Atomic = " ).add( finalval ) \
	.add( " in function: " ).add( func );
#define mSetErrMsg( finalval, val ) \
    errmsg.set( "Expected: " ).add( finalval ) \
	  .add( ", received: " ).add( val );  \

#define mRunTest( func, finalval ) \
    res = func; \
    val = atomic.load(); \
    mSetDesc( finalval, #func ) \
    mSetErrMsg( finalval, val ) \
    mRunStandardTestWithError( res && val==finalval, \
			       desc.str(), errmsg.str() );

#define mTestVal 100

template <class T>
class AtomicIncrementer : public CallBacker
{
public:
			AtomicIncrementer( Threads::Atomic<T>& val,
					   const bool& stopflag )
			    : val_( val )
			    , stopflag_( stopflag )
			{}

    void		doRun(CallBacker*)
			{
			    while ( !stopflag_ )
				val_++;
			}
protected:

    Threads::Atomic<T>&	val_;
    const bool&		stopflag_;
};


template <class T>
bool testAtomic( const char* valtype )
{
    bool stopflag = false;
    Threads::Atomic<T> atomic( 0 );

    logStream() << od_newline << "Data type in test: " << valtype
		<< od_newline << "=====================" << od_endl;

    BufferString desc, errmsg;
    bool res;
    T val;

    T curval = 2;
    mRunTest( !atomic.setIfValueIs( 2, 1, &curval ) && curval==0, 0 ); //0
    curval = 2;
    mRunTest( !atomic.setIfValueIs( 2, 1, &curval ) && curval==0, 0 ); //0
    curval = 0;
    mRunTest( atomic.setIfValueIs( 0, 1 , &curval) && curval==0, 1 );  //1
    mRunTest( ++atomic==2, 2 ); //2
    mRunTest( atomic++==2, 3 ); //3
    mRunTest( atomic--==3, 2 ); //2
    mRunTest( --atomic==1, 1 ); //1
    mRunTest( (atomic+=2)==3, 3 ); //3
    mRunTest( (atomic-=2)==1, 1 );  //1
    mRunTest( atomic.exchange(2)==1, 2 ); //2

    //Let's do some stress-test
    AtomicIncrementer<T> inc1( atomic, stopflag );
    AtomicIncrementer<T> inc2( atomic, stopflag );

    Threads::Thread t1( mCB(&inc1,AtomicIncrementer<T>,doRun), "t1" );
    Threads::Thread t2( mCB(&inc2,AtomicIncrementer<T>,doRun), "t2" );

    int count = 10000000;
    bool successfound = false, failurefound = false;
    curval = atomic.load();
    for ( int idx=0; idx<count; idx++ )
    {
	if ( atomic.setIfValueIs( curval, mTestVal, &curval ) )
	    successfound = true;
	else
	    failurefound = true;

	if ( successfound && failurefound )
	    break;
    }

    mSetDesc( mTestVal, "weakSetIfEqual stress test" )
    mRunStandardTest( successfound && failurefound, desc.str() );

    count = 1000000000;
    successfound = false;
    failurefound = false;
    int idx;
    for ( idx=0; idx<count; idx++ )
    {
	curval = atomic.load();
	if ( atomic.setIfValueIs(curval,mTestVal,&curval) )
	    successfound = true;
	else
	    failurefound = true;

	if ( successfound && failurefound )
	    break;
    }

    BufferString message("nrattempts = ", idx );
    message.add( ", successfound=" ).add( successfound )
	   .add( ", failurefound=" ).add( failurefound );

    mSetDesc( atomic.load(), "strongSetIfEqual stress test" )
    mRunStandardTestWithError( successfound && failurefound,
			       desc.str(), message.str() );
    stopflag = true;

    return true;
}


#define mRunTestWithType(thetype) \
    if ( !testAtomic<thetype>( " " #thetype " " ) ) \
	return false; \

bool testAtomicWithType()
{
    mRunTestWithType(od_int64);
    mRunTestWithType(od_uint64);
    mRunTestWithType(od_int32);
    mRunTestWithType(od_uint32);
    mRunTestWithType(od_int16);
    mRunTestWithType(od_uint16);
    /*mRunTestWithType(long long);
    mRunTestWithType(unsigned long long);
    mRunTestWithType(long);
    mRunTestWithType(unsigned long);*/
    mRunTestWithType(int);
    mRunTestWithType(unsigned int);
    mRunTestWithType(short);
    mRunTestWithType(unsigned short);

    return true;
}


bool testAtomicSetIfValueIs()
{
    volatile int val = 0;

    logStream() << od_newline;

    int curval = 1;
    mRunStandardTest(
	!Threads::atomicSetIfValueIs( val, curval, 1, &curval ) && curval == 0,
	"atomicSetIfValueIs shoud fail" );

    curval = 0;
    mRunStandardTest(
	Threads::atomicSetIfValueIs( val, curval, 1, &curval ) && curval == 0,
	"atomicSetIfValueIs success" );

    return true;
}


bool testAtomicPointer()
{
    Threads::AtomicPointer<void> curthread;

    curthread = Threads::currentThread();
    mRunStandardTest(curthread == Threads::currentThread(),
		     "Atomic Pointer assignment");

    return true;
}


/* Locker class that
1. Starts a thread that does a trylock
2. Waits for the new thread has tried to do the trylock.
3. At this point (after the contructor), the res_ variable can be examined
4. The unLockIfLocked() is called, and the other thead unlocks the mutex
5. After unLockIfLocked() is called, the class cannot be used again.
*/

template <class T>
struct LockerTester : public CallBacker
{
    LockerTester( T& thelock )
	: lock_( thelock )
	, res_( false )
	, hastried_( false )
	, canunlock_( false )
	, thread_( mCB( this, LockerTester, tryLock ), "locktest thread" )
    {
	hastriedlock_.lock();
	while ( !hastried_ )
	    hastriedlock_.wait();
	hastriedlock_.unLock();
    }

    void unLockIfLocked()
    {
	canunlocklock_.lock();
	canunlock_ = true;
	canunlocklock_.signal( true );
	canunlocklock_.unLock();

	thread_.waitForFinish();
    }

    void tryLock(CallBacker*)
    {
	res_ = lock_.tryLock();
	hastriedlock_.lock();
	hastried_ = true;
	hastriedlock_.signal( true );
	hastriedlock_.unLock();

	canunlocklock_.lock();
	while ( !canunlock_ )
	    canunlocklock_.wait();

	canunlocklock_.unLock();
	if ( res_ )
	    lock_.unLock();
    }

    bool			res_;
    bool			hastried_;
    bool			canunlock_;
    T&				lock_;
    Threads::ConditionVar	hastriedlock_;
    Threads::ConditionVar	canunlocklock_;

				//Must be at bottom
    Threads::Thread		thread_;
};



template <class T> inline
bool testLock( bool testcount, const char* type )
{
    logStream() << od_newline << type << " tests" << od_newline
		<< "===================" << od_endl;

    {
	T lock( false );
	mDynamicCastGet( Threads::SpinLock*, spinlock, testcount ? &lock : 0 );

	if ( spinlock )
	    mRunStandardTest( spinlock->count()==0, "Inital count" );

	mRunStandardTest( lock.tryLock(), "tryLock on unlocked lock" );
	if ( spinlock )
	    mRunStandardTest( spinlock->count()==1, "Locked count" );

	mRunStandardTest( !lock.tryLock(), "tryLock on locked lock" );
	if ( spinlock )
	    mRunStandardTest( spinlock->count()==1,
			      "Locked count after lock attempt" );

	lock.unLock();

	//No lock
	mRunStandardTest( lock.tryLock(), "tryLock on unlocked lock (2)" );
	lock.unLock();

	//No lock
	LockerTester<T> otherthreadlocker( lock );
	mRunStandardTest( otherthreadlocker.res_,
			  "tryLock on unlocked lock from other thread" );
	mRunStandardTest( !lock.tryLock(),
			  "tryLock on lock that is locked in other thread" );
	otherthreadlocker.unLockIfLocked();

	//No lock
	lock.lock();
	mRunStandardTest( !lock.tryLock(), "tryLock on locked lock (2)" );
	lock.unLock();

	lock.lock();
	LockerTester<T> otherthreadlocker2( lock );
	mRunStandardTest( !otherthreadlocker2.res_,
			  "tryLock on locked lock from other thread" );
	otherthreadlocker2.unLockIfLocked();
	lock.unLock();
	//No lock
    }

    {
	T rlock( true );

	mDynamicCastGet( Threads::SpinLock*, spinlock,
			 testcount ? &rlock : 0 );

	if ( spinlock )
	    mRunStandardTest( spinlock->count()==0, "Inital count" );

	mRunStandardTest( rlock.tryLock(),
			  "tryLock on unlocked recursive lock" );
	if ( spinlock )
	    mRunStandardTest( spinlock->count()==1,
			      "Locked count on single locked recursive lock" );

	mRunStandardTest( rlock.tryLock(),
			  "tryLock on locked recursive lock" );
	if ( spinlock )
	    mRunStandardTest( spinlock->count()==2,
			      "Locked count on double locked recursive lock" );

	rlock.unLock();
	rlock.unLock();
	mRunStandardTest( rlock.tryLock(),
			  "tryLock on unlocked recursive lock (2)" );
	rlock.unLock();

	//No lock
	LockerTester<T> otherthreadlocker( rlock );
	mRunStandardTest( otherthreadlocker.res_,
			  "tryLock on unlocked lock from other thread" );
	mRunStandardTest( !rlock.tryLock(),
			  "tryLock on lock that is locked in other thread" );
	otherthreadlocker.unLockIfLocked();

	//No lock
	rlock.lock();
	mRunStandardTest( rlock.tryLock(),
			  "tryLock on locked recursive lock (2)" );
	rlock.unLock();
	rlock.unLock();

	rlock.lock();
	LockerTester<T> otherthreadlocker2( rlock );
	mRunStandardTest( !otherthreadlocker2.res_,
			  "tryLock on locked lock from other thread" );
	otherthreadlocker2.unLockIfLocked();
	rlock.unLock();
    }

    return true;
}


bool testSimpleSpinLock()
{
    volatile int lock = 0;
    Threads::lockSimpleSpinLock( lock, Threads::Locker::WaitIfLocked );

    mRunStandardTest( lock==1, "Simple spinlock acquire lock" );
    mRunStandardTest(
	!Threads::lockSimpleSpinLock( lock, Threads::Locker::DontWaitForLock ),
	"Simple spinlock trylock on locked lock" );

    Threads::unlockSimpleSpinLock( lock );
    mRunStandardTest( lock==0, "Simple spinlock release lock" );

    mRunStandardTest(
	Threads::lockSimpleSpinLock( lock, Threads::Locker::DontWaitForLock ),
	"Simple spinlock trylock on unlocked lock." );

    return true;
}


bool testConditionVarTimeout()
{
    Threads::ConditionVar condvar;

    Threads::MutexLocker locker( condvar );
    mRunStandardTest( !condvar.wait( 20 ), "Condition variable timeout" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testAtomicWithType()
      || !testAtomicSetIfValueIs()
      || !testAtomicPointer()
      || !testSimpleSpinLock()
      || !testConditionVarTimeout()
      || !testLock<Threads::Mutex>(false,"Mutex")
      || !testLock<Threads::SpinLock>(true,"SpinLock") )
	return 1;

    return 0;
}
