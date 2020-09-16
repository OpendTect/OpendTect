/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "nrbytes2string.h"
#include "testprog.h"
#include "globexpr.h"



int main( int argc, char** argv )
{
    mInitTestProg();

    if ( quiet_ || argc != 3 )
	return ExitProgram( 0 );

    BufferString tomatch( argv[1] );
    GlobExpr ge( argv[2] );

    od_cout() << "'" << argv[1]
	<< (ge.matches(argv[1]) ? "' matches '" : "' doesn't match '")
	<< argv[2] << "'" << od_endl;

    return ExitProgram( 0 );
}
