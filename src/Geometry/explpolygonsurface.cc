/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : July 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "explpolygonsurface.h"

#include "delaunay3d.h"
#include "polygonsurface.h"
#include "positionlist.h"
#include "trigonometry.h"

namespace Geometry {


ExplPolygonSurface::ExplPolygonSurface( const PolygonSurface* surf, 
					float zscale )
    : bodytriangle_( 0 )
    , tetrahedratree_( 0 )  
    , polygondisplay_( 0 )			 
    , displaypolygons_( true )
    , displaybody_( true )
    , scalefacs_( 1, 1, zscale)			  
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
    removeAll( true );
    surface_ = psurf;
    needsupdate_ = true;
}


void ExplPolygonSurface::setZScale( float zscale )
{
    scalefacs_.z = zscale;
}


void ExplPolygonSurface::removeAll( bool deep )
{
    if ( polygondisplay_ && deep )
	removeFromGeometries( polygondisplay_ );
    
    delete polygondisplay_;
    polygondisplay_ = 0;

    if ( bodytriangle_ && deep )
	removeFromGeometries( bodytriangle_ );

    delete bodytriangle_;
    bodytriangle_ = 0;
}


bool ExplPolygonSurface::update( bool forceall, TaskRunner* tr )
{
    if ( forceall )
	removeAll( true );

    if ( !surface_ || !needsupdate_ )
	return true;

    const StepInterval<int> rrg = surface_->rowRange();
    if ( rrg.isUdf() )
	return true;
    
    samples_.erase();
    TypeSet<int> plgcrdindices; 
    int prevnrknots = 0;
    for ( int plg=rrg.start; plg<=rrg.stop; plg += rrg.step )
    {
	prevnrknots = samples_.size();
	surface_->getCubicBezierCurve( plg, samples_, (float)scalefacs_.z );

	if ( displaypolygons_ )
	{
    	    for ( int knotidx=prevnrknots; knotidx<samples_.size(); knotidx++ )
    		plgcrdindices += knotidx;
	    
    	    plgcrdindices += prevnrknots;
    	    plgcrdindices += -1;
	}
    }

    for ( int idx=0; idx<samples_.size(); idx++ )
	coordlist_->set( idx, samples_[idx] );
    
    updateGeometries();

    if ( displaypolygons_ )
    {
	polygondisplay_->coordindices_.erase();
	for ( int idx=0; idx<plgcrdindices.size(); idx++ )
    	    polygondisplay_->coordindices_ += plgcrdindices[idx];

	polygondisplay_->ischanged_ = true;
    }
  
    if ( displaybody_ && !updateBodyDisplay() )
	return false;

    setRightHandedNormals( true );
    needsupdate_ = true;
    return true;
}


bool ExplPolygonSurface::updateBodyDisplay()
{
    const int sampsz = samples_.size();
    if ( sampsz<3 || surface_->nrPolygons()<2 )
	return true;

    if ( sampsz==3 )
    {
	bodytriangle_->coordindices_.erase();
 	bodytriangle_->coordindices_ += 0;
 	bodytriangle_->coordindices_ += 1;
 	bodytriangle_->coordindices_ += 2;
       	bodytriangle_->coordindices_ += -1;
 	bodytriangle_->ischanged_ = true;
 	return true;	
    }

    if ( !tetrahedratree_ ) 
      tetrahedratree_ = new DAGTetrahedraTree;	
  
    TypeSet<Coord3> pts;
    for ( int idx=0; idx<sampsz; idx++ )
	pts += samples_[idx].scaleBy(scalefacs_);

    if ( !tetrahedratree_->setCoordList( pts, false ) )
	return false;

    ParallelDTetrahedralator triangulator( *tetrahedratree_ );
    if ( !triangulator.execute(true) )
	return false;

    sampleindices_.erase();
    tetrahedratree_->getSurfaceTriangles( sampleindices_ );
    
    const int nrindices = sampleindices_.size();
    TypeSet<int> invalidknots;
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	int counts = 0;
	for ( int idy=0; idy<nrindices; idy++ )
	{
	    if ( idx==sampleindices_[idy] )
		counts++;
	}

	if ( counts && counts<3 )
	    invalidknots += idx;
    }
   
    bodytriangle_->coordindices_.erase();
    bool allvalid = !invalidknots.size();
    for ( int idx=0; idx<nrindices/3; idx++ )
    {
	if ( !allvalid && ( invalidknots.validIdx(sampleindices_[3*idx]) ||
		    	    invalidknots.validIdx(sampleindices_[3*idx+1]) ||
			    invalidknots.validIdx(sampleindices_[3*idx+2]) ) )
	    continue;

	bodytriangle_->coordindices_ += sampleindices_[3*idx];
	bodytriangle_->coordindices_ += sampleindices_[3*idx+1];
	bodytriangle_->coordindices_ += sampleindices_[3*idx+2];
	bodytriangle_->coordindices_ += -1;
    }

    bodytriangle_->ischanged_ = true;
    return true;
}


