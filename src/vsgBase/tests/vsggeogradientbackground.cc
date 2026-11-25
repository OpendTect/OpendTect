/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "vsggeogradientbackground.h"
#include "vsgtestassert.h"
#include "vsgtesthelper.h"

#include "math2.h"


static bool isNear( int value, int expected, int tolerance )
{
    return Math::Abs( value - expected ) <= tolerance;
}


static bool testGradient()
{
    vsgTest::OffscreenViewer ov( 256, 256 );
    mRunStandardTestWithError( ov.isOK(), "Gradient background: Off-screen "
			       "viewer", ov.errMsg().c_str() );

    vsg::ref_ptr<vsgGeo::GradientBackground> bg =
					vsgGeo::GradientBackground::create();
    bg->setColors( vsg::vec4(0.4f,0.4f,0.4f,1.f),
		   vsg::vec4(0.1f,0.1f,0.1f,1.f) );
    bg->setSize( 256.f, 256.f );
    ov.addHud()->addChild( bg );
    mRunStandardTest( ov.compile(), "Gradient background: Viewer compiled" );
    ov.frame( 3 );

    TypeSet<vsg::ubvec4> pixels;
    mRunStandardTest( ov.readColor(pixels), "Gradient background: Image is "
		      "read" );
    int center = ov.pixel( pixels, 128, 128 ).r;
    int edge = ov.pixel( pixels, 128, 2 ).r;
    int corner = ov.pixel( pixels, 1, 1 ).r;
    BufferString msg( "Center: ", center );
    msg.add( ", edge: " ).add( edge ).add( ", corner: " ).add( corner );
    mRunStandardTestWithError( isNear(center,102,3) && isNear(edge,49,4) &&
			       isNear(corner,26,4),
			       "Gradient background: The color goes from the "
			       "center color to the corner color", msg );

    bg->setColors( vsg::vec4(0.2f,0.2f,0.2f,1.f),
		   vsg::vec4(0.2f,0.2f,0.2f,1.f) );
    ov.frame( 3 );
    mRunStandardTest( ov.readColor(pixels), "Gradient background: Second "
		      "image is read" );
    center = ov.pixel( pixels, 128, 128 ).r;
    corner = ov.pixel( pixels, 1, 1 ).r;
    msg.set( "Center: " ).add( center ).add( ", corner: " ).add( corner );
    mRunStandardTestWithError( isNear(center,51,2) && isNear(corner,51,2),
			       "Gradient background: The colors can be changed "
			       "afterwards", msg );

    mRunStandardTestWithError( ov.nrValidationMessages()==0,
			       "Gradient background: No validation messages",
			       BufferString("Messages: ",
					    ov.nrValidationMessages()) );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testGradient() )
	return 1;

    return 0;
}
