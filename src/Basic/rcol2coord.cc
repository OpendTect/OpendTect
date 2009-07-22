/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          18-4-1996
________________________________________________________________________

-*/
static const char* rcsID = "$Id: rcol2coord.cc,v 1.8 2009-07-22 16:01:31 cvsbert Exp $";

#include "rcol2coord.h"

#include "ranges.h"


bool RCol2Coord::set3Pts( const Coord& c0, const Coord& c1,
			  const Coord& c2, const RCol& rc0,
			  const RCol& rc1, od_int32 col2 )
{
    if ( rc1.r() == rc0.r() )
	return false;
    if ( rc0.c() == col2 )
        return false;

    RCTransform nxtr, nytr;
    od_int32 cold = rc0.c() - col2;
    nxtr.c = ( c0.x - c2.x ) / cold;
    nytr.c = ( c0.y - c2.y ) / cold;
    const od_int32 rowd = rc0.r() - rc1.r();
    cold = rc0.c() - rc1.c();
    nxtr.b = ( c0.x - c1.x ) / rowd - ( nxtr.c * cold / rowd );
    nytr.b = ( c0.y - c1.y ) / rowd - ( nytr.c * cold / rowd );
    nxtr.a = c0.x - nxtr.b * rc0.r() - nxtr.c * rc0.c();
    nytr.a = c0.y - nytr.b * rc0.r() - nytr.c * rc0.c();

    if ( mIsZero(nxtr.a,mDefEps) ) nxtr.a = 0;
    if ( mIsZero(nxtr.b,mDefEps) ) nxtr.b = 0;
    if ( mIsZero(nxtr.c,mDefEps) ) nxtr.c = 0;
    if ( mIsZero(nytr.a,mDefEps) ) nytr.a = 0;
    if ( mIsZero(nytr.b,mDefEps) ) nytr.b = 0;
    if ( mIsZero(nytr.c,mDefEps) ) nytr.c = 0;

    if ( !nxtr.valid(nytr) )
	return false;

    xtr = nxtr;
    ytr = nytr;
    return true;
}


Coord RCol2Coord::transform( const RCol& rc ) const
{
    return Coord( xtr.a + xtr.b*rc.r() + xtr.c*rc.c(),
		  ytr.a + ytr.b*rc.r() + ytr.c*rc.c() );
}


RowCol RCol2Coord::transformBack( const Coord& coord,
				  const StepInterval<int>* rowrg,
				  const StepInterval<int>* colrg) const
{
    if ( mIsUdf(coord.x) || mIsUdf(coord.y) )
	return RowCol(mUdf(int),mUdf(int));

    const Coord res = transformBackNoSnap( coord );
    if ( mIsUdf(res.x) || mIsUdf(res.y) )
	return RowCol(mUdf(int),mUdf(int));

    return RowCol(rowrg ? rowrg->snap(res.x) : mNINT(res.x),
	    	  colrg ? colrg->snap(res.y) : mNINT(res.y));
}


Coord RCol2Coord::transformBackNoSnap( const Coord& coord ) const
{
    if ( mIsUdf(coord.x) || mIsUdf(coord.y) )
	return Coord::udf();

    double det = xtr.det( ytr );
    if ( mIsZero(det,mDefEps) ) 
	return Coord::udf();

    const double x = coord.x - xtr.a;
    const double y = coord.y - ytr.a;
    return Coord( (x*ytr.c - y*xtr.c) / det, (y*xtr.b - x*ytr.b) / det );
}


Coord RCol2Coord::transform( const Coord& rc ) const
{
    return Coord( xtr.a + xtr.b*rc.x + xtr.c*rc.y,
		  ytr.a + ytr.b*rc.x + ytr.c*rc.y );
}


