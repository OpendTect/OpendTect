/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "geometry.h"


#include "testprog.h"


static bool testRectangle()
{
    Geom::Rectangle<int> normalrect( 0, 0, 1, 1 );
    Geom::Rectangle<int> reversedrect( 1, 1, 0, 0 );

    Geom::Rectangle<int> test = reversedrect;
    test.limitTo( normalrect );
    mRunStandardTest( test==reversedrect,
		      "Reversed limit by normal rectangle" );
    test.limitTo( reversedrect );
    mRunStandardTest( test==reversedrect,
		      "Reversed limit by reversed rectangle");

    test = normalrect;
    test.limitTo( normalrect );
    mRunStandardTest( test==normalrect, "Normal limit by normal rectangle" );
    test.limitTo( reversedrect );
    mRunStandardTest( test==normalrect, "Normal limit by reversed rectangle" );

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testRectangle() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
