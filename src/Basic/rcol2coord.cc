/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          18-4-1996
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "rcol2coord.h"

#include "ranges.h"


bool RCol2Coord::set3Pts( const Coord& c0, const Coord& c1,
			  const Coord& c2, const RowCol& rc0,
			  const RowCol& rc1, od_int32 col2 )
{
    return set3Pts( c0, c1, c2, BinID(rc0), BinID(rc1), col2 );
}


bool RCol2Coord::set3Pts( const Coord& c0, const Coord& c1,
			  const Coord& c2, const BinID& bid0,
			  const BinID& bid1, od_int32 crl2 )
{
    if ( bid1.inl == bid0.inl )
	return false;
    if ( bid0.crl == crl2 )
        return false;

    RCTransform nxtr, nytr;
    od_int32 cold = bid0.crl - crl2;
    nxtr.c = ( c0.x - c2.x ) / cold;
    nytr.c = ( c0.y - c2.y ) / cold;
    const od_int32 rowd = bid0.inl - bid1.inl;
    cold = bid0.crl - bid1.crl;
    nxtr.b = ( c0.x - c1.x ) / rowd - ( nxtr.c * cold / rowd );
    nytr.b = ( c0.y - c1.y ) / rowd - ( nytr.c * cold / rowd );
    nxtr.a = c0.x - nxtr.b * bid0.inl - nxtr.c * bid0.crl;
    nytr.a = c0.y - nytr.b * bid0.inl - nytr.c * bid0.crl;

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


Coord RCol2Coord::transform( const RowCol& rc ) const
{
    return Coord( xtr.a + xtr.b*rc.row + xtr.c*rc.col,
		  ytr.a + ytr.b*rc.row + ytr.c*rc.col );
}


Coord RCol2Coord::transform( const BinID& rc ) const
{
    return Coord( xtr.a + xtr.b*rc.inl + xtr.c*rc.crl,
		  ytr.a + ytr.b*rc.inl + ytr.c*rc.crl );
}


BinID RCol2Coord::transformBack( const Coord& coord,
				  const StepInterval<int>* rowrg,
				  const StepInterval<int>* colrg) const
{
    if ( mIsUdf(coord.x) || mIsUdf(coord.y) )
	return RowCol(mUdf(int),mUdf(int));

    const Coord res = transformBackNoSnap( coord );
    if ( mIsUdf(res.x) || mIsUdf(res.y) )
	return RowCol(mUdf(int),mUdf(int));

    return BinID(rowrg ? rowrg->snap(res.x) : mNINT32(res.x),
	    	  colrg ? colrg->snap(res.y) : mNINT32(res.y));
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


