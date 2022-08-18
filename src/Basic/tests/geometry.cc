/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometry.h"


#include "testprog.h"


static bool testRectangle()
{
    const Geom::Rectangle<int> normalrect( 0, 0, 1, 1 );
    const Geom::Rectangle<int> reversedrect( 1, 1, 0, 0 );

    const Geom::Rectangle<int> normalinput( -1, -1, 2, 2 );
    const Geom::Rectangle<int> reversedinput( 2, 2, -1, -1 );


    Geom::Rectangle<int> test = reversedinput;
    test.limitTo( normalrect );
    mRunStandardTest( test==reversedrect,
		      "Reversed limit by normal rectangle" );

    test = reversedinput;
    test.limitTo( reversedrect );
    mRunStandardTest( test==reversedrect,
		      "Reversed limit by reversed rectangle");

    test = normalinput;
    test.limitTo( normalrect );
    mRunStandardTest( test==normalrect, "Normal limit by normal rectangle" );

    test = normalinput;
    test.limitTo( reversedrect );
    mRunStandardTest( test==normalrect, "Normal limit by reversed rectangle" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    return testRectangle() ? 0 : 1;
}
