/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2014
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uistring.h"
#include "testprog.h"

#include <QString>

bool testArg()
{
    uiString string = uiString( "Hello %1 %2").arg( "Dear" ).arg( toString(1) );
    mRunStandardTest( string.getQtString()==QString("Hello Dear 1"),
		     "Standard argument order");

    string = uiString( "Hello %2 %1").arg( toString( 1 ) ).arg( "Dear" );
    mRunStandardTest( string.getQtString()==QString("Hello Dear 1"),
		     "Reversed argument order");

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testArg() )
	ExitProgram( 1 );

    ExitProgram( 0 );
}
