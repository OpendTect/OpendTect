/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "thread.h"
#include "genc.h"
#include "keystrs.h"
#include "string2.h"
#include "commandlineparser.h"
#include "callback.h"
#include "limits.h"

#include <iostream>

#define mPrintResult(func) \
{ \
	if ( quiet ) \
        { \
	    std::cout << "\nData type in test: " << valtype; \
	    std::cout << "\n====================\n"; \
	} \
	std::cerr << "Atomic = " << atomic.get() << " in function: "; \
	std::cerr << func << " failed!\n"; \
	stopflag = true; \
	return false; \
} \
else \
{ \
	std::cout << "Atomic = " << atomic.get() << " in function: "; \
	std::cerr << func << " OK\n"; \
}

#define mRunTest( func, finalval ) \
    if ( (func)==false || atomic.get()!=finalval ) \
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
bool testAtomic( const char* valtype, bool quiet )
{
    bool stopflag = false;
    Threads::Atomic<T> atomic( 0 );

    if ( !quiet )
    {
	std::cout << "\nData type in test: " << valtype;
	std::cout << "\n====================\n";
    }

    mRunTest( !atomic.strongSetIfEqual( 1, 2 ), 0 ); //0
    mRunTest( !atomic.strongSetIfEqual( 1, 2 ), 0 ); //0
    mRunTest( atomic.strongSetIfEqual( 1, 0 ), 1 );  //1
    mRunTest( ++atomic==2, 2 ); //2
    mRunTest( atomic++==2, 3 ); //3
    mRunTest( atomic--==3, 2 ); //2
    mRunTest( --atomic==1, 1 ); //1
    mRunTest( (atomic+=2)==3, 3 ); //3
    mRunTest( (atomic-=2)==1, 1 );  //1
    mRunTest( atomic.exchange(2)==1, 2 ); //2

    T expected = 2;
    while ( !atomic.weakSetIfEqual( 1, expected ) ) {}
    if ( atomic.get()!=1 )
	mPrintResult( "weakSetIfEqual" )

    //Let's do some stress-test
    AtomicIncrementer<T> inc1( atomic, stopflag );
    AtomicIncrementer<T> inc2( atomic, stopflag );
    
    Threads::Thread t1( mCB(&inc1,AtomicIncrementer<T>,doRun) );
    Threads::Thread t2( mCB(&inc2,AtomicIncrementer<T>,doRun) );
    
    int count = 10000000;
    bool successfound = false, failurefound = false;
    expected = atomic.get();
    for ( int idx=0; idx<count; idx++ )
    {
	if ( atomic.weakSetIfEqual( mTestVal,expected) )
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
	expected = atomic.get();
	if ( atomic.strongSetIfEqual(mTestVal,expected) )
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
    
    if ( !quiet )
	std::cout << "\n";

    return true;
}

#undef mRunTest

#define mRunTest( desc, test ) \
{ \
    if ( (test) ) \
    { \
	if ( !quiet ) \
	{ \
	    std::cout << desc << ":"; \
	    std::cout << " OK\n"; \
	} \
    } \
    else \
    { \
	std::cout << desc << ":"; \
	std::cout << " Fail\n"; \
	return false; \
    } \
}


/* Locker class that
1. Starts a thread that does a trylock
2. Waits for the new thread has tried to do the trylock.
3. At this point (after the contructor), the res_ variable can be examined
4. The unLockIfLocked() is called, and the other thead unlocks the mutex
5. After unLockIfLocked() is called, the class cannot be used again.
*/

template <class T>
struct Locker : public CallBacker
{
    Locker( T& thelock )
	: lock_( thelock )
	, res_( false )
	, hastried_( false )
	, canunlock_( false )
	, thread_( mCB( this, Locker, tryLock ) )
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
bool testLock( bool quiet, bool testcount, const char* type )
{
    if ( !quiet )
    {
	std::cout << "\n" << type << " tests\n====================\n";
    }

    {
	T lock( false );
	Threads::SpinLock* spinlock = testcount
	    ? (Threads::SpinLock*) &lock
	    : 0;

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
	Locker<T> otherthreadlocker( lock );
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
	Locker<T> otherthreadlocker2( lock );
	mRunTest( "tryLock on locked lock from other thread",
		  !otherthreadlocker2.res_ );
	otherthreadlocker2.unLockIfLocked();
	lock.unLock();
	//No lock
    }


    {
	T rlock( true );
	Threads::SpinLock* spinlock = testcount
	    ? (Threads::SpinLock*) &rlock
	    : 0;

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
	Locker<T> otherthreadlocker( rlock );
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
	Locker<T> otherthreadlocker2( rlock );
	mRunTest( "tryLock on locked lock from other thread",
		  !otherthreadlocker2.res_ );
	otherthreadlocker2.unLockIfLocked();
	rlock.unLock();
    }

    return true;
}


#define mRunTestWithType(thetype) \
    if ( !testAtomic<thetype>( " " #thetype " ", quiet ) ) \
	ExitProgram( 1 );


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );

    CommandLineParser parser;
    const bool quiet = parser.hasKey( sKey::Quiet() );

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

    if ( !testLock<Threads::Mutex>( quiet, false, "Mutex" ) )
	ExitProgram( 1 );

    if ( !testLock<Threads::SpinLock>( quiet, true, "SpinLock" ) )
	ExitProgram( 1 );

    ExitProgram( 0 );
}
