/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2009
-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "bendpoints2coords.h"
#include "sorting.h"
#include <iostream>


BendPoints2Coords::BendPoints2Coords( const TypeSet<Coord>& crds,
				      const int* nrs )
{
    init( crds, nrs );
}


BendPoints2Coords::BendPoints2Coords( std::istream& strm )
{
    readFrom( strm );
}


void BendPoints2Coords::readFrom( std::istream& strm )
{
    TypeSet<Coord> crds; TypeSet<int> nrs;
    Coord crd; int nr;
    while ( strm.good() )
    {
	strm >> nr >> crd.x >> crd.y;
	if ( !strm.good() || (mIsZero(crd.x,1e-4) && mIsZero(crd.y,1e-4)) )
	    break;
	nrs += nr;
	crds += crd;
    }

    init( crds, nrs.arr() );
}


void BendPoints2Coords::init( const TypeSet<Coord>& crds,
			      const int* inpnrs )
{
    coords_.erase(); nrs_.erase();

    const int sz = crds.size();
    if ( sz < 1 )
	return;
    else if ( sz == 1 )
    {
	coords_ += crds[0]; nrs_ += inpnrs ? inpnrs[0] : 0;
	return;
    }
    else if ( !inpnrs )
    {
	for ( int idx=0; idx<sz; idx++ )
	    { coords_ += crds[idx]; nrs_ += idx; }
	return;
    }

    TypeSet<int> nrs( inpnrs, sz );
    mGetIdxArr(int,idxs,sz)
    if ( !idxs ) return;
    sort_coupled( nrs.arr(), idxs, sz );

    for ( int idx=0; idx<sz; idx++ )
    {
	const int sidx = idxs[idx];
	coords_ += crds[sidx]; nrs_ += inpnrs[sidx];
    }

    delete [] idxs;
}


Coord BendPoints2Coords::coordAt( float pos ) const
{
    const int sz = coords_.size();
    if ( sz < 1 )
	return Coord(0,0);
    else if ( sz == 1 )
	return coords_[0];

    Interval<int> idxs; getIndexes( pos, idxs );
    const Coord c0( coords_[idxs.start] ); const Coord c1( coords_[idxs.stop] );
    const float frac = (pos-nrs_[idxs.start])
		     / (nrs_[idxs.stop]-nrs_[idxs.start]);
    return Coord( c0.x + frac * (c1.x - c0.x), c0.y + frac * (c1.y - c0.y) );
}


void BendPoints2Coords::getIndexes( float pos, Interval<int>& idxs ) const
{
    idxs.start = 0; idxs.stop = nrs_.size()-1;

    while ( idxs.stop-idxs.start > 1 )
    {
	const int mididx = (idxs.stop+idxs.start) / 2;
	if ( nrs_[mididx] > pos )
	    idxs.stop = mididx;
	else
	    idxs.start = mididx;
    }
}
