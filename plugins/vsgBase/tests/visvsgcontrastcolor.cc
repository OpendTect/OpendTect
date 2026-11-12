/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 The color that is drawn for the annotation on a background. No GPU.
-*/

#include "testprog.h"

#include "visvsgdata.h"


static bool isGray( const OD::Color& col )
{
    return col.r()==col.g() && col.g()==col.b();
}


static bool testContrastingColor()
{
    const OD::Color snow( 255, 250, 250 );
    const OD::Color darkcyan( 0, 139, 139 );
    const OD::Color white = OD::Color::White();
    const OD::Color black = OD::Color::Black();

    const OD::Color onsnow = ODVsg::contrastingColor( white, snow );
    BufferString msg( "Contrast: ", onsnow.contrast(snow) );
    mRunStandardTestWithError( isGray(onsnow) && onsnow!=black &&
			       onsnow.contrast(snow)>=5.f &&
			       onsnow.contrast(snow)<6.f,
			       "Contrasting color: White on a light background "
			       "is made a gray with a contrast of 5", msg );
    mRunStandardTest( ODVsg::contrastingColor(white,darkcyan) == white,
		      "Contrasting color: White on a dark background is kept" );
    mRunStandardTest( ODVsg::contrastingColor(black,snow) == black,
		      "Contrasting color: Black on a light background is "
		      "kept" );

    const OD::Color ondark = ODVsg::contrastingColor(black,
						      OD::Color::Anthracite());
    msg.set( "Color: " ).add( ondark.r() );
    mRunStandardTestWithError( isGray(ondark) && ondark.r()>=140 &&
			       ondark.r()<=190,
			       "Contrasting color: Black on a dark background "
			       "is made a light gray", msg );

    const OD::Color midgray( 118, 118, 118 );
    mRunStandardTest( ODVsg::contrastingColor(midgray,midgray) == black,
		      "Contrasting color: Black is used when no gray stands "
		      "out enough" );

    const OD::Color red( 220, 20, 60 );
    mRunStandardTest( ODVsg::contrastingColor(red,snow) == red,
		      "Contrasting color: A color that stands out is kept" );
    mRunStandardTest( ODVsg::contrastingColor(red,red) != red,
		      "Contrasting color: A color on itself is changed" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testContrastingColor() )
	return 1;

    return 0;
}
