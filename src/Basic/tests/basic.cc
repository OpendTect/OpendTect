/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "testprog.h"

#include "multiid.h"


#define mTest( testname, test ) \
if ( (test)==true ) \
{ \
    if ( !quiet ) \
    { \
        od_cout() << testname << ": OK\n"; \
    } \
} \
else \
{ \
    od_cout() << testname << ": Failed\n"; \
    return false; \
}


bool testPointerCast()
{
    float val;
    float* ptr = &val;

    void* voidptr = ptr;

    float* newptr = reinterpret_cast<float*>(voidptr);

    mTest( "Pointer cast", newptr==ptr );

    return true;
}


bool testPointerAlignment()
{
    char buffer[] = { 0, 0, 0, 0, 1, 1, 1, 1 };

    char* ptr0 = buffer;
    char* ptr1 = buffer+1;

    od_uint32* iptr0 = reinterpret_cast<od_uint32*>( ptr0 );
    od_uint32* iptr1 = reinterpret_cast<od_uint32*>( ptr1 );

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


bool testCompoundKey()
{
    mRunStandardTest( MultiID::udf().isUdf(), "Undefined multiid" );

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    // Main idea is to test things that are so basic they don't
    // really fit anywhere else.

    if ( !testPointerCast()
	|| !testCompoundKey()
        || !testPointerAlignment() )
        ExitProgram( 1 );

    return ExitProgram( 0 );
}
