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
	    std::cout << "Data type in test: " << valtype << " \n"; \
	std::cerr << "Atomic = " << atomic.get() << " in function: "; \
	std::cerr << func << " failed!\n"; \
	stopflag = true; \
	return false; \
} \
else \
{ \
	std::cout << "Atomic = " << atomic.get() << " in function: "; \
	std::cerr << func << " passed!\n"; \
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
	std::cout << "Data type in test: " << valtype << " \n";

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


#ifndef __win__
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
    
    count = 10000000;
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
    
    BufferString message("strongSetIfEqual stresstest: nrattempts = ", toString(idx) );
    message += ", successfound=";
    message += toString(successfound);
    message += ", failurefound=";
    message += toString(failurefound);

    if ( !successfound || !failurefound )
	mPrintResult( message.buf() );
    
    stopflag = true;

#endif
    
    if ( !quiet )
	std::cout << "\n";

    return true;
}


#define mRunTestWithType(thetype) \
    if ( !testAtomic<thetype>( " " #thetype " ", quiet ) ) \
	return 1


int main( int narg, char** argv )
{
    SetProgramArgs( narg, argv );

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
    return 0;
}
