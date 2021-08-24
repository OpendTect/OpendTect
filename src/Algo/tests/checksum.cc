/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2015
 * FUNCTION :
-*/


#include "checksum.h"
#include "testprog.h"


bool testChecksum()
{
    mRunStandardTest(
	  checksum64( (unsigned char*) "123456789",9,0 )==0xe9c6d914c4b8d9caULL,
	    "64 bit checksum" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    return testChecksum() ? 0 : 1;
}
