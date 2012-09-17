/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 9-3-1999
-*/
static const char* rcsID = "$Id: bendpointfinder.cc,v 1.5 2010/05/27 14:23:02 cvsjaap Exp $";

#include "bendpointfinder.h"
#include "sorting.h"
#include "trigonometry.h"


BendPointFinderBase::BendPointFinderBase( int sz, float eps )
    : sz_(sz)
    , epssq_(eps*eps)
{}


bool BendPointFinderBase::doPrepare( int nrthreads )
{
    bendpts_ += 0;
    if ( sz_ > 1 )
	bendpts_ += sz_-1;

    queue_.erase();

    queue_ += Interval<int>( 0, sz_-1 );

    finished_ = false;
    nrwaiting_ = 0;
    nrthreads_ = nrthreads;

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
    while ( !finished_ )
    {
	if ( !queue_.size() )
	{
	    nrwaiting_ ++;
	    if ( nrwaiting_==nrthreads_ )
	    {
		finished_ = true;
		lock_.signal( true );
		nrwaiting_--;
		break;
	    }

	    lock_.wait();
	    nrwaiting_ --;
	}

	const int sz = queue_.size();
	if ( !sz )
	    continue;

	const Interval<int> segment = queue_[sz-1];
	queue_.remove( sz-1 );
	lock_.unLock();

	findInSegment( segment.start, segment.stop );

	lock_.lock();
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


BendPointFinder2D::BendPointFinder2D( const TypeSet<Coord>& crd, float eps )
    : BendPointFinderBase( crd.size(), eps )
    , coords_( crd.arr() )
{}


BendPointFinder2D::BendPointFinder2D( const Coord* crd, int sz, float eps )
    : BendPointFinderBase( sz, eps )
    , coords_( crd )
{}


float BendPointFinder2D::getMaxSqDistToLine( int& idx, int start,
					     int stop ) const
{
    if ( stop-start==2 )
    {
	idx = start+1;
	return ((coords_[start]+coords_[stop])/2).sqDistTo( coords_[idx] );
    }

    const Line2 line( coords_[start], coords_[stop] );
    Coord nearestpt;
    double dsqmax = 0;

    for ( int ipt=start+1; ipt<stop; ipt++ )
    {
	const Coord crd( coords_[ipt] );
	nearestpt = line.closestPoint( crd );
	const double dsq = nearestpt.sqDistTo( crd );
	if ( dsq>dsqmax ) { dsqmax = dsq; idx = ipt; }
    }

    return dsqmax;
}



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
	return ((coords_[start]+coords_[stop]).scaleBy(scale_)/2).
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

    return dsqmax;
}
