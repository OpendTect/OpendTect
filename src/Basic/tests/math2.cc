/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2013
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

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

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    return testBits() ? 0 : 1;
}
