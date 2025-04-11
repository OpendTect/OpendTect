/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "color.h"

#include "testprog.h"

bool testRGB2HSVConversion()
{
    OD::Color col(220, 20, 60);
    unsigned char h, s, v;

    col.getHSV( h, s, v );
    mRunStandardTest( h==247 && s==232 && v==220, "RGB -> HSV" );

    col = OD::Color(100,100,100);
    col.getHSV( h, s, v );
    mRunStandardTest( h==0 && s==0 && v==100, "RGB -> HSV" );

    return true;
}


bool testHSV2RGBConversion()
{
    OD::Color col;
    col.setHSV( 247, 232, 220 );
    mRunStandardTest( col.r()==220 && col.g()==20 && col.b()==58,
		      "HSV -> RGB" );

    col.setHSV( 0, 0, 100 );
    mRunStandardTest( col.r()==100 && col.g()==100 && col.b()==100,
		      "HSV -> RGB" );

    return true;
}


bool testDistinctColors()
{
    const OD::Color col(220, 20, 60);
    TypeSet<OD::Color> cols = col.distinctColors( 2 );
    mRunStandardTest( cols[0]==OD::Color(58,220,20) &&
		      cols[1]==OD::Color(20,58,220), "Distinct Colors" );
    return true;
}


bool testComplimentaryColors()
{
    const OD::Color col(32, 32, 32);
    TypeSet<OD::Color> cols = col.complimentaryColors( 2 );
    mRunStandardTest( cols[0]==OD::Color(0,223,0) &&
    cols[1]==OD::Color(0,0,223), "Complimentary Colors" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testRGB2HSVConversion() ||
	 !testHSV2RGBConversion() ||
	 !testDistinctColors() ||
	 !testComplimentaryColors() )
	return 1;

    return 0;
}
