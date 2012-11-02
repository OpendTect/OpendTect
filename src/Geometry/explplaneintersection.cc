/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : October 2007
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "explplaneintersection.h"

#include "linerectangleclipper.h"
#include "multidimstorage.h"
#include "polygon.h"
#include "positionlist.h"
#include "survinfo.h"
#include "task.h"
#include "trigonometry.h"

namespace Geometry {


struct ExplPlaneIntersectionExtractorPlane
{
    ExplPlaneIntersectionExtractorPlane( const Coord3& normal,
					 const TypeSet<Coord3>& pointsonplane )
	: planecoordsys_( normal, pointsonplane[0], pointsonplane[1] )
    {
	Interval<double> xrg, yrg;
	for ( int idx=0; idx<pointsonplane.size(); idx++ )
	{
	    const Coord pt = planecoordsys_.transform(pointsonplane[idx],false);
	    polygon_.add( pt );
	    if ( !idx )
	    {
		xrg.start = xrg.stop = pt.x;
		yrg.start = yrg.stop = pt.y;
		continue;
	    }

	    xrg.start = mMIN(xrg.start,pt.x); xrg.stop = mMAX(xrg.stop,pt.x);
	    yrg.start = mMIN(yrg.start,pt.y); yrg.stop = mMAX(yrg.stop,pt.y);
	}

	bbox_.setTopLeft( Coord(xrg.start, yrg.stop) );
	bbox_.setBottomRight( Coord(xrg.stop, yrg.start) );
    }

    void cutLine( const Line3& line, double& t0, double& t1,
	          bool& t0change, bool& t1change, const Coord3 center )
    {
	LineRectangleClipper<double> clipper( bbox_ );
	clipper.setLine( 
		planecoordsys_.transform( line.getPoint(t0)+center, false ),
		planecoordsys_.transform( line.getPoint(t1)+center, false) );

	if ( clipper.isStartChanged() )
	{
	    t0 = line.closestPoint(
		    planecoordsys_.transform(clipper.getStart())-center );
	    t0change = true;
	}
	else
	    t0change = false;

	if ( clipper.isStopChanged() )
	{
	    t1 = line.closestPoint(
		    planecoordsys_.transform(clipper.getStop())-center );
	    t1change = true;
	}
	else
	    t1change = false;
    }


    bool isInside( const Coord3& pt ) const
    {
	return polygon_.isInside(planecoordsys_.transform(pt,false), true, 0);
    }

