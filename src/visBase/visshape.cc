/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: visshape.cc,v 1.36 2012-09-03 14:33:23 cvskris Exp $";

#include "visshape.h"

#include "errh.h"
#include "iopar.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdetail.h"
#include "visevent.h"
#include "visforegroundlifter.h"
#include "vismaterial.h"
#include "visnormals.h"
#include "vistexture2.h"
#include "vistexture3.h"
#include "vistexturecoords.h"

#include "Inventor/nodes/SoIndexedShape.h"
#include "Inventor/nodes/SoMaterialBinding.h"
#include "Inventor/nodes/SoNormalBinding.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoShapeHints.h"
#include "Inventor/nodes/SoSwitch.h"

#include <osg/PrimitiveSet>


namespace visBase
{

const char* Shape::sKeyOnOff() 			{ return  "Is on";	}
const char* Shape::sKeyTexture() 		{ return  "Texture";	}
const char* Shape::sKeyMaterial() 		{ return  "Material";	}

Shape::Shape( SoNode* shape )
    : shape_( shape )
    , onoff_( new SoSwitch )
    , texture2_( 0 )
    , texture3_( 0 )
    , material_( 0 )
    , root_( new SoSeparator )
    , materialbinding_( 0 )
    , lifter_( ForegroundLifter::create() )			   
    , lifterswitch_( new SoSwitch )
{
    onoff_->ref();
    onoff_->addChild( root_ );
    onoff_->whichChild = 0;
    insertNode( shape_ );

    lifterswitch_->ref();
    lifterswitch_->whichChild = SO_SWITCH_NONE;
    lifter_->ref();
    lifter_->setLift(0.8);
    lifterswitch_->addChild( lifter_->getInventorNode() );
    insertNode( lifterswitch_ );
}


Shape::~Shape()
{
    if ( texture2_ ) texture2_->unRef();
    if ( texture3_ ) texture3_->unRef();
    if ( material_ ) material_->unRef();

    getInventorNode()->unref();
    lifter_->unRef();
}


void Shape::turnOnForegroundLifter( bool yn )
{ lifterswitch_->whichChild = yn ? 0 : SO_SWITCH_NONE; }


void Shape::turnOn(bool n)
{
    if ( onoff_ ) onoff_->whichChild = n ? 0 : SO_SWITCH_NONE;
    else if ( !n )
    {
	pErrMsg( "Turning off object without switch");
    }
}


bool Shape::isOn() const
{
    return !onoff_ || !onoff_->whichChild.getValue();
}


void Shape::removeSwitch()
{
    root_->ref();
    onoff_->unref();
    onoff_ = 0;
}


void Shape::setRenderCache(int mode)
{
    if ( !mode )
	root_->renderCaching = SoSeparator::OFF;
    else if ( mode==1 )
	root_->renderCaching = SoSeparator::ON;
    else
	root_->renderCaching = SoSeparator::AUTO;
}


int Shape::getRenderCache() const
{
    if ( root_->renderCaching.getValue()==SoSeparator::OFF )
	return 0;

    if ( root_->renderCaching.getValue()==SoSeparator::ON )
	return 1;
    
    return 2;
}


#define mDefSetGetItem(ownclass, clssname, variable) \
void ownclass::set##clssname( clssname* newitem ) \
{ \
    if ( variable ) \
    { \
	removeNode( variable->getInventorNode() ); \
	variable->unRef(); \
	variable = 0; \
    } \
 \
    if ( newitem ) \
    { \
	variable = newitem; \
	variable->ref(); \
	insertNode( variable->getInventorNode() ); \
    } \
} \
 \
 \
clssname* ownclass::gt##clssname() const \
{ \
    return const_cast<clssname*>( variable ); \
}


mDefSetGetItem( Shape, Texture2, texture2_ );
mDefSetGetItem( Shape, Texture3, texture3_ );
mDefSetGetItem( Shape, Material, material_ );


void Shape::setMaterialBinding( int nv )
{
    mDynamicCastGet( const IndexedShape*, isindexed, this );

    if ( !materialbinding_ )
    {
	materialbinding_ = new SoMaterialBinding;
	insertNode( materialbinding_ );
    }
    if ( nv==cOverallMaterialBinding() )
    {
	materialbinding_->value = SoMaterialBinding::OVERALL;
    }
    else if ( nv==cPerFaceMaterialBinding() )
    {
	materialbinding_->value = isindexed ?
	    SoMaterialBinding::PER_FACE_INDEXED : SoMaterialBinding::PER_FACE;
    }
    else if ( nv==cPerVertexMaterialBinding() )
    {
	materialbinding_->value = isindexed
	    ? SoMaterialBinding::PER_VERTEX_INDEXED
	    : SoMaterialBinding::PER_VERTEX;
    }
    else if ( nv==cPerPartMaterialBinding() )
    {
	materialbinding_->value = isindexed
	    ? SoMaterialBinding::PER_PART_INDEXED
	    : SoMaterialBinding::PER_PART;
    }
}


int Shape::getMaterialBinding() const
{
    if ( !materialbinding_ ) return cOverallMaterialBinding();

    if (materialbinding_->value.getValue()==SoMaterialBinding::PER_FACE ||
	materialbinding_->value.getValue()==SoMaterialBinding::PER_FACE_INDEXED)
	return cPerFaceMaterialBinding();

    if (materialbinding_->value.getValue()==SoMaterialBinding::PER_PART ||
	materialbinding_->value.getValue()==SoMaterialBinding::PER_PART_INDEXED)
	return cPerPartMaterialBinding();
    
    if (materialbinding_->value.getValue()==SoMaterialBinding::PER_VERTEX ||
	materialbinding_->value.getValue()==
	    SoMaterialBinding::PER_VERTEX_INDEXED)
	return cPerVertexMaterialBinding();

    return cOverallMaterialBinding();
}


void Shape::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    VisualObject::fillPar( iopar, saveids );

