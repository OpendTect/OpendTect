/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bendpoints2coords.h"
#include "sorting.h"
#include "od_iostream.h"


BendPoints2Coords::BendPoints2Coords( const TypeSet<Coord>& crds,
				      const int* nrs )
{
    init( crds, nrs );
}


BendPoints2Coords::BendPoints2Coords( od_istream& strm )
{
    readFrom( strm );
}


void BendPoints2Coords::readFrom( od_istream& strm )
{
    TypeSet<Coord> crds;
    TypeSet<int> nrs;

    Coord crd; int nr;

    while ( strm.isOK() )
    {
        strm >> nr >> crd.x_ >> crd.y_;
	if ( strm.isBad() )
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
    mGetIdxArr( int, idxs, sz );
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
    const Coord c0( coords_[idxs.start_] ); const Coord c1( coords_[idxs.stop_] );
    const float frac = (pos-nrs_[idxs.start_])
		     / (nrs_[idxs.stop_]-nrs_[idxs.start_]);
    return Coord( c0.x_ + frac * (c1.x_ - c0.x_), c0.y_ + frac * (c1.y_ - c0.y_) );
}


void BendPoints2Coords::getIndexes( float pos, Interval<int>& idxs ) const
{
    idxs.start_ = 0;
    idxs.stop_ = nrs_.size()-1;

    while ( idxs.stop_-idxs.start_ > 1 )
    {
	const int mididx = (idxs.stop_+idxs.start_) / 2;
	if ( nrs_[mididx] > pos )
	    idxs.stop_ = mididx;
	else
	    idxs.start_ = mididx;
    }
}
