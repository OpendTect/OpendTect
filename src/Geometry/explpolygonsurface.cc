/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: explpolygonsurface.cc,v 1.6 2008-09-25 17:15:59 cvsyuancheng Exp $";

#include "explpolygonsurface.h"

#include "delaunay3d.h"
#include "polygonsurface.h"
#include "positionlist.h"
#include "trigonometry.h"


namespace Geometry {


ExplPolygonSurface::ExplPolygonSurface( const PolygonSurface* surf )
    : bodytriangle_( 0 )
    , tetrahedratree_( 0 )  
    , polygondisplay_( 0 )			 
    , displaypolygons_( true )
    , displaybody_( true )
    , needsupdate_( true )			  
{
    setSurface( surf );
}


ExplPolygonSurface::~ExplPolygonSurface()
{
    delete tetrahedratree_;
    setSurface( 0 );
}


void ExplPolygonSurface::setSurface( const PolygonSurface* psurf )
{
    removeAll();
    surface_ = psurf;
    needsupdate_ = true;
}


void ExplPolygonSurface::removeAll()
{
    if ( polygondisplay_ )
    {
    	removeFromGeometries( polygondisplay_ );
    	delete polygondisplay_;
    	polygondisplay_ = 0;
    }

    if ( bodytriangle_ )
    {
    	removeFromGeometries( bodytriangle_ );
    	delete bodytriangle_;
    	bodytriangle_ = 0;
    }
}



bool ExplPolygonSurface::update( bool forceall, TaskRunner* tr )
{
    if ( forceall )
	removeAll();

    if ( !surface_ || !needsupdate_ )
	return true;

    const StepInterval<int> rrg = surface_->rowRange();
    
    TypeSet<Coord3> pts;
    TypeSet<int> plgcrdindices; 
    int prevnrknots = 0;
    for ( int plg=rrg.start; plg<=rrg.stop; plg += rrg.step )
    {
	prevnrknots = pts.size();
	surface_->getCubicBezierCurve( plg, pts );

	if ( displaypolygons_ )
	{
    	    for ( int knotidx=prevnrknots; knotidx<pts.size(); knotidx++ )
    		plgcrdindices += knotidx;
	    
    	    plgcrdindices += prevnrknots;
    	    plgcrdindices += -1;
	}
    }

    for ( int idx=0; idx<pts.size(); idx++ )
	coordlist_->set( idx, pts[idx] );
    
    updateGeometries();

    if ( displaypolygons_ )
    {
	polygondisplay_->coordindices_.erase();
	for ( int idx=0; idx<plgcrdindices.size(); idx++ )
    	    polygondisplay_->coordindices_ += plgcrdindices[idx];

	polygondisplay_->ischanged_ = true;
    }
  
    if ( displaybody_ && !updateBodyDisplay(pts) )
	return false;

    setRightHandedNormals( true );
    needsupdate_ = true;
    return true;
}


bool ExplPolygonSurface::updateBodyDisplay( const TypeSet<Coord3>&  pts )
{
    if ( pts.size()<4 || surface_->nrPolygons()<2 )
	return true;

    if ( !tetrahedratree_ ) 
      tetrahedratree_ = new DAGTetrahedraTree;	
   
    if ( !tetrahedratree_->setCoordList( pts, false ) )
	return false;

    ParallelDTetrahedralator triangulator( *tetrahedratree_ );
    triangulator.dataIsRandom( true );
    if ( !triangulator.execute(true) )
	return false;

    TypeSet<int> triangles;
    tetrahedratree_->getSurfaceTriangles( triangles );
    //tetrahedratree_->getTetrahedraTriangles( triangles );
   
    bodytriangle_->coordindices_.erase();
    for ( int idx=0; idx<triangles.size()/3; idx++ )
    {
	bodytriangle_->coordindices_ += triangles[3*idx];
	bodytriangle_->coordindices_ += triangles[3*idx+1];
	bodytriangle_->coordindices_ += triangles[3*idx+2];
	bodytriangle_->coordindices_ += -1;
    }

    bodytriangle_->ischanged_ = true;
    return true;
}


void ExplPolygonSurface::display( bool polygons, bool body )
{
    if ( displaypolygons_==polygons && displaybody_==body )
	return;

    displaypolygons_ = polygons;
    displaybody_ = body;
    update( false, 0 );
}


void ExplPolygonSurface::addToGeometries( IndexedGeometry* ig )
{
    if ( !ig ) return;
    geometrieslock_.writeLock();
    if ( geometries_.indexOf( ig )!=-1 )
	pErrMsg("Adding more than once");

    ig->ischanged_ = true;
    geometries_ += ig;
    geometrieslock_.writeUnLock();
}


void ExplPolygonSurface::removeFromGeometries( const IndexedGeometry* ig )
{
    if ( !ig ) return;
    geometrieslock_.writeLock();
    const int idx = geometries_.indexOf( ig );

    if ( idx!=-1 )
	geometries_.removeFast( idx );

    geometrieslock_.writeUnLock();
}


void ExplPolygonSurface::setRightHandedNormals( bool yn )
{
    if ( yn==righthandednormals_ )
	return;

    IndexedShape::setRightHandedNormals( yn );

    if ( !normallist_ )
	return;

    int id = -1;
    while ( true )
    {
	id = normallist_->nextID( id );
	if ( id==-1 )
	    break;

	normallist_->set( id, -normallist_->get( id ) );
    }
}


void ExplPolygonSurface::updateGeometries()
{
    if ( !displaybody_ && bodytriangle_ )
    {
	removeFromGeometries( bodytriangle_ );
    }
    else if ( !bodytriangle_ && displaybody_ )
    {
	bodytriangle_ = new IndexedGeometry( IndexedGeometry::TriangleStrip,
	       IndexedGeometry::PerVertex, coordlist_, normallist_, 0 );

	addToGeometries( bodytriangle_ );
    }
    
    if ( !displaypolygons_ && polygondisplay_ )
    {
	removeFromGeometries( polygondisplay_ );
    }
    else if ( !polygondisplay_ && displaypolygons_ )
    {
	polygondisplay_ = new IndexedGeometry( IndexedGeometry::Lines, 
		IndexedGeometry::PerFace, coordlist_, normallist_, 0 );

	addToGeometries( polygondisplay_ );
    }
}

}; // namespace Geometry
