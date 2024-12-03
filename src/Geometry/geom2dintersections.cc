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
{}


BendPoints::~BendPoints()
{}


BendPointFinder2DGeomSet::BendPointFinder2DGeomSet(
					const TypeSet<Pos::GeomID>& geomids,
					ObjectSet<BendPoints>& bpts )
    : ParallelTask("Analyzing 2D Line geometries")
    , geomids_(geomids)
    , bendptset_(bpts)
{
    bendptset_.allowNull();
    for ( int idx=0; idx<geomids_.size(); idx++ )
	bendptset_.add( nullptr );
}


BendPointFinder2DGeomSet::~BendPointFinder2DGeomSet()
{}


od_int64 BendPointFinder2DGeomSet::nrIterations() const
{ return geomids_.size(); }

uiString BendPointFinder2DGeomSet::uiMessage() const
{ return tr("Analyzing 2D Line geometries" ); }

uiString BendPointFinder2DGeomSet::uiNrDoneText() const
{ return tr("Lines done"); }


bool BendPointFinder2DGeomSet::doWork( od_int64 start, od_int64 stop,
				       int threadid )
{
    for ( auto idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			Survey::GM().getGeometry(geomids_[idx]))
	if ( !geom2d )
	    continue;

	const float avgtrcdist = geom2d->averageTrcDist();
	if ( mIsUdf(avgtrcdist) || mIsZero(avgtrcdist,1e-3) )
	    continue;

	BendPointFinder2DGeom bpfinder( geom2d->data().positions(),
					avgtrcdist );
	if ( !bpfinder.executeParallel(false) )
	    continue;

	auto* bp = new BendPoints;
	bp->geomid_ = geomids_[idx];
	bp->idxs_ = bpfinder.bendPoints();
	bendptset_.replace( idx, bp );
    }

    return true;
}



// Line2DInterSection

Line2DInterSection::Point::Point( const Pos::GeomID& myid,
				  const Pos::GeomID& lineid,
				  int mynr,int linenr )
    : line(lineid)
    , mygeomids_(myid)
    , mytrcnr(mynr)
    , linetrcnr(linenr)
{
}


Line2DInterSection::Point::Point( const Point& pt )
    : line(pt.line)
    , mygeomids_(pt.mygeomids_)
    , mytrcnr(pt.mytrcnr)
    , linetrcnr(pt.linetrcnr)
{
}

Line2DInterSection::Point::Point( const Pos::GeomID& id, int mynr, int linenr )
    : line(id)
    , mytrcnr(mynr)
    , linetrcnr(linenr)
{}


Line2DInterSection::Point::~Point()
{}


bool Line2DInterSection::Point::isOpposite( const Point& pt ) const
{
    return mygeomids_==pt.line &&
	   line==pt.mygeomids_ &&
	   mytrcnr==pt.linetrcnr &&
	   linetrcnr==pt.mytrcnr;
}


Line2DInterSection::Line2DInterSection( const Pos::GeomID& geomid )
    : geomid_(geomid)
{}


Line2DInterSection::~Line2DInterSection()
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
Line2DInterSectionSet::Line2DInterSectionSet()
{}


Line2DInterSectionSet::~Line2DInterSectionSet()
{}


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
    : ParallelTask("Finding Intersections")
    , bendptset_(bps)
    , lsintersections_(lsis)
{
    deepErase( lsintersections_ );
    lsintersections_.allowNull();
    geoms_.allowNull();

    const Geom::Rectangle<double> udfbox( mUdf(double), mUdf(double),
					  mUdf(double), mUdf(double) );
    bboxs_.setSize( bps.size(), udfbox );
    for ( int idx=0; idx<bps.size(); idx++ )
    {
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			Survey::GM().getGeometry(bps[idx]->geomid_))
	if ( !geom2d )
	{
	    geoms_ += nullptr;
	    lsintersections_+= nullptr;
	    continue;
	}

	geoms_ += geom2d;
	lsintersections_ += new Line2DInterSection( bps[idx]->geomid_ );
	for ( const auto& bpidx : bps[idx]->idxs_ )
	    bboxs_[idx].include( geom2d->data().positions()[bpidx].coord_ );
    }
}