    ODPolygon<double>		polygon_;
    Plane3CoordSystem		planecoordsys_;
    Geom::Rectangle<double>	bbox_;
};



class ExplPlaneIntersectionExtractor : public ParallelTask
{
public:
ExplPlaneIntersectionExtractor( ExplPlaneIntersection& efss )
    : explsurf_( efss )
    , intersectioncoordids_( 3, 1 )
{
    planes_.allowNull( true );
    explsurf_.pis_.erase();

    for ( int idx=0; idx<efss.nrPlanes(); idx++ )
    {
	const int planeid = explsurf_.planeID( idx );
	const TypeSet<Coord3>& pointsonplane = explsurf_.planePolygon(planeid);
	if ( pointsonplane.size()<3 )
	{
	    planes_ += 0;
	}
	else
	{
	    planes_ += new ExplPlaneIntersectionExtractorPlane(
		    explsurf_.planeNormal( planeid ), pointsonplane );
	}

	ExplPlaneIntersection::PlaneIntersection pi;
	explsurf_.pis_ += pi;
    }

    intersectioncoordids_.allowDuplicates( false );
    output_ = const_cast<IndexedGeometry*>( explsurf_.getGeometry()[0] );
    output_->removeAll( true );
    addedposes_.erase();
}


~ExplPlaneIntersectionExtractor()
{
    deepErase( planes_ );
}

    
od_int64 nrIterations() const
{
    return explsurf_.getShape()->getGeometry().size();
}


#define mStick	0
#define mKnot	1

bool doWork( od_int64 start, od_int64 stop, int )
{
    if ( !explsurf_.nrPlanes() )
	return false;

    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	const IndexedGeometry* inputgeom =
	    explsurf_.getShape()->getGeometry()[idx];

	Threads::MutexLocker inputlock( inputgeom->lock_ );

	if ( inputgeom->type_==IndexedGeometry::Triangles )
	{
	    for ( int idy=0; idy<inputgeom->coordindices_.size()-2; idy+=3 )
	    {
		intersectTriangle( inputgeom->coordindices_[idy],
				   inputgeom->coordindices_[idy+1],
				   inputgeom->coordindices_[idy+2] );
	    }
	}
	else if ( inputgeom->type_==IndexedGeometry::TriangleStrip )
	{
	    for ( int idy=0; idy<inputgeom->coordindices_.size(); idy++ )
	    {
		if ( inputgeom->coordindices_[idy]==-1 )
		{
		    idy += 2;
		    continue;
		}

		if ( idy<2 )
		    continue;

		intersectTriangle( inputgeom->coordindices_[idy-2],
				   inputgeom->coordindices_[idy-1],
				   inputgeom->coordindices_[idy] );
	    }
	}
	else if ( inputgeom->type_==IndexedGeometry::TriangleFan )
	{
	    int center = 0;
	    for ( int idy=0; idy<inputgeom->coordindices_.size(); idy++ )
	    {
		if ( inputgeom->coordindices_[idy]==-1 )
		{
		    center = idy+1;
		    idy += 2;
		    continue;
		}

		if ( idy<2 )
		    continue;

		intersectTriangle( inputgeom->coordindices_[center],
				   inputgeom->coordindices_[idy-1],
				   inputgeom->coordindices_[idy] );
	    }
	}
    }

