/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          August 2006
 RCS:           $Id: visgeomindexedshape.cc,v 1.14 2008-10-27 19:49:30 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "visgeomindexedshape.h"

#include "indexedshape.h"

#include "viscoord.h"
#include "visnormals.h"
#include "vistexturecoords.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <SoIndexedTriangleFanSet.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/SoDB.h>

#include "SoIndexedLineSet3D.h"

mCreateFactoryEntry( visBase::GeomIndexedShape );

namespace visBase
{

GeomIndexedShape::GeomIndexedShape()
    : VisualObjectImpl( true )
    , coords_( Coordinates::create() )
    , normals_( Normals::create() )
    , texturecoords_( TextureCoords::create() )				   
    , shape_( 0 )
    , lineradius_( -1 )
    , lineconstantonscreen_( false )
    , linemaxsize_( -1 )
{
    coords_->ref();
    addChild( coords_->getInventorNode() );

    normals_->ref();
    addChild( normals_->getInventorNode() );

    texturecoords_->ref();
    addChild( texturecoords_->getInventorNode() );
}


GeomIndexedShape::~GeomIndexedShape()
{
    coords_->unRef();
    normals_->unRef();
    texturecoords_->unRef();
}


void GeomIndexedShape::setDisplayTransformation( mVisTrans* nt )
{
    coords_->setDisplayTransformation( nt );
    normals_->setDisplayTransformation( nt );
}


mVisTrans* GeomIndexedShape::getDisplayTransformation()
{ return coords_->getDisplayTransformation(); }


void GeomIndexedShape::setRightHandSystem( bool yn )
{
    if ( yn!=righthandsystem_ )
	normals_->inverse();

    VisualObjectImpl::setRightHandSystem( yn );
    if ( shape_ ) shape_->setRightHandedNormals( yn );

    for ( int idx=lines_.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( SoIndexedLineSet3D*, line3d, lines_[idx] );
	if ( !line3d )
	    continue;

	line3d->rightHandSystem = righthandsystem_;
    }
}


void GeomIndexedShape::setSurface( Geometry::IndexedShape* ns, TaskRunner* tr )
{
    shape_ = ns;
    shape_->setCoordList( new CoordListAdapter(*coords_),
	    		  new NormalListAdapter(*normals_), 
			  new TextureCoordListAdapter(*texturecoords_) );
    shape_->setRightHandedNormals( righthandsystem_ );
    touch( false, tr );
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

void GeomIndexedShape::touch( bool forall, TaskRunner* tr )
{
    //SoDB::writelock();
    if ( shape_ && shape_->needsUpdate() )
	shape_->update( forall, tr );

    ObjectSet<SoIndexedShape> newstrips;
    ObjectSet<const Geometry::IndexedGeometry> newstripgeoms;

    ObjectSet<SoIndexedShape> newlines;
    ObjectSet<const Geometry::IndexedGeometry> newlinegeoms;

    ObjectSet<SoIndexedShape> newfans;
    ObjectSet<const Geometry::IndexedGeometry> newfangeoms;

    if ( !shape_ )
    {
	mRemoveOld( strip );
	mRemoveOld( fan );
	mRemoveOld( line );
	return;
    }

    const ObjectSet<Geometry::IndexedGeometry>& geoms=shape_->getGeometry();

    for ( int idx=0; idx<geoms.size(); idx++ )
    {
	const Geometry::IndexedGeometry* geom = geoms[idx];
	SoIndexedShape* shape = 0;
	mHandleType( TriangleStrip, SoIndexedTriangleStripSet, strip )
	else mHandleType( TriangleFan, SoIndexedTriangleFanSet, fan )
	else mHandleType( Lines, SoIndexedLineSet, line )
	else if ( lineradius_>0 )
	{
	    mHandleType( Lines, SoIndexedLineSet3D, line );
	    mDynamicCastGet( SoIndexedLineSet3D*, line3d, shape );
	    if ( line3d )
	    {
		line3d->radius = lineradius_;
		line3d->screenSize = lineconstantonscreen_;
		line3d->maxRadius = linemaxsize_;
		line3d->rightHandSystem = righthandsystem_;
	    }
	}
	else
	    mHandleType( Lines, SoIndexedLineSet, line )

	if ( !shape )
	    continue;

	if ( geom->ischanged_ )
	{
	    /* TODO: leads to crash. Probably because geom has been deleted.
	    shape->coordIndex.setValuesPointer(
		geom->coordindices_.size(), geom->coordindices_.arr() );

	    shape->normalIndex.setValuesPointer(
		geom->normalindices_.size(), geom->normalindices_.arr() );
	    */

	    SbBool oldstatus = shape->coordIndex.enableNotify( false );
	    shape->coordIndex.setValues( 0,
		geom->coordindices_.size(), geom->coordindices_.arr() );
	    shape->coordIndex.setNum( geom->coordindices_.size() );
	    shape->coordIndex.enableNotify( oldstatus );
	    shape->coordIndex.touch();

	    if ( shape_->createsNormals() )
	    {
    		oldstatus = shape->normalIndex.enableNotify( false );
    		shape->normalIndex.setValues( 0,
			geom->normalindices_.size(), 
			geom->normalindices_.arr() );
    		shape->normalIndex.setNum( geom->normalindices_.size() );
    		shape->normalIndex.enableNotify( oldstatus );
		shape->normalIndex.touch();
	    }
	    else
		shape->normalIndex.setNum( 0 );

	    if ( shape_->createsTextureCoords() )
	    {
		oldstatus = shape->textureCoordIndex.enableNotify( false );
		shape->textureCoordIndex.setValues( 0,
			geom->texturecoordindices_.size(), 
			geom->texturecoordindices_.arr() );
		shape->textureCoordIndex.setNum(
			geom->texturecoordindices_.size() );    
		shape->textureCoordIndex.enableNotify( oldstatus );
		shape->textureCoordIndex.touch();
	    }
	    else
		shape->textureCoordIndex.setNum( 0 );

	    shape->touch();
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

    mRemoveOld( strip );
    mRemoveOld( fan );
    mRemoveOld( line );
    //SoDB::writeunlock();
}

}; // namespace visBase
