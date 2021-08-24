/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : July 2008
-*/


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
    setPolygonSurface( surf );
}


ExplPolygonSurface::~ExplPolygonSurface()
{
    delete tetrahedratree_;
    setPolygonSurface( 0 );
}


void ExplPolygonSurface::setPolygonSurface( const PolygonSurface* psurf )
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
	}
    }

    for ( int idx=0; idx<samples_.size(); idx++ )
	coordlist_->set( idx, samples_[idx] );
    
    updateGeometries();

    if ( displaypolygons_ )
    {
	polygondisplay_->setCoordIndices( plgcrdindices );
	polygondisplay_->ischanged_ = true;
    }
  
    if ( displaybody_ && !updateBodyDisplay() )
	return false;

    needsupdate_ = true;
    return true;
}

void ExplPolygonSurface::addToTrianglePrimitiveSet(Geometry::PrimitiveSet* ps,
    int idx1, int idx2, int idx3 )
{
    if ( !ps ) return;
    Geometry::IndexedPrimitiveSet* idxps = (Geometry::IndexedPrimitiveSet*) ps;

    const int pssize = idxps->size();
    const int nrtriangles = pssize/3;

    bool reverse = bool( nrtriangles%2 ) ? true : false;

    //reverse = false;
    const int startidx = reverse ? idx3 : idx1;
    const int endidx = reverse ? idx1 : idx3;
    idxps->append( startidx );
    idxps->append( idx2 );
    idxps->append( endidx );
}

bool ExplPolygonSurface::updateBodyDisplay()
{
    const int sampsz = samples_.size();
    if ( sampsz<3 || surface_->nrPolygons()<2 )
	return true;

    if ( sampsz==3 )
    {
	TypeSet<int> smpidxs;
	for ( int idx = 0; idx<3; idx ++)
	    smpidxs += idx;
	bodytriangle_->appendCoordIndices( smpidxs );
 	bodytriangle_->ischanged_ = true;
 	return true;	
    }

    if ( tetrahedratree_ )
	delete tetrahedratree_;

    tetrahedratree_ = new DAGTetrahedraTree;
  
    TypeSet<Coord3> pts;
    for ( int idx=0; idx<sampsz; idx++ )
	pts += samples_[idx].scaleBy(scalefacs_);

    if ( !tetrahedratree_->setCoordList( pts, false ) )
	return false;

    ParallelDTetrahedralator triangulator( *tetrahedratree_ );
    if ( !triangulator.execute() )
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

	normallist_->set( idx, Coord3( 0 , 0 , 0 ) );
    }
   
    bool allvalid = !invalidknots.size();
    for ( int idx=0; idx<nrindices/3; idx++ )
    {
	if ( !allvalid && ( invalidknots.validIdx(sampleindices_[3*idx]) ||
		    	    invalidknots.validIdx(sampleindices_[3*idx+1]) ||
			    invalidknots.validIdx(sampleindices_[3*idx+2]) ) )
	    continue;

	TypeSet<int> triangleidxs;
	for ( int triangleidx = 3*idx; triangleidx < (3*idx+3); triangleidx++)
	{
	    triangleidxs += sampleindices_[triangleidx];
	}
	bodytriangle_->appendCoordIndices( triangleidxs );
	//calcNormals( idx, sampleindices_[3*idx],sampleindices_[3*idx+1],
	//    sampleindices_[3*idx+2] );
    }

    bodytriangle_->ischanged_ = true;
    return true;
}


// below function is not used for the time being. It will be used in the
// future once we figure out the vertex order from Tetrahedralator
void ExplPolygonSurface::calcNormals( int nrtriangles,
    int idx1, int idx2, int idx3 )
{
    if ( idx1>= sampleindices_.size() ||
	idx2 >= sampleindices_.size() ||
	idx3 >= sampleindices_.size() )
    return;

    bool reverse = bool( nrtriangles%2 ) ? true : false;

    reverse = false;

    const int startidx = reverse ? idx3 : idx1;
    const int endidx = reverse ? idx1 : idx3;

    const Coord3 cstart = coordlist_->get( startidx );
    const Coord3 cmiddel = coordlist_->get( idx2 );
    const Coord3 cend = coordlist_->get( endidx );

    if ( !cstart.isDefined() || !cmiddel.isDefined() || !cend.isDefined() )
	return;

    const Coord3 v0 = cstart- cmiddel;
    const Coord3 v1 = cend- cmiddel;

    Coord3 normal = v0.cross( v1 ).normalize();

    const double normalsqlen = normal.sqAbs();
    if ( !normalsqlen )
	normal = Coord3( 1, 0, 0 );
    else
	normal /= Math::Sqrt( normalsqlen );

    normallist_->set( idx1, normal );
    normallist_->set( idx2, normal );
    normallist_->set( idx3, normal );
}


bool ExplPolygonSurface::prepareBodyDAGTree()
{
    if ( tetrahedratree_ )
	delete tetrahedratree_;

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
    return triangulator.executeParallel(true);
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

    mGetIndexedShapeWriteLocker4Geometries();
    if ( geometries_.isPresent( ig ) )
    {
	pErrMsg("Adding more than once");
    }

    ig->ischanged_ = true;
    geometries_ += ig;
}


void ExplPolygonSurface::removeFromGeometries( const IndexedGeometry* ig )
{
    if ( !ig ) return;

    mGetIndexedShapeWriteLocker4Geometries();
    const int idx = geometries_.indexOf( ig );

    if ( idx!=-1 )
	geometries_.removeSingle( idx, false );
}


void ExplPolygonSurface::updateGeometries()
{
    if ( !displaybody_ && bodytriangle_ )
    {
	removeFromGeometries( bodytriangle_ );
    }
    else if ( !bodytriangle_ && displaybody_ )
    {
	bodytriangle_ = new IndexedGeometry( IndexedGeometry::Triangles,
					     coordlist_, normallist_, 0 );

	addToGeometries( bodytriangle_ );
    }
    
    if ( !displaypolygons_ && polygondisplay_ )
    {
	removeFromGeometries( polygondisplay_ );
    }
    else if ( !polygondisplay_ && displaypolygons_ )
    {
	polygondisplay_ = new IndexedGeometry( IndexedGeometry::Lines, 
					       coordlist_, normallist_, 0 );
	addToGeometries( polygondisplay_ );
    }
}

}; // namespace Geometry
