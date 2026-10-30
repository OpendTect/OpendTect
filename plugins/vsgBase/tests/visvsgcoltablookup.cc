/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "visvsgcoltablookup.h"
#include "vsggeocolorlookup.h"

#include "coltabsequence.h"
#include "math2.h"

namespace
{

ColTab::Sequence redToBlue()
{
    ColTab::Sequence seq;
    seq.setColor( 0.f, 255, 0, 0 );
    seq.setColor( 1.f, 0, 0, 255 );
    return seq;
}


bool isNear( const vsg::vec4& col, float red, float green, float blue,
	     float alpha )
{
    const float eps = 3.f / 255.f;
    return Math::Abs(col.r-red)<eps && Math::Abs(col.g-green)<eps &&
	   Math::Abs(col.b-blue)<eps && Math::Abs(col.a-alpha)<eps;
}


} // namespace


static bool testSequence()
{
    dGBVis::ColTabLookup lookup;
    mRunStandardTest( lookup.nrColors()==255, "Lookup: 255 colors" );

    lookup.setSequence( redToBlue() );
    mRunStandardTest( isNear(lookup.colorAt(0.f),1.f,0.f,0.f,1.f),
		      "Lookup: The start of the table is red" );
    mRunStandardTest( isNear(lookup.colorAt(1.f),0.f,0.f,1.f,1.f),
		      "Lookup: The end of the table is blue" );
    mRunStandardTest( isNear(lookup.colorAt(0.5f),0.5f,0.f,0.5f,1.f),
		      "Lookup: The middle is a mix of the two" );
    return true;
}


static bool testUndefColor()
{
    ColTab::Sequence seq = redToBlue();
    seq.setUndefColor( OD::Color(10,20,30) );
    dGBVis::ColTabLookup lookup;
    lookup.setSequence( seq );
    mRunStandardTest( isNear(lookup.undefColor(),10.f/255.f,20.f/255.f,
			     30.f/255.f,1.f),
		      "Lookup: The undefined color is kept apart" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testSequence() ||
	 !testUndefColor() )
	return 1;

    return 0;
}
