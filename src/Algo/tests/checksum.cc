/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
