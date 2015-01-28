/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2015
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id: callback.cc 33379 2014-02-18 07:54:56Z kristofer.tingdahl@dgbes.com $";

#include "checksum.h"
#include "testprog.h"


bool testChecksum()
{
    mRunStandardTest(
	    checksum64( (unsigned char*) "123456789",9,0 )==0xe9c6d914c4b8d9ca,
	    "64 bit checksum" );

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testChecksum() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
