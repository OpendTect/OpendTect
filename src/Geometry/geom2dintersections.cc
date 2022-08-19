/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geom2dintersections.h"

#include "bendpointfinder.h"
#include "survgeom2d.h"
#include "trigonometry.h"


using namespace PosInfo;


// BendPointFinder2DGeomSet
BendPoints::BendPoints()
    : geomid_(Survey::GeometryManager::cUndefGeomID())
{}


BendPointFinder2DGeomSet::BendPointFinder2DGeomSet(
					const TypeSet<Pos::GeomID>& geomids )
    : Executor("Analyzing 2D Line geometries")
    , geomids_(geomids)
    , curidx_(0)
{
}


od_int64 BendPointFinder2DGeomSet::nrDone() const
{ return curidx_; }

od_int64 BendPointFinder2DGeomSet::totalNr() const
{ return geomids_.size(); }

uiString BendPointFinder2DGeomSet::uiMessage() const
{ return tr("Analyzing 2D Line geometries" ); }

uiString BendPointFinder2DGeomSet::uiNrDoneText() const
{ return tr("Lines done"); }


int BendPointFinder2DGeomSet::nextStep()
{
#define mRetMoreToDo() \
    curidx_++; \
    return MoreToDo();

    if ( curidx_ >= geomids_.size() )
	return Finished();

    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(geomids_[curidx_]))
    if ( !geom2d )
    { mRetMoreToDo(); }

    const float avgtrcdist = geom2d->averageTrcDist();
    if ( mIsUdf(avgtrcdist) || mIsZero(avgtrcdist,1e-3) )
	{ mRetMoreToDo(); }

    BendPointFinder2DGeom bpfinder( geom2d->data().positions(), avgtrcdist );
    if ( !bpfinder.execute() )
	{ mRetMoreToDo(); }

    BendPoints* bp = new BendPoints;
    bp->geomid_ = geomids_[curidx_];
    bp->idxs_ = bpfinder.bendPoints();
    bendptset_ += bp;
    mRetMoreToDo();
}



// Line2DInterSection

Line2DInterSection::Point::Point( Pos::GeomID myid, Pos::GeomID lineid,
				  int mynr,int linenr )
    : line(lineid)
    , mytrcnr(mynr)
    , linetrcnr(linenr)
	, mygeomids_(myid)
{
}


Line2DInterSection::Point::Point( const Point& pt )
    : line(pt.line)
    , mytrcnr(pt.mytrcnr)
    , linetrcnr(pt.linetrcnr)
	, mygeomids_(pt.mygeomids_)
{
}


bool Line2DInterSection::Point::isOpposite( const Point& pt ) const
{
    return mygeomids_==pt.line &&
	   line==pt.mygeomids_ &&
	   mytrcnr==pt.linetrcnr &&
	   linetrcnr==pt.mytrcnr;
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
		   j>=0 && points_[j].mytrcnr>points_[j+d].mytrcnr; j-=d )
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
	if ( points_[idx].line == geomid )
	{
	    index = idx;
	    break;
	}
    }

    if ( index < 0 ) return false;

    mytrcnr = points_[index].mytrcnr;
    crosstrcnr = points_[index].linetrcnr;
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
    lsintersections_.allowNull( true );
    for ( int idx=0; idx<bps.size(); idx++ )
    {
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			Survey::GM().getGeometry(bps[idx]->geomid_))
	if ( !geom2d )
	    break;

	geoms_ += geom2d;
	lsintersections_ += new Line2DInterSection( bps[idx]->geomid_ );
    }
}


od_int64 Line2DInterSectionFinder::nrIterations() const
{ return bendptset_.size()-1; }

uiString Line2DInterSectionFinder::uiMessage() const
{ return tr("Finding intersections" ); }

uiString Line2DInterSectionFinder::uiNrDoneText() const
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
		    if ( interpos == Coord::udf() )
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