    if ( material_ )
	iopar.set( sKeyMaterial(), material_->id() );

    int textureindex = -1;
    if ( texture2_ )
	textureindex = texture2_->id();
    else if ( texture3_ )
	textureindex = texture3_->id();

    if ( textureindex != -1 )
    {
	iopar.set( sKeyTexture(), textureindex );
	if ( saveids.indexOf(textureindex) == -1 )
	    saveids += textureindex;
    }

    iopar.setYN( sKeyOnOff(), isOn() );
}


int Shape::usePar( const IOPar& par )
{
    int res = VisualObject::usePar( par );
    if ( res!=1 ) return res;

    bool ison;
    if ( par.getYN( sKeyOnOff(), ison) )
	turnOn( ison );

    int textureindex;
    if ( par.get(sKeyTexture(),textureindex) && textureindex!=-1 )
    {
	if ( !DM().getObject(textureindex) )
	    return 0;

	Texture2* t2 = dynamic_cast<Texture2*>(DM().getObject(textureindex));
	Texture3* t3 = dynamic_cast<Texture3*>(DM().getObject(textureindex));

	if ( t2 ) setTexture2( t2 );
	else if ( t3 ) setTexture3( t3 );
	else return -1;
    }

    return 1;
}

	
SoNode* Shape::gtInvntrNode()
{ return onoff_ ? (SoNode*) onoff_ : (SoNode*) root_; }


void Shape::insertNode( SoNode*  node )
{
    root_->insertChild( node, 0 );
}


void Shape::replaceShape( SoNode* node )
{
    removeNode( shape_ );
    root_->addChild( node );
}


void Shape::removeNode( SoNode* node )
{
    while ( root_->findChild( node ) != -1 )
	root_->removeChild( node );
}


VertexShape::VertexShape( SoVertexShape* shape )
    : Shape( shape )
    , normals_( 0 )
    , coords_( 0 )
    , texturecoords_( 0 )
    , normalbinding_( 0 )
    , shapehints_( 0 )
{
    setCoordinates( Coordinates::create() );
}


VertexShape::~VertexShape()
{
    if ( normals_ ) normals_->unRef();
    if ( coords_ ) coords_->unRef();
    if ( texturecoords_ ) texturecoords_->unRef();
}


void VertexShape::setDisplayTransformation( const mVisTrans* tr )
{ coords_->setDisplayTransformation( tr ); }


const mVisTrans* VertexShape::getDisplayTransformation() const
{ return  coords_->getDisplayTransformation(); }


mDefSetGetItem( VertexShape, Coordinates, coords_ );
mDefSetGetItem( VertexShape, Normals, normals_ );
mDefSetGetItem( VertexShape, TextureCoords, texturecoords_ );



void VertexShape::setNormalPerFaceBinding( bool nv )
{
    mDynamicCastGet( const IndexedShape*, isindexed, this );

    if ( !normalbinding_ )
    {
	normalbinding_ = new SoNormalBinding;
	insertNode( normalbinding_ );
    }
    if ( nv )
    {
	normalbinding_->value = isindexed ?
	    SoNormalBinding::PER_FACE_INDEXED : SoNormalBinding::PER_FACE;
    }
    else
    {
	normalbinding_->value = isindexed
	    ? SoNormalBinding::PER_VERTEX_INDEXED
	    : SoNormalBinding::PER_VERTEX;
    }
}


bool VertexShape::getNormalPerFaceBinding() const
{
    if ( !normalbinding_ ) return true;
    return normalbinding_->value.getValue()==SoNormalBinding::PER_FACE ||
	   normalbinding_->value.getValue()==SoNormalBinding::PER_FACE_INDEXED;
}


