/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "testprog.h"

#include "atomic.h"
#include "callback.h"
#include "limits.h"
#include "thread.h"


#define mPrintResult(func) \
{ \
    if ( !quiet_ ) \
    { \
	od_cout() << "\nData type in test: " << valtype; \
	od_cout() << "\n====================\n"; \
	od_cout() << "Atomic = " << atomic.load() << " in function: "; \
    } \
    od_cout() << func << " failed!\n"; \
    stopflag = true; \
    return false; \
} \
else \
{ \
    if ( !quiet_ ) \
    { \
	od_cout() << "Atomic = " << atomic.load() << " in function: "; \
	od_cout() << func << " OK\n"; \
    } \
}

#undef mRunTest
#define mRunTest( func, finalval ) \
    if ( (func)==false || atomic.load()!=finalval ) \
	mPrintResult( #func )

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

    if ( !quiet_ )
    {
	od_cout() << "\nData type in test: " << valtype;
	od_cout() << "\n====================\n";
    }

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

    if ( !successfound || !failurefound )
	mPrintResult( "weakSetIfEqual stresstest");

    count = 1000000000;
    successfound = false;
    failurefound = false;
    int idx;
    for ( idx=0; idx<count; idx++ )
    {
	curval = atomic.load();
	if ( atomic.setIfValueIs(curval,mTestVal,&curval ) )
	    successfound = true;
	else
	    failurefound = true;

	if ( successfound && failurefound )
	    break;
    }

    BufferString message("strongSetIfEqual stresstest: nrattempts = ",
			 toString(idx) );
    message += ", successfound=";
    message += toString(successfound);
    message += ", failurefound=";
    message += toString(failurefound);

    if ( !successfound || !failurefound )
	mPrintResult( message.buf() );

    stopflag = true;

    if ( !quiet_ )
	od_cout() << "\n";

    return true;
}




#undef mRunTest

#define mRunTest( desc, test ) \
{ \
    if ( (test) ) \
    { \
	if ( !quiet_ ) \
	{ \
	    od_cout() << desc << ":"; \
	    od_cout() << " OK\n"; \
	} \
    } \
    else \
    { \
	od_cout() << desc << ":"; \
	od_cout() << " Fail\n"; \
	return false; \
    } \
}

