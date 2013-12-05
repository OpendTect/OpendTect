/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION : 
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "commandlineparser.h"
#include "keystrs.h"

#include "od_iostream.h"

#define mTest( testname, test ) \
if ( (test)==true ) \
{ \
    if ( !quiet ) \
    { \
        od_ostream::logStream() << testname << ": OK\n"; \
    } \
} \
else \
{ \
    od_ostream::logStream() << testname << ": Failed\n"; \
    return false; \
}


bool testPointerCast( bool quiet )
{
    float val;
    float* ptr = &val;

    void* voidptr = ptr;

    float* newptr = mCastPtr(float,voidptr);

    mTest( "Pointer cast", newptr==ptr );

    return true;
}


bool testPointerAlignment( bool quiet )
{
    char buffer[] = { 0, 0, 0, 0, 1, 1, 1, 1 };

    char* ptr0 = buffer;
    char* ptr1 = buffer+1;

    od_uint32* iptr0 = mCastPtr( od_uint32, ptr0 );
    od_uint32* iptr1 = mCastPtr( od_uint32, ptr1 );

    od_uint32 ival0 = *iptr0;
    od_uint32 ival1 = *iptr1;

    char* ptr0_mod = (char*) iptr0;
    char* ptr1_mod = (char*) iptr1;

    union IVal1Reference
    {
    public:
			IVal1Reference()
        		{
                            charval_[0] = 0;
                            charval_[1] = 0;
                            charval_[2] = 0;
                            charval_[3] = 1;
                        }
        char		charval_[4];
        int		intval_;
    }  val1ref;
    
    mTest("Pointer alignment", ptr0_mod==ptr0 && ptr1_mod==ptr1 &&
                               ival0==0 && ival1 == val1ref.intval_ );

    return true;
}


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );
    const bool quiet = CommandLineParser().hasKey( sKey::Quiet() );

    // Main idea is to test things that are so basic they don't
    // really fit anywhere else.

    if ( !testPointerCast(quiet) )
	ExitProgram(1);


    if ( !testPointerAlignment(quiet) )
        ExitProgram( 1 );

    ExitProgram( 0 );
}