#define mCheckCreateShapeHints() \
    if ( !shapehints_ ) \
    { \
	shapehints_ = new SoShapeHints; \
	insertNode( shapehints_ ); \
    }

void VertexShape::setVertexOrdering( int nv )
{
    mCheckCreateShapeHints()
    if ( nv==cClockWiseVertexOrdering() )
	shapehints_->vertexOrdering = SoShapeHints::CLOCKWISE;
    else if ( nv==cCounterClockWiseVertexOrdering() )
	shapehints_->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    else if ( nv==cUnknownVertexOrdering() )
	shapehints_->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
}


int VertexShape::getVertexOrdering() const
{
    if ( !shapehints_ ) return cUnknownVertexOrdering();

    if ( shapehints_->vertexOrdering.getValue()==SoShapeHints::CLOCKWISE )
	return cClockWiseVertexOrdering();
    if ( shapehints_->vertexOrdering.getValue()==SoShapeHints::COUNTERCLOCKWISE )
	return cCounterClockWiseVertexOrdering();

    return cUnknownVertexOrdering();
}


void VertexShape::setFaceType( int ft )
{
    mCheckCreateShapeHints()
    shapehints_->faceType = ft==cUnknownFaceType()
	? SoShapeHints::UNKNOWN_FACE_TYPE
	: SoShapeHints::CONVEX;
}


int VertexShape::getFaceType() const
{
    return shapehints_ && 
	    shapehints_->faceType.getValue()==SoShapeHints::CONVEX
	? cConvexFaceType() : cUnknownFaceType();
}


void VertexShape::setShapeType( int st )
{
    mCheckCreateShapeHints()
    shapehints_->shapeType = st==cUnknownShapeType()
	? SoShapeHints::UNKNOWN_SHAPE_TYPE
	: SoShapeHints::SOLID;
}


int VertexShape::getShapeType() const
{
    return shapehints_ && 
	   shapehints_->shapeType.getValue()==SoShapeHints::SOLID
	? cSolidShapeType()
	: cUnknownShapeType();
}


IndexedShape::IndexedShape( SoIndexedShape* shape )
    : VertexShape( shape )
    , indexedshape_( shape )
{}


void IndexedShape::replaceShape( SoNode* node )
{
    mDynamicCastGet( SoIndexedShape*, is, node );
    if ( !is ) return;

    indexedshape_ = is;
    Shape::replaceShape( node );
}


#define setGetIndex( resourcename, fieldname )  \
int IndexedShape::nr##resourcename##Index() const \
{ return indexedshape_->fieldname.getNum(); } \
 \
 \
void IndexedShape::set##resourcename##Index( int pos, int idx ) \
{ indexedshape_->fieldname.set1Value( pos, idx ); } \
 \
 \
void IndexedShape::remove##resourcename##IndexAfter(int pos) \
{  \
    if ( indexedshape_->fieldname.getNum()>pos+1 ) \
	indexedshape_->fieldname.deleteValues(pos+1); \
} \
 \
 \
int IndexedShape::get##resourcename##Index( int pos ) const \
{ return indexedshape_->fieldname[pos]; } \
 \
 \
void IndexedShape::set##resourcename##Indices( const int* ptr, int sz ) \
{ return indexedshape_->fieldname.setValuesPointer( sz, ptr ); } \
\
void IndexedShape::set##resourcename##Indices( const int* ptr, int sz, \
					       int start ) \
{ return indexedshape_->fieldname.setValues( start, sz, ptr ); } \


setGetIndex( Coord, coordIndex );
setGetIndex( TextureCoord, textureCoordIndex );
setGetIndex( Normal, normalIndex );
setGetIndex( Material, materialIndex );


void IndexedShape::copyCoordIndicesFrom( const IndexedShape& is )
{
    indexedshape_->coordIndex = is.indexedshape_->coordIndex;
}


int IndexedShape::getClosestCoordIndex( const EventInfo& ei ) const
{
    mDynamicCastGet(const FaceDetail*,facedetail,ei.detail)
    if ( !facedetail ) return -1;

    return facedetail->getClosestIdx( getCoordinates(), ei.localpickedpos );
}
    
    
class osgPrimitive : public Geometry::IndexedPrimitive
{
public:
    osgPrimitive() : element_( new osg::DrawElementsUInt) {}
    
    virtual void	push( int ) {}
    virtual int		pop() { return 0; }
    virtual int		size() const { return 0; }
    virtual int		get(int) const { return 0; }
    virtual int		set(int) const { return 0; }
    
    osg::ref_ptr<osg::DrawElementsUInt>	element_;
};
    
    
Geometry::IndexedPrimitive* visBase::IndexedShape::createPrimitive()
{
    return new osgPrimitive;
}
    
    
void visBase::IndexedShape::addPrimitive( Geometry::IndexedPrimitive* p )
{
    primitives_ += p;
}
    
} // namespace visBase
