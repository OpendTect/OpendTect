/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: visshape.cc,v 1.1 2003-01-07 10:29:54 kristofer Exp $";

#include "visshape.h"

#include "viscoord.h"
#include "vismaterial.h"
#include "visnormals.h"
#include "vistexture2.h"
#include "vistexturecoords.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoIndexedShape.h"
#include "Inventor/nodes/SoMaterialBinding.h"
#include "Inventor/nodes/SoNormalBinding.h"

visBase::Shape::Shape( SoShape* shape_ )
    : shape( shape_ )
    , texture2( 0 )
    , texture3( 0 )
    , material( 0 )
    , root( new SoSeparator )
    , materialbinding( 0 )
{
    root->ref();
    addNode( shape );
}


visBase::Shape::~Shape()
{
    if ( texture2 ) texture2->unRef();
    // if ( texture3 ) texture3->unRef();
    if ( material ) material->unRef();

    root->unref();
}

#define setGetItem(ownclass, clssname, variable) \
void visBase::ownclass::set##clssname( visBase::clssname* newitem ) \
{ \
    if ( variable ) \
    { \
	removeNode( variable->getData() ); \
	variable->unRef(); \
	variable = 0; \
    } \
 \
    if ( newitem ) \
    { \
	variable = newitem; \
	variable->ref(); \
	addNode( variable->getData() ); \
    } \
} \
 \
 \
visBase::clssname* visBase::ownclass::get##clssname() \
{ \
    return variable; \
}


setGetItem( Shape, Texture2, texture2 );
//setGetItem( Shape, Texture3, texture3 );
setGetItem( Shape, Material, material );


void visBase::Shape::setMaterialBinding( int nv )
{
    bool isindexed = dynamic_cast<IndexedShape*>( this );

    if ( !materialbinding )
    {
	materialbinding = new SoMaterialBinding;
	addNode( materialbinding );
    }
    if ( !nv )
    {
	materialbinding->value = SoMaterialBinding::OVERALL;
    }
    else if ( nv==1 )
    {
	materialbinding->value = isindexed ?
	    SoMaterialBinding::PER_FACE_INDEXED : SoMaterialBinding::PER_FACE;
    }
    else
    {
	materialbinding->value = isindexed
	    ? SoMaterialBinding::PER_VERTEX_INDEXED
	    : SoMaterialBinding::PER_VERTEX;
    }
}


int visBase::Shape::getMaterialBinding() const
{
    if ( !materialbinding ) return 0;
    return materialbinding->value.getValue()==SoMaterialBinding::PER_FACE ||
	   materialbinding->value.getValue()==
	   			SoMaterialBinding::PER_FACE_INDEXED ? 1 : 2;
}


SoNode* visBase::Shape::getData()
{ return root; }


void visBase::Shape::addNode( SoNode*  node )
{
    root->insertChild( node, 0 );
}


void visBase::Shape::removeNode( SoNode* node )
{
    while ( root->findChild( node ) != -1 )
	root->removeChild( node );
}


visBase::VertexShape::VertexShape( SoVertexShape* shape_ )
    : Shape( shape_ )
    , normals( 0 )
    , coords( 0 )
    , texturecoords( 0 )
    , normalbinding( 0 )
{
    setCoordinates( visBase::Coordinates::create() );
}


visBase::VertexShape::~VertexShape()
{
    setCoordinates( 0 );
    setTextureCoords( 0 );
    setNormals( 0 );
}


setGetItem( VertexShape, Coordinates, coords );
setGetItem( VertexShape, Normals, normals );
setGetItem( VertexShape, TextureCoords, texturecoords );


void visBase::VertexShape::setNormalPerFaceBinding( bool nv )
{
    bool isindexed = dynamic_cast<IndexedShape*>( this );

    if ( !normalbinding )
    {
	normalbinding = new SoNormalBinding;
	addNode( normalbinding );
    }
    if ( nv )
    {
	normalbinding->value = isindexed ?
	    SoNormalBinding::PER_FACE_INDEXED : SoNormalBinding::PER_FACE;
    }
    else
    {
	normalbinding->value = isindexed
	    ? SoNormalBinding::PER_VERTEX_INDEXED
	    : SoNormalBinding::PER_VERTEX;
    }
}


bool visBase::VertexShape::getNormalPerFaceBinding() const
{
    if ( !normalbinding ) return true;
    return normalbinding->value.getValue()==SoNormalBinding::PER_FACE ||
	   normalbinding->value.getValue()==SoNormalBinding::PER_FACE_INDEXED;
}


visBase::IndexedShape::IndexedShape( SoIndexedShape* shape_ )
    : VertexShape( shape_ )
    , indexedshape( shape_ )
{}


#define setGetIndex( resourcename, fieldname )  \
int visBase::IndexedShape::nr##resourcename() const \
{ return indexedshape->fieldname.getNum(); } \
 \
 \
void visBase::IndexedShape::set##resourcename( int pos, int idx ) \
{ indexedshape->fieldname.set1Value( pos, idx ); } \
 \
 \
int visBase::IndexedShape::get##resourcename( int pos ) const \
{ return indexedshape->fieldname[pos]; } \


setGetIndex( CoordIndex, coordIndex );
setGetIndex( TextureCoordIndex, textureCoordIndex );
setGetIndex( NormalIndex, normalIndex );
setGetIndex( MaterialIndex, materialIndex );
