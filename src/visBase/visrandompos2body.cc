/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          January 2009
________________________________________________________________________

-*/

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

    setRenderMode( RenderBothSides );
}


RandomPos2Body::~RandomPos2Body()
{
    if ( transformation_ )
	transformation_->unRef();
    if ( vtxshape_ )
	vtxshape_->unRef();
 }


void RandomPos2Body::setRenderMode( RenderMode mode )
{
    vtxshape_->setRenderMode( mode );
}


bool RandomPos2Body::setPoints( const TypeSet<Coord3>& pts )
{
    return setPoints( pts, false );
}


bool RandomPos2Body::setPoints( const TypeSet<Coord3>& pts, bool ispoly )
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
    const float zeps = SI().zStep()/2.f;

    TrcKeyZSampling bbox; // TODO: Use a plane fitter
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	vtxshape_->getCoordinates()->setPos( idx, pts[idx] );
	picks += Coord3(pts[idx].coord(), pts[idx].z*zscale);

	const BinID bid = SI().transform( pts[idx] );
	if ( idx==0 )
	{
	    inl = bid.inl();
	    crl = bid.crl();
	    z = pts[idx].z;
	    bbox.hsamp_.init( TrcKey(bid) );
	    bbox.zsamp_.set( pts[idx].z, pts[idx].z, SI().zStep() );
	}
	else
	{
	    if ( oninline && inl!=bid.inl() )
		oninline = false;

	    if ( oncrossline && crl!=bid.crl() )
		oncrossline = false;

	    if ( onzslice && !mIsEqual(z,pts[idx].z,zeps) )
		onzslice = false;
	    bbox.include( bid, pts[idx].z );
	}
    }

    vtxshape_->getCoordinates()->removeAfter( pts.size()-1 );
    if ( ispoly )
    {
	const int nrinl = bbox.nrInl();
	const int nrcrl = bbox.nrCrl();
	const int nrz = bbox.nrZ();

	oninline = nrinl<=nrcrl && nrinl<=nrz;
	oncrossline = nrcrl<=nrinl && nrcrl<=nrz;
	onzslice = nrz<=nrcrl && nrz<=nrinl;
    }

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
