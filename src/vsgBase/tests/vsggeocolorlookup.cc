/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "vsggeocolorlookup.h"

#include "math2.h"

namespace
{

bool isNear( const vsg::vec4& col, float red, float green, float blue,
	     float alpha )
{
    const float eps = 1.5f / 255.f;
    return Math::Abs(col.r-red)<eps && Math::Abs(col.g-green)<eps &&
	   Math::Abs(col.b-blue)<eps && Math::Abs(col.a-alpha)<eps;
}

} // namespace


static bool testColors()
{
    vsg::ref_ptr<vsgGeo::ColorLookup> lookup = vsgGeo::ColorLookup::create( 5 );
    mRunStandardTest( lookup->nrColors()==5, "Lookup: Number of colors" );

    const vsg::vec4 colors[5] = {
	vsg::vec4( 1.f, 0.f, 0.f, 1.f ), vsg::vec4( 0.f, 1.f, 0.f, 1.f ),
	vsg::vec4( 0.f, 0.f, 1.f, 1.f ), vsg::vec4( 1.f, 1.f, 0.f, 0.5f ),
	vsg::vec4( 0.f, 0.f, 0.f, 0.f ) };
    const vsg::vec4 undefcolor( 0.f, 1.f, 1.f, 1.f );
    mRunStandardTest( lookup->setColors(colors,5,undefcolor),
		      "Lookup: Colors are set" );
    mRunStandardTest( !lookup->setColors(colors,4,undefcolor),
		      "Lookup: A wrong number of colors is refused" );
    mRunStandardTest( isNear(lookup->undefColor(),0.f,1.f,1.f,1.f),
		      "Lookup: The undefined color follows the colors" );

    mRunStandardTest( isNear(lookup->colorAt(0.f),1.f,0.f,0.f,1.f),
		      "Lookup: The first color is the value 0" );
    mRunStandardTest( isNear(lookup->colorAt(1.f),0.f,0.f,0.f,0.f),
		      "Lookup: The last color is the value 1" );
    mRunStandardTest( isNear(lookup->colorAt(0.5f),0.f,0.f,1.f,1.f),
		      "Lookup: The middle color is the value 0.5" );
    mRunStandardTest( isNear(lookup->colorAt(0.125f),0.5f,0.5f,0.f,1.f),
		      "Lookup: Between two colors it is blended" );
    mRunStandardTest( isNear(lookup->colorAt(-3.f),1.f,0.f,0.f,1.f) &&
		      isNear(lookup->colorAt(7.f),0.f,0.f,0.f,0.f),
		      "Lookup: Outside 0 to 1 it stays at the ends" );
    return true;
}


static bool testDescriptor()
{
    vsg::ref_ptr<vsgGeo::ColorLookup> lookup = vsgGeo::ColorLookup::create();
    mRunStandardTest( lookup->nrColors()==255,
		      "Lookup: 255 colors by default" );
    vsg::ref_ptr<vsg::DescriptorImage> descriptor = lookup->descriptor( 3 );
    mRunStandardTest( descriptor.valid() && descriptor->dstBinding==3,
		      "Lookup: The descriptor is made for the binding" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testColors() ||
	 !testDescriptor() )
	return 1;

    return 0;
}
