/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id: svnversion.cc 28757 2013-03-07 10:50:10Z kristofer.tingdahl@dgbes.com $";

#include "thread.h"
#include "genc.h"
#include "keystrs.h"
#include "commandlineparser.h"

#include <iostream>

#define mRunTest( val,func, finalval ) \
    if ( (func)==false || val!=finalval ) \
    { \
	if ( quiet ) \
	    std::cout << "Data type in test: " << valtype << " \n"; \
	std::cerr << "Atomic = " << val << " in function: "; \
	std::cerr << #func " failed!\n"; \
	return false; \
    } \
    else if ( !quiet ) \
    { \
	std::cout << "Atomic = " << val << " in function: "; \
	std::cerr << #func " passed!\n"; \
    }


template <class T>
bool testAtomic( const char* valtype, bool quiet )
{
    Threads::Atomic<T> atomic( 0 );

    if ( !quiet )
	std::cout << "Data type in test: " << valtype << " \n";

    mRunTest( atomic.get(),!atomic.setIfEqual( 1, 2 ), 0 ); //0
    mRunTest( atomic.get(),!atomic.setIfEqual( 1, 2 ), 0 ); //0
    mRunTest( atomic.get(),atomic.setIfEqual( 1, 0 ), 1 );  //1
    mRunTest( atomic.get(), ++atomic==2, 2 ); //2
    mRunTest( atomic.get(), atomic++==2, 3 ); //3
    mRunTest( atomic.get(), atomic--==3, 2 ); //2
    mRunTest( atomic.get(), --atomic==1, 1 ); //1
    mRunTest( atomic.get(), (atomic+=2)==3, 3 ); //3
    mRunTest( atomic.get(), (atomic-=2)==1, 1 );  //1

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
    const bool quiet = parser.hasKey( sKey::quiet() );

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
