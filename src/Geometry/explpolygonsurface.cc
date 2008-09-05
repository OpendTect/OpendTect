/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Yuancheng Liu
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: explpolygonsurface.cc,v 1.1 2008-09-05 16:48:42 cvsyuancheng Exp $";

#include "explpolygonsurface.h"

#include "delaunay3d.h"
#include "polygonsurface.h"
#include "positionlist.h"
#include "trigonometry.h"


namespace Geometry {


ExplPolygonSurface::ExplPolygonSurface( PolygonSurface* surf )
    : surface_( 0 )
    , bodytriangle_( 0 ) 
    , polygondisplay_( 0 )			 
    , displaypolygons_( true )
    , displaybody_( true )
    , needsupdate_( true )			  
{
    setSurface( surf );
}


ExplPolygonSurface::~ExplPolygonSurface()
{
    setSurface( 0 );
}


void ExplPolygonSurface::setSurface( PolygonSurface* psurf )
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

    TypeSet<Coord3> pts; 
    const StepInterval<int> rrg = surface_->rowRange();
    
    for ( int plg=rrg.start; plg<=rrg.stop; plg += rrg.step )
	surface_->getPolygonCrds( plg, pts );

    for ( int idx=0; idx<pts.size(); idx++ )
	coordlist_->set( idx, pts[idx] );
    
    updateGeometries();

    if ( displaypolygons_ )
	updatePolygonDisplay();
  
    if ( displaybody_ )
    {
	if ( pts.size()<4 || surface_->nrPolygons()<2 )
    	    return true;

	if ( !updateBodyDisplay( pts ) )
	    return false;
    }

    setRightHandedNormals( true );
    needsupdate_ = true;
    return true;
}


void ExplPolygonSurface::display( bool ynpolygons, bool ynbody )
{
    if ( displaypolygons_==ynpolygons && displaybody_==ynbody )
	return;

    displaypolygons_ = ynpolygons;
    displaybody_ = ynbody;
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


void ExplPolygonSurface::updatePolygonDisplay()
{
    const StepInterval<int> rrg = surface_->rowRange();
    polygondisplay_->coordindices_.erase();

    int previousknots = 0;
    for ( int plg=rrg.start; plg<=rrg.stop; plg += rrg.step )
    {
	const int nrknots = surface_->colRange( plg ).nrSteps()+1;
	for ( int knot=0; knot<nrknots; knot++ )
	    polygondisplay_->coordindices_ +=  knot+previousknots;

	polygondisplay_->coordindices_ += previousknots;
	polygondisplay_->coordindices_ += -1;
	previousknots += nrknots;
    }

    polygondisplay_->ischanged_ = true;
}


bool ExplPolygonSurface::updateBodyDisplay( const TypeSet<Coord3>&  pts )
{
    const StepInterval<int> rrg = surface_->rowRange();
   
    DAGTetrahedraTree dagtree;
    if ( !dagtree.setCoordList( pts, false ) )
	return false;

    ParallelDTetrahedralator triangulator( dagtree );
    triangulator.dataIsRandom( true );
    if ( !triangulator.execute(true) )
	return false;

    TypeSet<int> triangles;
    dagtree.getTetrahedraTriangles( triangles );
    
    TypeSet<int> plgknots[rrg.nrSteps()+1];
    TypeSet<int> concaveedges;
    int usednrknots = 0;
    for ( int plg=0; plg<rrg.nrSteps()+1; plg++ )
    {
	if ( surface_->colRange(plg).isUdf() )
	    continue;
	
	const int nrknots = surface_->colRange(plg).nrSteps()+1;
	for ( int idx=0; idx<nrknots; idx++ )
	    plgknots[plg] += idx+usednrknots;

	TypeSet<int> ccts;
	surface_->getPolygonConcaveTriangles( plg, ccts );
	
	for ( int vidx=0; vidx<ccts.size()/3; vidx++ )
	{
	    if ( ccts[3*vidx]==0 && ccts[3*vidx+2]==nrknots-1 )
	    {
		if ( abs(ccts[3*vidx+1]-ccts[3*vidx]) != 1 )
		{
		    concaveedges += ccts[3*vidx]+usednrknots;
    		    concaveedges += ccts[3*vidx+1]+usednrknots;
		}
	    }
	    else if ( abs(ccts[3*vidx+2]-ccts[3*vidx])!=1 )
	    {
    		concaveedges += ccts[3*vidx]+usednrknots;
    		concaveedges += ccts[3*vidx+2]+usednrknots;
	    }
	}

	usednrknots += nrknots;
    }

    bodytriangle_->coordindices_.erase();
    for ( int idx=0; idx<triangles.size()/3; idx++ )
    {
	 const int v0 = triangles[3*idx]; 
	 const int v1 = triangles[3*idx+1]; 
	 const int v2 = triangles[3*idx+2]; 

	bool isconcaveedge = false;
	for (int ti=0; ti<concaveedges.size()/2; ti++ )
	{
	    if ( (concaveedges[2*ti]==v0 || concaveedges[2*ti]==v1 ||
		  concaveedges[2*ti]==v2) && (concaveedges[2*ti+1]==v0 || 
		  concaveedges[2*ti+1]==v1 || concaveedges[2*ti+1]==v2) )
	    {
		isconcaveedge = true;
		break;
	    }
	}

	if ( !isconcaveedge )
	{
	    bodytriangle_->coordindices_ += v0;
	    bodytriangle_->coordindices_ += v1;
	    bodytriangle_->coordindices_ += v2;
	    bodytriangle_->coordindices_ += -1;
	}
    }

    bodytriangle_->ischanged_ = true;
    return true;
}

}; // namespace Geometry
