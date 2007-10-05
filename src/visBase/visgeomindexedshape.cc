/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          August 2006
 RCS:           $Id: visgeomindexedshape.cc,v 1.2 2007-10-05 16:55:32 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "visgeomindexedshape.h"

#include "indexedshape.h"

#include "viscoord.h"
#include "visnormals.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <SoIndexedTriangleFanSet.h>
#include <Inventor/nodes/SoNormalBinding.h>

mCreateFactoryEntry( visBase::GeomIndexedShape );

namespace visBase
{

GeomIndexedShape::GeomIndexedShape()
    : VisualObjectImpl( true )
    , coords_( Coordinates::create() )
    , normals_( Normals::create() )
    , shape_( 0 )
{
    coords_->ref();
    addChild( coords_->getInventorNode() );

    normals_->ref();
    addChild( normals_->getInventorNode() );
}


GeomIndexedShape::~GeomIndexedShape()
{
    coords_->unRef();
    normals_->unRef();
}


void GeomIndexedShape::setRightHandSystem( bool yn )
{
    if ( yn!=righthandsystem_ )
	normals_->inverse();

    VisualObjectImpl::setRightHandSystem( yn );
    shape_->setRightHandedNormals( yn );
}


void GeomIndexedShape::setSurface( Geometry::IndexedShape* ns )
{
    shape_ = ns;
    shape_->setCoordList( new CoordListAdapter(*coords_),
	    		  new NormalListAdapter(*normals_) );
    shape_->setRightHandedNormals( righthandsystem_ );
    touch();
}


#define mHandleType( type, SoObj, list ) \
if ( geom->type_==Geometry::IndexedGeometry::type ) \
{ \
    const int idy = list##geoms_.indexOf( geom ); \
    if ( idy==-1 ) \
    { \
	shape = new SoObj; \
	addChild( shape ); \
    } \
    else \
    { \
	shape = list##s_[idy]; \
	list##s_.remove( idy ); \
	list##geoms_.remove( idy ); \
    } \
 \
    new##list##s += shape; \
    new##list##geoms += geom; \
}


#define mRemoveOld( list ) \
    while ( list##s_.size() ) \
    { \
	SoIndexedShape* shape = list##s_.remove(0); \
 \
	const int idx = childIndex( shape ); \
	mDynamicCastGet(SoNormalBinding*, nb, idx>0 ? getChild(idx-1) : 0); \
	if ( nb ) removeChild( nb ); \
	removeChild( shape ); \
    } \
 \
    list##geoms_.erase(); \
 \
    list##s_ = new##list##s; \
    list##geoms_ = new##list##geoms

void GeomIndexedShape::touch()
{
    if ( shape_ && shape_->needsUpdate() )
	shape_->update( false );

    ObjectSet<SoIndexedShape> newstrips;
    ObjectSet<const Geometry::IndexedGeometry> newstripgeoms;

    ObjectSet<SoIndexedShape> newlines;
    ObjectSet<const Geometry::IndexedGeometry> newlinegeoms;

    ObjectSet<SoIndexedShape> newfans;
    ObjectSet<const Geometry::IndexedGeometry> newfangeoms;

    if ( shape_ )
    {
	const ObjectSet<Geometry::IndexedGeometry>& geoms=shape_->getGeometry();

	for ( int idx=0; idx<geoms.size(); idx++ )
	{
	    const Geometry::IndexedGeometry* geom = geoms[idx];
	    SoIndexedShape* shape = 0;
	    mHandleType( TriangleStrip, SoIndexedTriangleStripSet, strip )
            else mHandleType( TriangleFan, SoIndexedTriangleFanSet, fan )
 	    else mHandleType( Lines, SoIndexedLineSet, line )

	    if ( geom->ischanged_ )
	    {
		shape->coordIndex.setValuesPointer(
		    geom->coordindices_.size(), geom->coordindices_.arr() );

		shape->normalIndex.setValuesPointer(
		    geom->normalindices_.size(), geom->normalindices_.arr() );
	    }

	    const int idy = childIndex( shape );
	    mDynamicCastGet(SoNormalBinding*, nb, idy>0 ? getChild(idy-1) : 0);

	    if ( geom->normalindices_.size() )
	    {
		if ( !nb )
		{
		    nb = new SoNormalBinding;
		    insertChild( idy, nb );
		}

		nb->value = geom->normalbinding_==
				 Geometry::IndexedGeometry::PerVertex 
		     ? SoNormalBindingElement::PER_VERTEX_INDEXED
		     : SoNormalBindingElement::PER_FACE_INDEXED;
	    }
	    else if ( nb )
		removeChild( nb );

	    geom->ischanged_ = false;
	}
    }

    mRemoveOld( strip );
    mRemoveOld( fan );
    mRemoveOld( line );
}

}; // namespace visBase
