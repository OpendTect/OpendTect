/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          January 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visrandompos2body.h"

#include "delaunay.h"
#include "delaunay3d.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vistransform.h"
#include "vistristripset.h"
#include "visshape.h"
#include "visnormals.h"
#include "vispolygonoffset.h"


mCreateFactoryEntry( visBase::RandomPos2Body );

namespace visBase
{

class NormalCalculator: public ParallelTask
{
public:
    NormalCalculator(Normals* normals, const od_int64 size,
	const TypeSet<int>&,const TypeSet<Coord3>&);
    od_int64	totalNr() const { return totalnrnormals_; }

protected:
    bool	doWork(od_int64 start, od_int64 stop, int);
    od_int64	nrIterations() const { return totalnrnormals_; }

private:
    Normals* normals_;
    Threads::Atomic<od_int64>	totalnrnormals_;
    const TypeSet<int>& psidx_;
    const TypeSet<Coord3>&	picks_;


};


NormalCalculator::NormalCalculator( Normals* normals, const od_int64 size,
		   const TypeSet<int>& psidx,const TypeSet<Coord3>&picks)
    : normals_( normals )
    , totalnrnormals_( size )
    , psidx_( psidx )
    , picks_( picks )
{
}

bool NormalCalculator::doWork(od_int64 start,od_int64 stop,int)
{
/* OSG-TODO: This implementation is not OK, and must be renewed in case
   we decide to set useOsgAutoNormalComputation(false). Reversing is only
   required when dealing with triangle strips, normals must be averaged
   and not overwritten, and what about the display transformation? */

/*
    for ( int idx = mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	const int nrtriangle = idx/3;
	const bool reverse = !((bool)((nrtriangle+1) % 2));
	const int startidx = reverse ? idx + 2 : idx;
	const int endidx = reverse ? idx : idx +2;

	const Coord3 v0 = picks_[psidx_[startidx]]-picks_[psidx_[idx+1]];
	const Coord3 v1 = picks_[psidx_[endidx]]-picks_[psidx_[idx+1]];

	Coord3 normal = v0.cross( v1 );
	const double normalsqlen = normal.sqAbs();
	if ( !normalsqlen )
	    normal = Coord3( 1, 0, 0 );
	else
	    normal /= Math::Sqrt( normalsqlen );

	normals_->setNormal( psidx_[idx], normal );
	normals_->setNormal( psidx_[idx+1], normal );
	normals_->setNormal( psidx_[idx+2], normal );
    }
*/
    return true;
}


RandomPos2Body::RandomPos2Body()
    : VisualObjectImpl( false )
    , transformation_( 0 )
    , vtxshape_( VertexShape::create() )
{
    vtxshape_->ref();
    addChild( vtxshape_->osgNode() );
    vtxshape_->setPrimitiveType( Geometry::PrimitiveSet::Triangles );
    vtxshape_->setNormalBindType( VertexShape::BIND_PER_VERTEX );
    vtxshape_->useOsgAutoNormalComputation( true );

    renderOneSide( 0 );
}


RandomPos2Body::~RandomPos2Body()
{
    if ( transformation_ )
	transformation_->unRef();
    if ( vtxshape_ )
	vtxshape_->unRef();
 }


void RandomPos2Body::renderOneSide( int side )
{
    vtxshape_->renderOneSide( side );
}


bool RandomPos2Body::setPoints( const TypeSet<Coord3>& pts )
{
    picks_.erase();
    picks_.copy( pts );

    if ( !pts.size() )
	return true;

    TypeSet<Coord3> picks;
    const float zscale = SI().zDomain().userFactor();
    bool oninline = true, oncrossline = true, onzslice = true;
    int inl=-1, crl=-1;
    float z=-1;
    const float zeps = SI().zStep()/2.0;

    for ( int idx=0; idx<pts.size(); idx++ )
    {
	vtxshape_->getCoordinates()->setPos( idx, pts[idx] );
	picks += Coord3(pts[idx].coord(), pts[idx].z*zscale);

	BinID bid = SI().transform( pts[idx] );
	if ( idx==0 )
	{
	    inl = bid.inl();
	    crl = bid.crl();
	    z = pts[idx].z;
	}
	else
	{
	    if ( oninline && inl!=bid.inl() )
		oninline = false;

	    if ( oncrossline && crl!=bid.crl() )
		oncrossline = false;

	    if ( onzslice && !mIsEqual(z,pts[idx].z,zeps) )
		onzslice = false;
	}
    }

    vtxshape_->getCoordinates()->removeAfter( pts.size()-1 );

    TypeSet<int> result;
    if ( !oninline && !oncrossline && !onzslice )
    {
	DAGTetrahedraTree tree;
	tree.setCoordList( picks, false );
	
	ParallelDTetrahedralator pdtri( tree );
	if ( !pdtri.execute() )
	    return false;

	if ( !tree.getSurfaceTriangles(result) )
	    return false;
    }
    else
    {
	TypeSet<Coord> knots;
	if ( onzslice )
	{
	    for ( int idx=0; idx<pts.size(); idx++ )
		knots += pts[idx].coord();
	}
	else if ( oninline )
	{
	    for ( int idx=0; idx<pts.size(); idx++ )
		knots += Coord(picks[idx].y,picks[idx].z);
	}
	else
	{
	    for ( int idx=0; idx<pts.size(); idx++ )
		knots += Coord(picks[idx].x,picks[idx].z);
	}

	PolygonTriangulate( knots, result );
    }

    Geometry::IndexedPrimitiveSet* primitiveset =
	Geometry::IndexedPrimitiveSet::create( false );
    primitiveset->ref();

    primitiveset->append( result.arr(), result.size() );
    vtxshape_->addPrimitiveSet( primitiveset );
    primitiveset->unRef();

/* OSG-TODO: Remove if we keep using useOsgAutoNormalComputation(true)
    Normals* normals = Normals::create();
    normals->ref();

    NormalCalculator nlcalcator( normals, result.size()-2, result, picks_ );
    TaskRunner tr;

    if( TaskRunner::execute( &tr,nlcalcator ) )
	vtxshape_->setNormals( normals );
    
    normals->unRef();
*/
    vtxshape_->dirtyCoordinates();
    
    return true;

}


void RandomPos2Body::setDisplayTransformation( const mVisTrans*  nt )
{
    if ( transformation_ ) transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ ) 
    {
	transformation_->ref();
	if ( vtxshape_ )
	    vtxshape_->setDisplayTransformation( transformation_ );

    }
}


const mVisTrans* RandomPos2Body::getDisplayTransformation() const
{ return transformation_; }


}; // namespace visBase
