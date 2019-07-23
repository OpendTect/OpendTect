/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2006
________________________________________________________________________

-*/

#include "geom2dintersections.h"

#include "posinfo2d.h"
#include "survgeom2d.h"
#include "trigonometry.h"


using namespace PosInfo;


// BendPointFinder2DGeomSet
BendPoints::BendPoints()
{}


BendPointFinder2DGeomSet::BendPointFinder2DGeomSet( const GeomIDSet& geomids )
    : Executor("Analyzing 2D Line geometries")
    , geomids_(geomids)
    , curidx_(0)
{
}


od_int64 BendPointFinder2DGeomSet::nrDone() const
{ return curidx_; }

od_int64 BendPointFinder2DGeomSet::totalNr() const
{ return geomids_.size(); }

uiString BendPointFinder2DGeomSet::message() const
{ return tr("Analyzing 2D Line geometries" ); }

uiString BendPointFinder2DGeomSet::nrDoneText() const
{ return tr("Lines done"); }


int BendPointFinder2DGeomSet::nextStep()
{
#define mRetMoreToDo() \
    curidx_++; \
    return MoreToDo();

    if ( curidx_ >= geomids_.size() )
	return Finished();

    const auto& geom2d = SurvGeom::get2D( geomids_[curidx_] );
    if ( geom2d.isEmpty() )
	{ mRetMoreToDo(); }

    BendPoints* bp = new BendPoints;
    bp->geomid_ = geomids_[curidx_];
    bp->idxs_ = geom2d.data().getBendPoints();
    bendptset_ += bp;
    mRetMoreToDo();
}


// Line2DInterSection
Line2DInterSection::Point::Point( Pos::GeomID myid, Pos::GeomID lineid,
				  int mynr,int linenr )
    : myid_(myid)
    , otherid_(lineid)
    , mytrcnr_(mynr)
    , othertrcnr_(linenr)
{
}


bool Line2DInterSection::Point::isOpposite( const Point& pt ) const
{
    return myid_==pt.otherid_ && otherid_==pt.myid_ &&
	   mytrcnr_==pt.othertrcnr_ && othertrcnr_==pt.mytrcnr_;
}


Line2DInterSection::Line2DInterSection( Pos::GeomID geomid )
    : geomid_(geomid)
{}


void Line2DInterSection::sort()
{
    const int sz = points_.size();
    for ( int d=sz/2; d>0; d=d/2 )
    {
	for ( int i=d; i<sz; i++ )
	{
	    for ( int j=i-d;
		   j>=0 && points_[j].mytrcnr_>points_[j+d].mytrcnr_; j-=d )
		points_.swap( j, j+d );
	}
    }
}


bool Line2DInterSection::getIntersectionTrcNrs( Pos::GeomID geomid,
						int& mytrcnr,
						int& crosstrcnr ) const
{
    int index = -1;
    for ( int idx=0; idx<points_.size(); idx++ )
    {
	if ( points_[idx].otherid_ == geomid )
	{
	    index = idx;
	    break;
	}
    }

    if ( index < 0 ) return false;

    mytrcnr = points_[index].mytrcnr_;
    crosstrcnr = points_[index].othertrcnr_;
    return true;
}


void Line2DInterSection::addPoint( Pos::GeomID geomid, int mynr, int linenr )
{
    points_ += Point( geomid_, geomid, mynr, linenr );
}



// Line2DInterSectionSet
const Line2DInterSection* Line2DInterSectionSet::getByGeomID(
						    Pos::GeomID geomid ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Line2DInterSection* isect = (*this)[idx];
	if ( isect && isect->geomID() == geomid )
	    return isect;
    }

    return 0;
}


static bool hasOpposite( const TypeSet<Line2DInterSection::Point>& pts,
			 const Line2DInterSection::Point& pt )
{
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	const bool isopp = pts[idx].isOpposite( pt );
	if ( isopp )
	    return true;
    }

    return false;
}


void Line2DInterSectionSet::getAll(
			TypeSet<Line2DInterSection::Point>& pts ) const
{
    pts.erase();
    for ( int idx=0; idx<size(); idx++ )
    {
	const Line2DInterSection* isect = (*this)[idx];
	if ( !isect ) continue;

	const int nrpts = isect->size();
	for ( int pidx=0; pidx<nrpts; pidx++ )
	{
	    const Line2DInterSection::Point& pt = isect->getPoint( pidx );
	    if ( !hasOpposite(pts,pt) )
		pts.add( pt );
	}
    }
}



// Line2DInterSectionFinder
Line2DInterSectionFinder::Line2DInterSectionFinder(
		const ObjectSet<BendPoints>& bps, Line2DInterSectionSet& lsis )
    : bendptset_(bps)
    , lsintersections_(lsis)
{
    deepErase( lsintersections_ );
    lsintersections_.setNullAllowed( true );
    for ( int idx=0; idx<bps.size(); idx++ )
    {
	const auto& geom2d = SurvGeom::get2D( bps[idx]->geomid_ );
	geoms_ += &geom2d;
	lsintersections_ += new Line2DInterSection( bps[idx]->geomid_ );
    }
}


od_int64 Line2DInterSectionFinder::nrIterations() const
{ return bendptset_.size()-1; }

uiString Line2DInterSectionFinder::message() const
{ return tr("Finding intersections" ); }

uiString Line2DInterSectionFinder::nrDoneText() const
{ return tr("Lines done"); }


bool Line2DInterSectionFinder::doWork( od_int64 start, od_int64 stop, int tid )
{
    const int nrlines = bendptset_.size();
    if ( geoms_.size() != nrlines )
	return false;

    for ( int idx=mCast(int,start); idx<=stop && shouldContinue();
				    idx++, addToNrDone(1) )
    {
	const Line2DData& curl2d = geoms_[idx]->data();
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

	    for ( int lineidx=idx+1; lineidx<nrlines; lineidx++ )
	    {
		if ( idx==lineidx ) continue;

		const Line2DData& l2d = geoms_[lineidx]->data();
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
		    if ( interpos.isUdf() )
			continue;

		    PosInfo::Line2DPos lpos1, lpos2;
		    if ( curl2d.getPos(interpos,lpos1,mUdf(float)) &&
			    l2d.getPos(interpos,lpos2,mUdf(float)) )
		    {
			Threads::Locker lckr( lock_ );
			lsintersections_[idx]->addPoint( geomid2, lpos1.nr_,
							 lpos2.nr_ );
			lsintersections_[lineidx]->addPoint( geomid, lpos2.nr_,
							 lpos1.nr_ );
		    }
		}
	    }
	}
    }

    return true;
}


bool Line2DInterSectionFinder::doFinish( bool success )
{
    for ( int idx=0; idx<lsintersections_.size(); idx++ )
	if ( lsintersections_[idx] )
	    lsintersections_[idx]->sort();

    return true;
}