    return true;
}


bool addNewPos( const Coord3& point, int& residx ) 
{
    addedposlock_.readLock(); 
    int tmpposidx = addedposes_.indexOf(point);
    
    if ( tmpposidx>=0 ) 
    { 
	residx = crdlistidx_[tmpposidx]; 
    	addedposlock_.readUnLock(); 
	return false;
    } 
 
    if ( !addedposlock_.convReadToWriteLock() ) 
    { 
	tmpposidx = addedposes_.indexOf(point); 
	if ( tmpposidx>=0 ) 
	{
	    residx = crdlistidx_[tmpposidx]; 
	    addedposlock_.writeUnLock(); 
	    return false;
	}
    } 
 
    residx = explsurf_.coordList()->add( point ); 
    addedposes_ += point; 
    crdlistidx_ += residx; 

    addedposlock_.writeUnLock();
    return true; 
}


void intersectTriangle( int lci0, int lci1, int lci2 ) 
{
    RefMan<const Coord3List> coordlist = explsurf_.getShape()->coordList();
    const float zscale = explsurf_.getZScale();

    Coord3 c0 = coordlist->get( lci0 ); c0.z *= zscale;
    Coord3 c1 = coordlist->get( lci1 ); c1.z *= zscale;
    Coord3 c2 = coordlist->get( lci2 ); c2.z *= zscale;
    const Coord3 centerpt = (c0+c1+c2)/3;
    c0 -= centerpt;
    c1 -= centerpt;
    c2 -= centerpt;

    const Coord3 trianglenormal = ((c1-c0).cross(c2-c0)).normalize();
    const Plane3 triangleplane( trianglenormal, c0, false );

    for ( int planeidx=explsurf_.nrPlanes()-1; planeidx>=0; planeidx-- )
    {
	const int planeid = explsurf_.planeID( planeidx );
	const Coord3 planenormal = explsurf_.planeNormal( planeid );
	if ( mIsEqual( fabs(planenormal.dot(trianglenormal)), 1, 1e-3 ) )
	    continue;
	
	const Coord3 ptonplane = explsurf_.planePolygon( planeid )[0]-centerpt;
	Plane3 plane( planenormal, ptonplane, false);
	Line3 intersectionline;
	if ( !triangleplane.intersectWith( plane, intersectionline ) )
	    continue;

#define mEdge01 0
#define mEdge12 1
#define mEdge20 2

#define mCheckEdge( edgeidx ) \
    intersectionline.closestPoint(edge##edgeidx,t[mEdge##edgeidx], \
	    			  edget##edgeidx);  \
	edgeok##edgeidx = edget##edgeidx<1+1e-2 && edget##edgeidx>-1e-2

	double t[3];
	int startedge=-1, stopedge=-1;

	const Line3 edge01( c0, c1-c0 );
	const Line3 edge12( c1, c2-c1 );
	const Line3 edge20( c2, c0-c2 );

	double edget01,edget12,edget20;
	bool edgeok01,edgeok12,edgeok20;
	mCheckEdge( 01 );
	mCheckEdge( 12 );
	mCheckEdge( 20 );

	const int nrintersections = edgeok01+edgeok12+edgeok20;

	if ( nrintersections<2 )
	    continue;

	if ( nrintersections==3 )//Round error case handle
	{
	    const float d01 = (float) plane.distanceToPoint(edge01.getPoint(edget01));
    	    const float d12 = (float) plane.distanceToPoint(edge12.getPoint(edget12));
    	    const float d20 = (float) plane.distanceToPoint(edge20.getPoint(edget20));

	    if ( mIsZero(d01,1e-5) && mIsZero(d12,1e-5) && mIsZero(d20,1e-5) )
	    {
		const float d0 = (float) (plane.getProjection(c0)-c0).sqAbs();
		const float d1 = (float) (plane.getProjection(c1)-c1).sqAbs();
		const float d2 = (float) (plane.getProjection(c2)-c2).sqAbs();
		if ( d0>d1 && d0>d2 )
		    edgeok12 = false;
		else if ( d1>d0 && d1>d2 )
		    edgeok20 = false;
		else
		    edgeok01 = false;
	    }
	    else
	    {
		if ( d01>d12 && d01>d20 )
		    edgeok01 = false;
		else if ( d12>d01 && d12>d20 )
		    edgeok12 = false;
		else
		    edgeok20 = false;
	    }
	}

	if ( edgeok01 && edgeok12 )
	{
	    startedge = mEdge01;
	    stopedge = mEdge12;
	}
	else
	{
	    startedge = edgeok01 ? mEdge01 : mEdge12;
	    stopedge = mEdge20;
	}

	if ( startedge==-1 )
	    continue;

	bool startcut = false, stopcut = false;
	Coord3 startpt = intersectionline.getPoint( t[startedge] ) + centerpt;
	Coord3 stoppt = intersectionline.getPoint( t[stopedge] ) + centerpt;
	if ( planes_[planeidx] )
	{
	    const char sum = planes_[planeidx]->isInside( startpt ) +
			     planes_[planeidx]->isInside( stoppt );
	    if ( sum==0 )
		continue;

	    if ( sum!=2 )
	    {
		planes_[planeidx]->cutLine( intersectionline, t[startedge],
			t[stopedge], startcut, stopcut, centerpt );
	    }
	}

#define mSetArrPos( tidx ) \
    int arraypos[3]; \
    arraypos[0] = planeid; \
    if ( tidx==0 ) \
    { arraypos[1]=mMIN(lci0,lci1); arraypos[2]=mMAX(lci0,lci1); } \
    else if ( tidx==1 ) \
    { arraypos[1]=mMIN(lci1,lci2); arraypos[2]=mMAX(lci1,lci2); } \
    else { arraypos[1]=mMIN(lci2,lci0); arraypos[2]=mMAX(lci2,lci0); } 

	int ci0;
	startpt.z /= zscale;
	if ( !startcut )
	{
	    mSetArrPos( startedge )
	    ci0 = getCoordIndex( arraypos, startpt, *explsurf_.coordList() );
	}
	else
	{
	    addNewPos( startpt, ci0 );
	}

	int ci1;
	stoppt.z /= zscale;
	if ( !stopcut )
	{
	    mSetArrPos( stopedge )
	    ci1 = getCoordIndex( arraypos, stoppt, *explsurf_.coordList() );
	}
	else
	{
	    addNewPos( stoppt, ci1 );
	}

	if ( ci0==ci1 || ci0<0 || ci1<0 )
	    continue;

	Threads::MutexLocker reslock( output_->lock_ );

	if ( output_->coordindices_.size() )
	    output_->coordindices_ += -1;

	output_->coordindices_ += ci0;
	output_->coordindices_ += ci1;
	output_->ischanged_ = true;

	TypeSet<Coord3>& crds = explsurf_.pis_[planeidx].knots_;
	TypeSet<int>& conns = explsurf_.pis_[planeidx].conns_;
	
	int id0 = crds.indexOf( startpt );
	if ( id0==-1 )
	{
	    crds += startpt;
	    id0 = crds.size()-1;
	}
	int id1 = crds.indexOf( stoppt );
	if ( id1==-1 )
	{
	    crds += stoppt;
	    id1 = crds.size()-1;
	}

        conns += id0;
        conns += id1;
        conns += -1;	

	/*
	const int ci0idx = output_->coordindices_.indexOf( ci0 );
	if ( ci0idx==-1 )
	{
	    const int ci1idx = output_->coordindices_.indexOf( ci1 );
	    if ( ci1idx==-1 )
	    {
		if ( output_->coordindices_.size() )
		    output_->coordindices_ += -1;

		output_->coordindices_ += ci0;
		output_->coordindices_ += ci1;
		output_->ischanged_ = true;
	    }
	    else if ( ci1idx==0 || output_->coordindices_[ci1idx-1]==-1 )
	    {
		output_->coordindices_.insert( ci1idx, ci0 );
		output_->ischanged_ = true;
	    }
	    else if ( ci1idx==output_->coordindices_.size()-1 ||
		      output_->coordindices_[ci1idx+1]==-1 )
	    {
		output_->coordindices_.insert( ci1idx+1, ci0 );
		output_->ischanged_ = true;
	    }
	    else
		pErrMsg("Hmm");
	}
	else if ( ci0idx==0 || output_->coordindices_[ci0idx-1]==-1 )
	{
	    output_->coordindices_.insert( ci0idx, ci1 );
	    output_->ischanged_ = true;
	}
	else if ( ci0idx==output_->coordindices_.size()-1 ||
		  output_->coordindices_[ci0idx+1]==-1 )
	{
	    output_->coordindices_.insert( ci0idx+1, ci1 );
	    output_->ischanged_ = true;
	}
	else
	    pErrMsg("Hmm");*/
    }
}


int getCoordIndex( const int* arraypos, const Coord3& point, 
		   Coord3List& coordlist )
{
    tablelock_.readLock();
   
    int res; 
    int idxs[3];
    if ( intersectioncoordids_.findFirst( arraypos, idxs ) )
    {
	res = intersectioncoordids_.getRef( idxs, 0 );
	tablelock_.readUnLock();
	return res;
    }

    if ( !tablelock_.convReadToWriteLock() )
    {
	if ( intersectioncoordids_.findFirst( arraypos, idxs ) )
	{
	    res = intersectioncoordids_.getRef( idxs, 0 );
	    tablelock_.writeUnLock();
	    return res;
	}
    }

    if ( addNewPos(point,res) )
	intersectioncoordids_.add( &res, arraypos );

    tablelock_.writeUnLock();
    return res;
}

    MultiDimStorage<int>				intersectioncoordids_;
    Threads::ReadWriteLock				tablelock_;

    IndexedGeometry*					output_;
    ObjectSet<ExplPlaneIntersectionExtractorPlane>	planes_;
    ExplPlaneIntersection&				explsurf_;

    Threads::ReadWriteLock				addedposlock_;
    TypeSet<Coord3>					addedposes_;
    TypeSet<int>					crdlistidx_;
};


ExplPlaneIntersection::ExplPlaneIntersection()
    : needsupdate_( true )
    , intersection_( 0 )
    , shape_( 0 )
    , shapeversion_( -1 )
    , zscale_( mCast( float, SI().zDomain().userFactor() ) )
{ }


ExplPlaneIntersection::~ExplPlaneIntersection()
{
    deepErase( planepts_ );
}


void ExplPlaneIntersection::removeAll( bool deep )
{
    intersection_ = 0;
    IndexedShape::removeAll( deep );
}


int ExplPlaneIntersection::nrPlanes() const
{ return planeids_.size(); }


int ExplPlaneIntersection::planeID( int idx ) const
{ return planeids_[idx]; }


const Coord3& ExplPlaneIntersection::planeNormal( int id ) const
{ return planenormals_[planeids_.indexOf(id)]; }


const TypeSet<Coord3>& ExplPlaneIntersection::planePolygon( int id ) const
{ return *planepts_[planeids_.indexOf(id)]; }


int ExplPlaneIntersection::addPlane( const Coord3& normal,
				     const TypeSet<Coord3>& pts )
{
    if ( pts.size()<1 )
	return -1;

    int id = 0;
    while ( planeids_.indexOf(id)!=-1 ) id++;

    planeids_ += id;
    planenormals_ += normal;
    planepts_+= new TypeSet<Coord3>( pts );

    const int idx = planeids_.size()-1;
    for ( int idy=0; idy<planepts_[idx]->size(); idy++ )
	(*planepts_[idx])[idy].z *= zscale_;

    needsupdate_ = true;

    return id;
}


bool ExplPlaneIntersection::setPlane( int id, const Coord3& normal,
				      const TypeSet<Coord3>& pts )
{
    if ( pts.size()<1 )
	return false;

    const int idx = planeids_.indexOf( id );

    planenormals_[idx] = normal;
    *planepts_[idx] = pts;

    for ( int idy=0; idy<planepts_[idx]->size(); idy++ )
	(*planepts_[idx])[idy].z *= zscale_;

    needsupdate_ = true;
    return true;
}


void ExplPlaneIntersection::removePlane( int id )
{
    const int idx = planeids_.indexOf( id );

    planeids_.removeSingle( idx, false );
    delete planepts_.removeSingle( idx, false );
    planenormals_.removeSingle( idx, false );

    needsupdate_ = true;
}


void ExplPlaneIntersection::setShape( const IndexedShape& is )
{
    shape_ = &is;
    needsupdate_ = true;
}


const IndexedShape* ExplPlaneIntersection::getShape() const
{ return shape_; }


bool ExplPlaneIntersection::needsUpdate() const
{
    if ( needsupdate_ )
	return true;

    if ( !shape_ )
	return false;

    return shape_->getVersion()!=shapeversion_;
}


bool ExplPlaneIntersection::update( bool forceall, TaskRunner* tr )
{
    if ( !forceall && !needsUpdate() )
	return true;
    
    if ( !intersection_ )
    {
	intersection_ = new IndexedGeometry( IndexedGeometry::Lines,
					     IndexedGeometry::PerFace,
					     coordlist_ );
	geometrieslock_.writeLock();
	geometries_ += intersection_;
	geometrieslock_.writeUnLock();
    }

    PtrMan<Task> updater = new ExplPlaneIntersectionExtractor( *this );

    if ( (tr && !tr->execute( *updater ) ) || (!tr && !updater->execute()) )
	return false;

    shapeversion_ = shape_->getVersion();
    needsupdate_ = true;
    return true;
}



}; // namespace Geometry
