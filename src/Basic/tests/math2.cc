/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2013
 * FUNCTION :
-*/


#include "thread.h"
#include "testprog.h"
#include "math2.h"
#include "genc.h"


#define mRunTest( func ) \
    if ( (func)==false ) \
    { \
	handleTestResult( false, #func ); \
	return false; \
    } \
    else \
    { \
	handleTestResult( true, #func ); \
    }

#define mTestVal 100


bool testBits()
{
    mRunTest( Math::SetBits(0x0000, 0x0001, false )==0x0000 )
    mRunTest( Math::SetBits(0x0000, 0x0001, true )==0x0001 )
    mRunTest( Math::SetBits(0x0000, 0x0200, false )==0x0000 )
    mRunTest( Math::SetBits(0x0000, 0x0200, true )==0x0200 )
    mRunTest( Math::SetBits(0x0020, 0x0001, false )==0x0020 )
    mRunTest( Math::SetBits(0x0020, 0x0001, true )==0x0021 )
    mRunTest( Math::SetBits(0x0020, 0x0200, false )==0x0020 )
    mRunTest( Math::SetBits(0x0020, 0x0200, true )==0x0220 )
    mRunTest( Math::SetBits(0x0021, 0x0001, false )==0x0020 )
    mRunTest( Math::SetBits(0x0021, 0x0001, true )==0x0021 )
    mRunTest( Math::SetBits(0x0220, 0x0200, false )==0x0020 )
    mRunTest( Math::SetBits(0x0220, 0x0200, true )==0x0220 )

    mRunTest( Math::AreBitsSet(0x0220, 0x0200, true )==true )
    mRunTest( Math::AreBitsSet(0x0220, 0x0200, false )==true )
    mRunTest( Math::AreBitsSet(0x0220, 0x0220, true )==true )
    mRunTest( Math::AreBitsSet(0x0200, 0x0220, true )==false )
    mRunTest( Math::AreBitsSet(0x0200, 0x0220, false )==true )
    mRunTest( Math::AreBitsSet(0x0220, 0x0002, true )==false )
    mRunTest( Math::AreBitsSet(0x0220, 0x0002, false )==false )

    return true;
}


bool testNiceNumber()
{
    mRunStandardTest(Math::NiceNumber(0.f)==0.f, "NiceNumber(0.f)");
    mRunStandardTest(Math::NiceNumber(14.f)==10.f, "NiceNumber(14.f)");
    mRunStandardTest(Math::NiceNumber(-14.f)==-10.f, "NiceNumber(-14.f)");
    mRunStandardTest(Math::NiceNumber(14.f, false)==20.f,
						     "NiceNumber(14.f, false)");
    mRunStandardTest(Math::NiceNumber(-14.f, false)==-20.f,
						 "NiceNumber(-14.f, false)");
    mRunStandardTest(Math::NiceNumber(0.14f)==0.1f, "NiceNumber(0.14f)");
    mRunStandardTest(Math::NiceNumber(-0.14f)==-0.1f, "NiceNumber(-0.14f)");
    mRunStandardTest(Math::NiceNumber(0.14f, false)==0.2f,
						 "NiceNumber(0.14f, false)");
    mRunStandardTest(Math::NiceNumber(-0.14f, false)==-0.2f,
						 "NiceNumber(-0.14f, false)");

    mRunStandardTest(Math::NiceNumber(0.0)==0.0, "NiceNumber(0.0)");
    mRunStandardTest(Math::NiceNumber(14.0)==10.0, "NiceNumber(14.0)");
    mRunStandardTest(Math::NiceNumber(-14.0)==-10.0, "NiceNumber(-14.0)");
    mRunStandardTest(Math::NiceNumber(14.0, false)==20.0,
						     "NiceNumber(14.0, false)");
    mRunStandardTest(Math::NiceNumber(-14.0, false)==-20.0,
						 "NiceNumber(-14.0, false)");
    mRunStandardTest(Math::NiceNumber(0.14)==0.1, "NiceNumber(0.14)");
    mRunStandardTest(Math::NiceNumber(-0.14)==-0.1, "NiceNumber(-0.14)");
    mRunStandardTest(Math::NiceNumber(0.14, false)==0.2,
						 "NiceNumber(0.14, false)");
    mRunStandardTest(Math::NiceNumber(-0.14, false)==-0.2,
						 "NiceNumber(-0.14, false)");
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if (       !testBits()
	    || !testNiceNumber() )
	return 1;

    return 0;
}
