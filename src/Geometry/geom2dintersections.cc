/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "geom2dintersections.h"

#include "bendpointfinder.h"
#include "linesetposinfo.h"
#include "survgeom2d.h"
#include "trigonometry.h"


using namespace PosInfo;

BendPoints::BendPoints()
    : geomid_(Survey::GeometryManager::cUndefGeomID())
{}


BendPointFinder::BendPointFinder( const LineSet2DData& geom,
				  const TypeSet<Pos::GeomID>& geomids )
    : geometry_(geom)
    , geomids_(geomids)
{
    bendptset_.allowNull();
    for ( int idx=0; idx<geom.nrLines(); idx++ )
	bendptset_ += 0;
}


od_int64 BendPointFinder::nrIterations() const
{ return geometry_.nrLines(); }

uiString BendPointFinder::uiMessage() const
{ return tr("Finding bendpoints" ); }

uiString BendPointFinder::uiNrDoneText() const
{ return tr("Lines done"); }


bool BendPointFinder::doWork( od_int64 start, od_int64 stop, int threadid )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	const Line2DData& l2d = geometry_.lineData( idx );
	const TypeSet<PosInfo::Line2DPos>& posns = l2d.positions();
	BendPoints* bp = new BendPoints;
	bp->geomid_ = geomids_[idx];
	bendptset_.replace( idx, bp );

	const int nrpos = posns.size();
	TypeSet<Coord> coords;
	coords.setCapacity( nrpos, false );
	for ( int posidx=0; posidx<nrpos; posidx++ )
	    coords += posns[posidx].coord_;

	float max, median; max = median = mUdf(float);
	l2d.compDistBetwTrcsStats( max, median );
	if ( mIsZero(median,mDefEps) || mIsUdf(median) ) median = 25;
	BendPointFinder2D finder( coords, median );
	finder.executeParallel( false );
	bp->idxs_ = finder.bendPoints();
    }

    return true;
}


void InterSections::sort()
{
    const int sz = mytrcnrs_.size();
    for ( int d=sz/2; d>0; d=d/2 )
    {
	for ( int i=d; i<sz; i++ )
	{
	    for ( int j=i-d; j>=0 && mytrcnrs_[j]>mytrcnrs_[j+d]; j-=d )
	    {
		mytrcnrs_.swap( j, j+d );
		crossgeomids_.swap( j, j+d );
		crosstrcnrs_.swap( j, j+d );
	    }
	}
    }
}


bool InterSections::getIntersectionTrcNrs( Pos::GeomID geomid, int& mytrcnr,
					   int& crosstrcnr ) const
{
    const int idx = crossgeomids_.indexOf( geomid );
    if ( idx<0 ) return false;

    mytrcnr = mytrcnrs_[idx];
    crosstrcnr = crosstrcnrs_[idx];
    return true;
}



const InterSections* LineSetInterSections::get( Pos::GeomID geomid ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const InterSections* isect = (*this)[idx];
	if ( isect && isect->geomid_ == geomid )
	    return isect;
    }

    return 0;
}


InterSectionFinder::InterSectionFinder( const PosInfo::LineSet2DData& geom,
					const ObjectSet<BendPoints>& bps,
					LineSetInterSections& lsis )
    : bendptset_(bps)
    , geometry_(geom)
    , lsintersections_(lsis)
{
    lsintersections_.allowNull( true );
    for ( int idx=0; idx<geom.nrLines(); idx++ )
	lsintersections_ += 0;
}


od_int64 InterSectionFinder::nrIterations() const
{ return bendptset_.size()-1; }

uiString InterSectionFinder::uiMessage() const
{ return tr("Finding intersections" ); }

uiString InterSectionFinder::uiNrDoneText() const
{ return tr("Lines done"); }


bool InterSectionFinder::doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int linesize = geometry_.nrLines();
    for ( int idx=mCast(int,start); idx<=stop && shouldContinue();
	    idx++, addToNrDone(1) )
    {
	const Line2DData& curl2d = geometry_.lineData( idx );
	const TypeSet<PosInfo::Line2DPos>& curposns = curl2d.positions();
	const BendPoints* curbpts = bendptset_[idx];
	if ( !curbpts ) continue;

	Pos::GeomID geomid = curbpts->geomid_;
	const int curbpsize = curbpts->idxs_.size();
	for ( int bpidx=1; bpidx<curbpsize; bpidx++ )
	{
	    int posidx1 = curbpts->idxs_[bpidx-1];
	    int posidx2 = curbpts->idxs_[bpidx];
	    if ( !curposns.validIdx(posidx1) ||
		 !curposns.validIdx(posidx2) )
		continue;

	    Coord crd1 = curposns[posidx1].coord_;
	    Coord crd2 = curposns[posidx2].coord_;
	    Line2 curline( crd1, crd2 );

	    for ( int lineidx=idx+1; lineidx<linesize; lineidx++ )
	    {
		if ( idx==lineidx ) continue;

		const Line2DData& l2d = geometry_.lineData( lineidx );
		const TypeSet<PosInfo::Line2DPos>& posns = l2d.positions();
		const BendPoints* bpts = bendptset_[lineidx];
		if ( !bpts ) continue;

		Pos::GeomID geomid2 = bpts->geomid_;
		const int bpsize = bpts->idxs_.size();
		for ( int bpidx2=1; bpidx2<bpsize; bpidx2++ )
		{
		    posidx1 = bpts->idxs_[bpidx2-1];
		    posidx2 = bpts->idxs_[bpidx2];
		    if ( !posns.validIdx(posidx1) ||
			 !posns.validIdx(posidx2) )
			continue;

		    crd1 = posns[posidx1].coord_;
		    crd2 = posns[posidx2].coord_;
		    Line2 line2( crd1, crd2 );
		    Coord interpos = curline.intersection( line2 );
		    if ( interpos == Coord::udf() )
			continue;

		    Threads::Locker lckr( lock_ );
		    if ( !lsintersections_[idx] )
			lsintersections_.replace( idx,
						  new InterSections(geomid) );
		    if ( !lsintersections_[lineidx] )
			lsintersections_.replace( lineidx,
						  new InterSections(geomid2) );

		    PosInfo::Line2DPos lpos1, lpos2;
		    if ( curl2d.getPos(interpos,lpos1,mUdf(float)) &&
			    l2d.getPos(interpos,lpos2,mUdf(float)) )
		    {
			lsintersections_[idx]->mytrcnrs_ += lpos1.nr_;
			lsintersections_[idx]->crossgeomids_.add( geomid2 );
			lsintersections_[idx]->crosstrcnrs_ += lpos2.nr_;

			lsintersections_[lineidx]->mytrcnrs_ += lpos2.nr_;
			lsintersections_[lineidx]->crossgeomids_.add(geomid);
			lsintersections_[lineidx]->crosstrcnrs_ += lpos1.nr_;
		    }
		}
	    }
	}
    }

    return true;
}


bool InterSectionFinder::doFinish( bool success )
{
    for ( int idx=0; idx<lsintersections_.size(); idx++ )
	if ( lsintersections_[idx] )
	    lsintersections_[idx]->sort();

    return true;
}