bool testAtomicSetIfValueIs()
{
    volatile int val = 0;

    int curval = 1;
    mRunTest( "atomicSetIfValueIs failure",
	      !Threads::atomicSetIfValueIs( val, curval, 1, &curval )
	      && curval == 0 )

    curval = 0;
    mRunTest( "atomicSetIfValueIs success",
	      Threads::atomicSetIfValueIs( val, curval, 1, &curval )
	      && curval == 0 )

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
    if ( !quiet_ )
    {
	od_cout() << "\n" << type << " tests\n====================\n";
    }

    {
	T lock( false );
	mDynamicCastGet( Threads::SpinLock*, spinlock, testcount ? &lock : 0 );

	if ( spinlock )
	    mRunTest( "Inital count", spinlock->count()==0 );
	mRunTest( "tryLock on unlocked lock", lock.tryLock() );
	if ( spinlock )
	    mRunTest( "Locked count", spinlock->count()==1 );
	mRunTest( "tryLock on locked lock", !lock.tryLock() );
	if ( spinlock )
	    mRunTest( "Locked count after lock attempt", spinlock->count()==1 );
	lock.unLock();

	//No lock
	mRunTest( "tryLock on unlocked lock (2)", lock.tryLock() );
	lock.unLock();

	//No lock
	LockerTester<T> otherthreadlocker( lock );
	mRunTest( "tryLock on unlocked lock from other thread",
		  otherthreadlocker.res_ );
	mRunTest( "tryLock on lock that is locked in other thread",
		  !lock.tryLock() );
	otherthreadlocker.unLockIfLocked();

	//No lock
	lock.lock();
	mRunTest( "tryLock on locked lock (2)", !lock.tryLock() );
	lock.unLock();

	lock.lock();
	LockerTester<T> otherthreadlocker2( lock );
	mRunTest( "tryLock on locked lock from other thread",
		  !otherthreadlocker2.res_ );
	otherthreadlocker2.unLockIfLocked();
	lock.unLock();
	//No lock
    }


    {
	T rlock( true );

	mDynamicCastGet( Threads::SpinLock*, spinlock,
			 testcount ? &rlock : 0 );

	if ( spinlock )
	    mRunTest( "Inital count", spinlock->count()==0 );
	mRunTest( "tryLock on unlocked recursive lock", rlock.tryLock() );
	if ( spinlock )
	    mRunTest( "Locked count on single locked recursive lock",
		  spinlock->count()==1 );
	mRunTest( "tryLock on locked recursive lock", rlock.tryLock() );
	if ( spinlock )
	    mRunTest( "Locked count on double locked recursive lock",
		  spinlock->count()==2 );
	rlock.unLock();
	rlock.unLock();
	mRunTest( "tryLock on unlocked recursive lock (2)", rlock.tryLock() );
	rlock.unLock();

	//No lock
	LockerTester<T> otherthreadlocker( rlock );
	mRunTest( "tryLock on unlocked lock from other thread",
		  otherthreadlocker.res_ );
	mRunTest( "tryLock on lock that is locked in other thread",
		  !rlock.tryLock() );
	otherthreadlocker.unLockIfLocked();

	//No lock
	rlock.lock();
	mRunTest( "tryLock on locked recursive lock (2)", rlock.tryLock() );
	rlock.unLock();
	rlock.unLock();

	rlock.lock();
	LockerTester<T> otherthreadlocker2( rlock );
	mRunTest( "tryLock on locked lock from other thread",
		  !otherthreadlocker2.res_ );
	otherthreadlocker2.unLockIfLocked();
	rlock.unLock();
    }

    return true;
}


bool testSimpleSpinLock()
{
    volatile int lock = 0;
    Threads::lockSimpleSpinLock( lock, Threads::Locker::WaitIfLocked );

    mRunTest( "Simple spinlock acquire lock", (lock==1));
    mRunTest( "Simple spinlock trylock on locked lock.",
        !Threads::lockSimpleSpinLock( lock, Threads::Locker::DontWaitForLock ));

    Threads::unlockSimpleSpinLock( lock );
    mRunTest( "Simple spinlock release lock", lock==0 );

    mRunTest( "Simple spinlock trylock on unlocked lock.",
         Threads::lockSimpleSpinLock( lock, Threads::Locker::DontWaitForLock ));

    return true;
}

bool testConditionVarTimeout()
{
    Threads::ConditionVar condvar;

    Threads::MutexLocker locker( condvar );

    mRunStandardTest( !condvar.wait( 20 ), "Condition variable timeout" );

    return true;
}


#define mRunTestWithType(thetype) \
    if ( !testAtomic<thetype>( " " #thetype " " ) ) \
	return 1;


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    mRunTestWithType(od_int64);
    mRunTestWithType(od_uint64);
    mRunTestWithType(od_int32);
    mRunTestWithType(od_uint32);
    mRunTestWithType(od_int16);
    mRunTestWithType(od_uint16);
    mRunTestWithType(long long);
    mRunTestWithType(unsigned long long );
    mRunTestWithType(long);
    mRunTestWithType(unsigned long);
    mRunTestWithType(int);
    mRunTestWithType(unsigned int);
    mRunTestWithType(short);
    mRunTestWithType(unsigned short);

    if ( !testAtomicSetIfValueIs()
      || !testAtomicPointer()
      || !testSimpleSpinLock()
      || !testConditionVarTimeout()
      || !testLock<Threads::Mutex>( false, "Mutex" )
      || !testLock<Threads::SpinLock>( true, "SpinLock" ) )
	return 1;

    return 0;
}
