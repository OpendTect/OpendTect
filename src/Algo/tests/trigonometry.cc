/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2015
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "trigonometry.h"
#include "testprog.h"


static bool testLine2()
{
    const Line2 line( Coord(-2,-2),Coord(2,2) );
    const double sqrt2 = Math::Sqrt(2.0);
    const double halfsqrt2 = sqrt2 / 2.0;

    mRunStandardTest( line.getPoint(0)==Coord(-2,-2) , "Start position" );
    mRunStandardTest( line.getPoint(1)==Coord(2,2) , "Stop position" );

    mRunStandardTest( line.direction(false)==Coord(4,4), "Direction" );
    mRunStandardTest( line.direction(true)==Coord(halfsqrt2,halfsqrt2),
		      "Normalized Direction" );

    mRunStandardTest( mIsEqual( line.closestParam(Coord(-1,1)),0.5,mDefEpsD),
			      "Closest param" );
    mRunStandardTest(
	    mIsEqual( line.sqDistanceToPoint( Coord(-1,1)), 2, mDefEpsD),
	    "sqDistanceToPoint");

    const Line2 crossingline( Coord(0,5), Coord(5,0) );

    mRunStandardTest(
	mIsZero( line.intersection(crossingline,false).sqDistTo(
	    Coord(2.5,2.5) ), mDefEpsD),
	"Intersection ouside range");

    mRunStandardTest( line.intersection( crossingline, true ).isUdf(),
		     "Intersection with range check");

    Line2 perpendicular;
    line.getPerpendicularLine(perpendicular, Coord(1,1));
    mRunStandardTest( perpendicular.getPoint(0)==Coord(1,1),
		      "Start point of perpendicular line");
    mRunStandardTest(
	    perpendicular.direction(true)==Coord(-halfsqrt2,halfsqrt2) ||
	    perpendicular.direction(true)==Coord(halfsqrt2,-halfsqrt2),
	    "Direction of perpendicular line");

    Line2 parallel;
    line.getParallelLine( parallel, sqrt2 );
    mRunStandardTest( parallel.direction(true)==line.direction(true),
		     "Direction of parallel line");
    mRunStandardTest( parallel.isOnLine( Coord(-1,1) ) ||
		      parallel.isOnLine( Coord(1,-1) ),
		     "Point on parallel line");

    mRunStandardTest( line.isOnLine(Coord(0,0)), "isOnLine - true");
    mRunStandardTest( !line.isOnLine(Coord(1,0)), "isOnLine - false");

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testLine2() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}