bool ExplPolygonSurface::prepareBodyDAGTree()
{
    if ( !tetrahedratree_ )
	tetrahedratree_ = new DAGTetrahedraTree;

    if ( !surface_ )
	return false;
    
    const StepInterval<int> rrg = surface_->rowRange();
    if ( rrg.isUdf() )
	return false;

    TypeSet<Coord3> pts;
    for ( int plg=rrg.start; plg<=rrg.stop; plg += rrg.step )
	surface_->getCubicBezierCurve( plg, pts, (float)scalefacs_.z );
   
    for ( int idx=0; idx<pts.size(); idx++ )
	coordlist_->set( idx, pts[idx] );

    for ( int idx=0; idx<pts.size(); idx++ )
	pts[idx].z *= scalefacs_.z;
    
    if ( !tetrahedratree_->setCoordList( pts, false ) )
	return false;
    
    ParallelDTetrahedralator triangulator( *tetrahedratree_ );
    return triangulator.execute(true);
}


char ExplPolygonSurface::positionToBody( const Coord3 pt )
{
    if ( !pt.isDefined() )
	return -1;
    
    if ( !tetrahedratree_ && !prepareBodyDAGTree() )
	return -1;
    
    const Coord3 scaledpt = pt.scaleBy(scalefacs_);
    return tetrahedratree_->searchTetrahedra( scaledpt );
}


/*
char ExplPolygonSurfacerrlocationToSurface( const Coord3 pt )
{
    if ( !pt.isDefined() || !sampleindices_.size() )
	return -1;

    const Coord3 scaledpt = pt.scaleBy(scalefacs_);

    const Line3 randomline( scaledpt, Coord3(1,0,0) );
    Coord3 intersectpt0 = Coord3::udf(), intersectpt1 = Coord3::udf();
    for ( int idx=0; idx<sampleindices_.size()/3; idx++ )
    {
	const Coord3 v0 = samples_[sampleindices_[3*idx]].scaleBy(scalefacs_);
	const Coord3 v1 = samples_[sampleindices_[3*idx+1]].scaleBy(scalefacs_);
	const Coord3 v2 = samples_[sampleindices_[3*idx+2]].scaleBy(scalefacs_);

	if ( mIsZero((v0-scaledpt).sqAbs(),1e-6) || 
	     mIsZero((v1-scaledpt).sqAbs(),1e-6) ||
	     mIsZero((v2-scaledpt).sqAbs(),1e-6) )
	    return 0;

	Plane3 plane( (v1-v0).cross(v2-v1), v0, false );
	Coord3 ptonplane; //the projection of pt on triangle plane.
	if ( !plane.intersectWith(randomline,ptonplane) )
	    continue;

	if ( !ptonplane.isDefined() )
	{
	    pErrMsg( "Something is wrong!" );
	    continue;
	}

	if ( pointInTriangle3D(ptonplane,v0,v1,v2,1e-3) )
	{
	    if ( !intersectpt0.isDefined() )
		intersectpt0 = ptonplane;
	    else if ( !intersectpt1.isDefined() )
		intersectpt1 = ptonplane;

	    if ( intersectpt0.isDefined() && intersectpt1.isDefined() )
		break;
	}
    }

    if ( mIsZero((intersectpt0-scaledpt).sqAbs(),1e-6) || 
	 mIsZero((intersectpt1-scaledpt).sqAbs(),1e-6) )
	return 0;

    if ( (scaledpt-intersectpt0).dot(intersectpt1-scaledpt)>0 )
	return 1;

    return -1;
}
*/

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
	geometries_.remove( idx, false );

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
