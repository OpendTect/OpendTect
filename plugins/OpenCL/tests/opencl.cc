/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

#include "openclplatform.h"

#include "testprog.h"

static bool testPlatform()
{
    mRunStandardTest( OpenCL::Platform::initClass(), "OpenCL init platforms" );
    
    return true;
}

int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testPlatform() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
