/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id: refcount.cc 30402 2013-06-21 07:20:27Z kristofer.tingdahl@dgbes.com $";

#include "genc.h"
#include "testprog.h"


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !quiet )
	od_cout() << "Exiting with return code 1" << od_endl;

    return ExitProgram( 1 );
}
