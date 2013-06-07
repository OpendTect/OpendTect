/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman K Singh
 * DATE     : July 2009
___________________________________________________________________

-*/

static const char* rcsID = "$Id$";


#include "survgeometry.h"
#include "survinfo.h"
#include "trigonometry.h"
#include "cubesampling.h"

Coord SurveyGeometry::getEdgePoint( const Coord& from, const Coord& to )
{
    Line2 line( from, to );
    line.start_ = Coord::udf();
    line.stop_ = Coord::udf();		// making the line infinite
    const HorSampling hs( SI().sampling(false).hrg );
    Coord svert[4];
    svert[0] = SI().transform( hs.start );
    svert[1] = SI().transform( BinID(hs.start.inl,hs.stop.crl) );
    svert[2] = SI().transform( hs.stop );
    svert[3] = SI().transform( BinID(hs.stop.inl,hs.start.crl) );

    TypeSet<Coord> points;
    for ( int idx=0; idx<4; idx++ )
    {
	Line2 survbound( svert[idx], idx < 3 ? svert[idx+1] : svert[0] );
	Coord pt = line.intersection( survbound );
	if ( !mIsUdf(pt.x) && !mIsUdf(pt.y) )
	    points += pt;
    }

    if ( points.size() == 1 )
	return points[0];

    if ( !points.size() )
    {
	double mindist = mUdf(double);
	int mindistidx = -1;
	for ( int idx=0; idx<4; idx++ )
	{
	    double dist = svert[idx].distTo( line.closestPoint(svert[idx]) );
	    if ( dist < mindist )
	    {
		mindist = dist;
		mindistidx = idx;
	    }
	}

	return svert[mindistidx];
    }

    // Two points: will choose the one in the direction from->to
    double linexdiff = to.x - from.x;
    if ( mIsZero(linexdiff,mDefEps) )
    {
	double lineydiff = to.y - from.y;
	double ptsydiff = points[1].y - points[0].y;
	return lineydiff * ptsydiff > 0 ? points[1] : points[0];
    }

    double ptsxdiff = points[1].x - points[0].x;
    return linexdiff * ptsxdiff > 0 ? points[1] : points[0];
}


bool SurveyGeometry::hasEdgePoint( const Coord& from, const Coord& to )
{
    Coord edgept = getEdgePoint( from, to );
    Line2 line( from, to );
    Coord ptonline = line.closestPoint( edgept );
    if ( ptonline.distTo(edgept) > 1e-4 )
	return false;			// EdgePoint not on line

    double linelen = from.distTo( to );
    double distfrom = from.distTo( edgept );
    double distto = to.distTo( edgept );
    if ( distfrom < linelen && distto < linelen )
	return true;			// EdgePoint between 'from' and 'to'

    if ( distfrom > distto )
	return true;			// EdgePoint in the 'to' direction

    return false;
}