Line2DInterSectionFinder::~Line2DInterSectionFinder()
{}


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

    for ( int curidx=mCast(int,start); curidx<=stop && shouldContinue();
				    curidx++, addToNrDone(1) )
    {
	if ( !geoms_[curidx] )
	    continue;

	const Line2DData& curl2d = geoms_[curidx]->data();
	const TypeSet<PosInfo::Line2DPos>& curposns = curl2d.positions();
	const BendPoints* curbpts = bendptset_[curidx];
	if ( !curbpts )
	    continue;

	const Pos::GeomID& curgeomid = curbpts->geomid_;
	const int curbpsize = curbpts->idxs_.size();


	for ( int lineidx=curidx+1; lineidx<nrlines; lineidx++ )
	{
	    if ( !bboxs_[curidx].intersects(bboxs_[lineidx]) )
		continue;

	    const Line2DData& l2d = geoms_[lineidx]->data();
	    const TypeSet<PosInfo::Line2DPos>& posns = l2d.positions();
	    const BendPoints* bpts = bendptset_[lineidx];
	    if ( !bpts )
		continue;

	    const Pos::GeomID& geomid = bpts->geomid_;
	    const int bpsize = bpts->idxs_.size();

	    for ( int bpidx=1; bpidx<curbpsize; bpidx++ )
	    {
		const int& curposidx1 = curbpts->idxs_[bpidx-1];
		const int& curposidx2 = curbpts->idxs_[bpidx];
		if ( !curposns.validIdx(curposidx1) ||
		     !curposns.validIdx(curposidx2) )
		    continue;

		const Coord& curstart = curposns[curposidx1].coord_;
		const Coord& curstop = curposns[curposidx2].coord_;
		const Line2 curline( curstart, curstop );

		for ( int bpidx2=1; bpidx2<bpsize; bpidx2++ )
		{
		    const int& posidx1 = bpts->idxs_[bpidx2-1];
		    const int& posidx2 = bpts->idxs_[bpidx2];
		    if ( !posns.validIdx(posidx1) ||
			 !posns.validIdx(posidx2) )
			continue;

		    const auto& linestart = posns[posidx1].coord_;
		    const auto& linestop = posns[posidx2].coord_;

		    if ( linestart.x_ > curstop.x_ ||
			 curstart.x_ > linestop.x_ ||
			 linestart.y_ < curstop.y_ ||
			 curstart.y_ < linestop.y_ )
			continue;

		    const Line2 line( linestart, linestop );
		    const Coord interpos = curline.intersection( line );
		    if ( interpos == Coord::udf() )
			continue;

		    PosInfo::Line2DPos lpos1, lpos2;
		    if ( curl2d.getPos(interpos,lpos1,mUdf(float)) &&
			    l2d.getPos(interpos,lpos2,mUdf(float)) )
		    {
			Threads::Locker lckr( lock_ );
			lsintersections_[curidx]->addPoint( geomid,
							    lpos1.nr_,
							    lpos2.nr_ );
			lsintersections_[lineidx]->addPoint( curgeomid,
							     lpos2.nr_,
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



// Line2DIntersectionManager
Line2DIntersectionManager::Line2DIntersectionManager()
{}


Line2DIntersectionManager::~Line2DIntersectionManager()
{
}


const Line2DIntersectionManager& Line2DIntersectionManager::instance()
{
    static PtrMan<Line2DIntersectionManager> mgr =
					new Line2DIntersectionManager;
    return *mgr;
}


Line2DIntersectionManager& Line2DIntersectionManager::instanceAdmin()
{
    return cCast(Line2DIntersectionManager&,instance());
}


bool Line2DIntersectionManager::computeBendpoints( TaskRunner* taskrunner )
{
    if ( !bendpointset_.isEmpty() )
	return true;

    BufferStringSet names;
    TypeSet<Pos::GeomID> geomids;
    Survey::GM().getList( names, geomids, true );
    BendPointFinder2DGeomSet bpfinder( geomids, bendpointset_ );
    if ( !TaskRunner::execute(taskrunner,bpfinder) )
	return false;

    for ( int idx=0; idx<geomids.size(); idx++ )
	geomidmap_[geomids[idx].asInt()] = idx;

    return true;
}


bool Line2DIntersectionManager::compute( TaskRunner* taskrunner )
{
    if ( !intersections_.isEmpty() )
	return true;

    if ( !computeBendpoints(taskrunner) )
	return false;

    Line2DInterSectionFinder finder( bendpointset_, intersections_ );
    if ( !TaskRunner::execute( taskrunner, finder ) )
	return false;

    return true;
}


const Line2DInterSectionSet& Line2DIntersectionManager::intersections() const
{
    return intersections_;
}


const ObjectSet<BendPoints>& Line2DIntersectionManager::bendpoints() const
{
    return bendpointset_;
}


int Line2DIntersectionManager::indexOf( const Pos::GeomID& geomid ) const
{
    auto it = geomidmap_.find( geomid.asInt() );
    if ( it != geomidmap_.end() )
	return it->second;

    return -1;
}


const BendPoints*
    Line2DIntersectionManager::getBendPoints( const Pos::GeomID& geomid ) const
{
    const int idx = indexOf( geomid );
    return bendpointset_.validIdx(idx) ? bendpointset_[idx] : nullptr;
}


const Line2DInterSection*
    Line2DIntersectionManager::getIntersection( const Pos::GeomID& geomid) const
{
    const int idx = indexOf( geomid );
    return intersections_.validIdx(idx) ? intersections_[idx] : nullptr;
}
