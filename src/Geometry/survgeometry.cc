/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "survgeometry.h"
#include "survinfo.h"
#include "trigonometry.h"
#include "trckeyzsampling.h"

Coord SurveyGeometry::getEdgePoint( const Coord& from, const Coord& to )
{
    Line2 line( from, to );
    line.start_ = Coord::udf();
    line.stop_ = Coord::udf();		// making the line infinite
    const TrcKeySampling hs( SI().sampling(false).hsamp_ );
    Coord svert[4];
    svert[0] = SI().transform( hs.start_ );
    svert[1] = SI().transform( BinID(hs.start_.inl(),hs.stop_.crl()) );
    svert[2] = SI().transform( hs.stop_ );
    svert[3] = SI().transform( BinID(hs.stop_.inl(),hs.start_.crl()) );

    TypeSet<Coord> points;
    for ( int idx=0; idx<4; idx++ )
    {
	Line2 survbound( svert[idx], idx < 3 ? svert[idx+1] : svert[0] );
	Coord pt = line.intersection( survbound );
        if ( !mIsUdf(pt.x_) && !mIsUdf(pt.y_) )
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
    double linexdiff = to.x_ - from.x_;
    if ( mIsZero(linexdiff,mDefEps) )
    {
        double lineydiff = to.y_ - from.y_;
        double ptsydiff = points[1].y_ - points[0].y_;
	return lineydiff * ptsydiff > 0 ? points[1] : points[0];
    }

        double ptsxdiff = points[1].x_ - points[0].x_;
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
