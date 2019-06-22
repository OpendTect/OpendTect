/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman K Singh
 * DATE     : July 2009
___________________________________________________________________

-*/



#include "survgeometry.h"
#include "survinfo.h"
#include "trigonometry.h"
#include "trckeysampling.h"

Coord SurveyGeometry::getEdgePoint( const Coord& from, const Coord& to )
{
    Line2 line( from, to );

    const TrcKeySampling hs( true );
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
	float mindist = mUdf(float);
	int mindistidx = -1;
	for ( int idx=0; idx<4; idx++ )
	{
	    const float dist =
		svert[idx].distTo<float>( line.closestPoint(svert[idx]) );
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
    if ( ptonline.distTo<float>(edgept) > 1e-4 )
	return false;			// EdgePoint not on line

    const float linelen = from.distTo<float>( to );
    const float distfrom = from.distTo<float>( edgept );
    const float distto = to.distTo<float>( edgept );
    if ( distfrom < linelen && distto < linelen )
	return true;			// EdgePoint between 'from' and 'to'

    if ( distfrom > distto )
	return true;			// EdgePoint in the 'to' direction

    return false;
}
