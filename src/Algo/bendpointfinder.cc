/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 9-3-1999
-*/

#include "bendpointfinder.h"
#include "sorting.h"
#include "survgeom.h"
#include "trigonometry.h"


#define nrworking_ nrthreads_	/* ABI preservation */

BendPointFinderBase::BendPointFinderBase( int sz, float eps )
    : sz_(sz)
    , epssq_(eps*eps)
    , nrworking_(0)
{}


bool BendPointFinderBase::doPrepare( int )
{
    bendpts_ += 0;
    if ( sz_ > 1 )
	bendpts_ += sz_-1;

    queue_.erase();

    queue_ += Interval<int>( 0, sz_-1 );

    nrworking_ = 0;

    return true;
}


bool BendPointFinderBase::doFinish( bool res )
{
    if ( res )
    {
	quickSort( bendpts_.arr(), bendpts_.size() );
    }

    return res;
}


bool BendPointFinderBase::doWork( od_int64, od_int64, int )
{
    lock_.lock();
    int sz = queue_.size();
    while ( sz || nrworking_ )
    {
	while ( sz )
	{
	    const Interval<int> segment = queue_[sz-1];
	    queue_.removeSingle( sz-1 );
	    nrworking_ ++;
	    lock_.unLock();

	    findInSegment( segment.start, segment.stop );

	    lock_.lock();
	    nrworking_--;
	    sz = queue_.size();

	    if ( !nrworking_ && !sz )
		lock_.signal( true );
	}

	//Wait while the other threads are either finishing or adding to queue
	while ( nrworking_ && !sz )
	{
	    lock_.wait();
	    sz = queue_.size();
	}
    }

    lock_.unLock();

    return true;
}


void BendPointFinderBase::findInSegment( int idx0, int idx1 )
{
    int nrdone = 0;
    while ( true )
    {
	if ( idx1<=idx0+1 )
	{
	    nrdone++;
	    break; // First stop criterion
	}

	int idx;
	const float sqdist = getMaxSqDistToLine( idx, idx0, idx1 );

	if ( sqdist < epssq_ )
	{
	    nrdone += idx1-idx0;
	    break; // Second stop criterion
	}

	lock_.lock();
	bendpts_ += idx;
	if ( idx1>idx+1 )
	{
	    queue_ += Interval<int>( idx, idx1 );
	    lock_.signal( queue_.size()>1 );
	}

	lock_.unLock();
	idx1 = idx;
    }

    if ( nrdone )
	addToNrDone( nrdone );
}

#undef nrworking_	/* ABI preservation */


// BendPointFinder2DBase
BendPointFinder2DBase::BendPointFinder2DBase( int sz, float eps )
    : BendPointFinderBase(sz,eps)
{
}


float BendPointFinder2DBase::getMaxSqDistToLine( int& idx, int start,
						 int stop ) const
{
    if ( stop-start==2 )
    {
	idx = start+1;
	return (float)(((coord(start)+coord(stop))/2).sqDistTo(coord(idx)));
    }

    const Line2 line( coord(start), coord(stop) );
    Coord nearestpt;
    double dsqmax = 0;

    for ( int ipt=start+1; ipt<stop; ipt++ )
    {
	const Coord crd( coord(ipt) );
	nearestpt = line.closestPoint( crd );
	const double dsq = nearestpt.sqDistTo( crd );
	if ( dsq>dsqmax ) { dsqmax = dsq; idx = ipt; }
    }

    return (float) dsqmax;
}



// BendPointFinder2D
BendPointFinder2D::BendPointFinder2D( const TypeSet<Coord>& crd, float eps )
    : BendPointFinder2DBase( crd.size(), eps )
    , coords_( crd.arr() )
{}


BendPointFinder2D::BendPointFinder2D( const Coord* crd, int sz, float eps )
    : BendPointFinder2DBase( sz, eps )
    , coords_( crd )
{}


const Coord& BendPointFinder2D::coord( int idx ) const
{ return coords_[idx]; }



// BendPointFinderTrcKey
BendPointFinderTrcKey::BendPointFinderTrcKey( const TypeSet<TrcKey>& tks,
					      float eps )
    : BendPointFinder2DBase( tks.size(), eps )
    , tks_(tks)
{
    for ( int idx=0; idx<tks_.size(); idx++ )
	coords_ += Survey::GM().toCoord( tks_[idx] );
}


const Coord& BendPointFinderTrcKey::coord( int idx ) const
{ return coords_[idx]; }



// BendPointFinder2DGeom
BendPointFinder2DGeom::BendPointFinder2DGeom(
	const TypeSet<PosInfo::Line2DPos>& pos, float eps )
    : BendPointFinder2DBase( pos.size(), eps )
    , positions_(pos)
{}


const Coord& BendPointFinder2DGeom::coord( int idx ) const
{ return positions_[idx].coord_; }




// BendPointFinder3D
BendPointFinder3D::BendPointFinder3D( const TypeSet<Coord3>& crd,
				      const Coord3& scale, float eps )
    : BendPointFinderBase( crd.size(), eps )
    , coords_( crd.arr() )
    , scale_( scale )
{}


float BendPointFinder3D::getMaxSqDistToLine( int& idx, int start,
					     int stop ) const
{
    if ( stop-start==2 )
    {
	idx = start+1;
	return (float)((coords_[start]+coords_[stop]).scaleBy(scale_)/2).
		sqDistTo( coords_[idx].scaleBy(scale_) );
    }

    const Line3 line( coords_[start].scaleBy(scale_),
		      (coords_[stop]-coords_[start]).scaleBy(scale_) );
    double dsqmax = 0;

    for ( int ipt=start+1; ipt<stop; ipt++ )
    {
	const double dsq =
	line.sqDistanceToPoint( coords_[ipt].scaleBy(scale_) );
	if ( dsq>dsqmax ) { dsqmax = dsq; idx = ipt; }
    }

    return (float) dsqmax;
}
