/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vistexture3viewer.cc,v 1.17 2004-01-05 09:43:23 kristofer Exp $";


#include "vistexture3viewer.h"

#include "visdataman.h"
#include "vistexture3.h"
#include "visselman.h"
#include "iopar.h"

#include "Inventor/nodes/SoRotation.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoFaceSet.h"
#include "Inventor/nodes/SoGroup.h"
#include "Inventor/nodes/SoTextureCoordinate3.h"
#include "Inventor/nodes/SoShapeHints.h"
#include "SoTranslateRectangleDragger.h"


const char* visBase::Texture3Viewer::textureidstr = "Texture ID";

mCreateFactoryEntry( visBase::Texture3Viewer );
mCreateFactoryEntry( visBase::MovableTextureSlice );
mCreateFactoryEntry( visBase::Texture3Slice );

visBase::Texture3Viewer::Texture3Viewer()
    : texture( 0 )
{
    setTexture( *visBase::Texture3::create() );
    textureobjects.allowNull();
}


visBase::Texture3Viewer::~Texture3Viewer()
{
    for ( int idx=0; idx<textureobjects.size(); idx++ )
    {
	if ( textureobjects[idx] )
	    removeObject( textureobjects[idx]->id() );
    }

    if ( texture ) texture->unRef();
}


int visBase::Texture3Viewer::addSlice( int dim, float origpos )
{
    MovableTextureSlice* slice = MovableTextureSlice::create();
    slice->setDim( dim );
    slice->setPosition( origpos );
    slice->ref();
    if ( texture ) slice->setTexture( *texture );
    textureobjects += slice;
    addChild( slice->getInventorNode() );
    return slice->id();
} 


float visBase::Texture3Viewer::slicePosition( int idnumber )
{
    for ( int idx=0; idx<textureobjects.size(); idx++ )
    {
	if ( !textureobjects[idx] ) continue;
	if ( textureobjects[idx]->id()!=idnumber ) continue;

	return textureobjects[idx]->position();
    }

    return 0;
}


int visBase::Texture3Viewer::getNrObjects() const
{ return textureobjects.size(); }


void visBase::Texture3Viewer::removeObject( int idnumber )
{
    for ( int idx=0; idx<textureobjects.size(); idx++ )
    {
	if ( !textureobjects[idx] ) continue;
	if ( textureobjects[idx]->id()!=idnumber ) continue;

	removeChild( textureobjects[idx]->getInventorNode() );
	textureobjects[idx]->unRef();

	textureobjects.replace( 0, idx );
	return;
    }
}


void visBase::Texture3Viewer::showObject( int idnumber, bool yn )
{
    for ( int idx=0; idx<textureobjects.size(); idx++ )
    {
	if ( !textureobjects[idx] ) continue;
	if ( textureobjects[idx]->id()!=idnumber ) continue;

	textureobjects[idx]->turnOn( yn );
	return;
    }
}


bool visBase::Texture3Viewer::isObjectShown( int idnumber ) const
{
    for ( int idx=0; idx<textureobjects.size(); idx++ )
    {
	if ( !textureobjects[idx] ) continue;
	if ( textureobjects[idx]->id()!=idnumber ) continue;

	return textureobjects[idx]->isOn();
    }

    return false;
}


void visBase::Texture3Viewer::setTexture( Texture3& nt )
{
    if ( texture ) texture->unRef();
    texture = &nt;
    texture->ref();

    for ( int idx=0; idx<textureobjects.size(); idx++ )
    {
	if ( !textureobjects[idx] ) continue;

	textureobjects[idx]->setTexture( nt );
    }
}


visBase::Texture3& visBase::Texture3Viewer::getTexture()
{ return *texture; }


void visBase::Texture3Viewer::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    int textureid = texture->id();
    par.set( textureidstr, textureid );

    if ( saveids.indexOf(textureid) == -1 ) saveids += textureid;
}


int visBase::Texture3Viewer::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    int textureid;
    if ( !par.get( textureidstr, textureid ) ) return 1; // old session file
    DataObject* dobj = DM().getObj( textureid );
    if ( !dobj ) return 0;
    mDynamicCastGet(Texture3*,texture_,dobj)
    if ( !texture_ ) return -1;

    setTexture( *texture_ );

    return 1;
}



visBase::Texture3Slice::Texture3Slice()
    : coords( new SoCoordinate3 )
    , Texture3ViewerObject( true )
    , texturecoords( new SoTextureCoordinate3 )
    , faces( new SoFaceSet )
    , pos( 0 )
    , texture( 0 )
    , dim_( 0 )
{
    addChild( texturecoords );
    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering.setValue( SoShapeHints::CLOCKWISE );
    hints->shapeType.setValue( SoShapeHints::UNKNOWN_SHAPE_TYPE );
    addChild( hints );
    addChild( coords );
    setUpCoords();
    addChild( faces );
    faces->numVertices.setValue( 5 );
}


visBase::Texture3Slice::~Texture3Slice()
{
    if ( texture ) texture->unRef();
}


int visBase::Texture3Slice::dim() const { return dim_; }


void visBase::Texture3Slice::setDim( int nd )
{
    dim_ = nd;
    setUpCoords();
}

float visBase::Texture3Slice::position() const { return pos; }


void visBase::Texture3Slice::setPosition( float np )
{
    pos = np;
    setUpCoords();
}


void visBase::Texture3Slice::setTexture( Texture3& nt )
{
    if ( texture )
    {
	removeChild( texture->getInventorNode() );
	texture->unRef();
    }

    texture = &nt;
    texture->ref();
    insertChild( 0, texture->getInventorNode() );
}

void visBase::Texture3Slice::setUpCoords()
{
    if ( !dim_ )
    {
	coords->point.set1Value(0, SbVec3f(pos,-1,-1) );
	coords->point.set1Value(1, SbVec3f(pos,-1, 1) );
	coords->point.set1Value(2, SbVec3f(pos, 1, 1) );
	coords->point.set1Value(3, SbVec3f(pos, 1,-1) );
	coords->point.set1Value(4, SbVec3f(pos,-1,-1) );

	const float tpos = (pos+1)/2;
	texturecoords->point.set1Value(0, SbVec3f(tpos, 0, 0) );
	texturecoords->point.set1Value(1, SbVec3f(tpos, 0, 1) );
	texturecoords->point.set1Value(2, SbVec3f(tpos, 1, 1) );
	texturecoords->point.set1Value(3, SbVec3f(tpos, 1, 0) );
	texturecoords->point.set1Value(4, SbVec3f(tpos, 0, 0) );
    }
    else if ( dim_==1 )
    {
	coords->point.set1Value( 0, SbVec3f( 1, pos, 1 ));
	coords->point.set1Value( 1, SbVec3f( 1, pos,-1 ));
	coords->point.set1Value( 2, SbVec3f(-1, pos,-1 ));
	coords->point.set1Value( 3, SbVec3f(-1, pos, 1 ));
	coords->point.set1Value( 4, SbVec3f( 1, pos, 1 ));

	const float tpos = (pos+1)/2;
	texturecoords->point.set1Value( 0, SbVec3f( 1, tpos, 1 ));
	texturecoords->point.set1Value( 1, SbVec3f( 1, tpos, 0 ));
	texturecoords->point.set1Value( 2, SbVec3f( 0, tpos, 0 ));
	texturecoords->point.set1Value( 3, SbVec3f( 0, tpos, 1 ));
	texturecoords->point.set1Value( 4, SbVec3f( 1, tpos, 1 ));
    }
    else
    {
	coords->point.set1Value( 0, SbVec3f(-1,-1, pos ));
	coords->point.set1Value( 1, SbVec3f( 1,-1, pos ));
	coords->point.set1Value( 2, SbVec3f( 1, 1, pos ));
	coords->point.set1Value( 3, SbVec3f(-1, 1, pos ));
	coords->point.set1Value( 4, SbVec3f(-1,-1, pos ));

	const float tpos = (pos+1)/2;
	texturecoords->point.set1Value( 0, SbVec3f( 0, 0, tpos ));
	texturecoords->point.set1Value( 1, SbVec3f( 1, 0, tpos ));
	texturecoords->point.set1Value( 2, SbVec3f( 1, 1, tpos ));
	texturecoords->point.set1Value( 3, SbVec3f( 0, 1, tpos ));
	texturecoords->point.set1Value( 4, SbVec3f( 0, 0, tpos ));
    }
}



visBase::MovableTextureSlice::MovableTextureSlice()
    : rotation( new SoRotation )
    , Texture3ViewerObject( true )
    , dragger( new SoTranslateRectangleDragger )
    , group( new SoGroup )
    , texturecoords( new SoTextureCoordinate3 )
    , dim_( 0 )
    , motion(this)
    , texture( 0 )
{
    addChild( rotation );
    addChild( dragger );
    group->ref();
    group->addChild( texturecoords );
    fieldsensor =
	new SoFieldSensor( &visBase::MovableTextureSlice::fieldsensorCB, this );
    fieldsensor->attach( &dragger->translation );

    dragger->addStartCallback( visBase::MovableTextureSlice::startCB, this );
    dragger->addMotionCallback( visBase::MovableTextureSlice::motionCB, this );
}


visBase::MovableTextureSlice::~MovableTextureSlice()
{
    dragger->removeStartCallback(
	    visBase::MovableTextureSlice::startCB, this );
    dragger->removeMotionCallback( 
	    visBase::MovableTextureSlice::motionCB, this );
    group->unref();

    if ( texture ) texture->unRef();
}


int visBase::MovableTextureSlice::dim() const { return dim_; }


void visBase::MovableTextureSlice::setDim( int nd )
{
    dim_ = nd;
    if ( !nd ) rotation->rotation.setValue( SbVec3f( 1, 0, 0 ), 0 );
    else if ( nd==1 ) rotation->rotation.setValue( SbVec3f( 0, 0, 1 ), M_PI_2 );
    else rotation->rotation.setValue( SbVec3f( 0, -1, 0 ), -M_PI_2 );

    fieldsensorCB( this, 0 );
}


float visBase::MovableTextureSlice::position() const
{
    return dragger->translation.getValue()[0];
}


void visBase::MovableTextureSlice::setPosition(float nv)
{
    SbVec3f pos = dragger->translation.getValue();
    pos[0] = nv;
    dragger->translation.setValue( pos );
}


void visBase::MovableTextureSlice::setTexture( Texture3& text )
{
    if ( texture )
    {
	texture->unRef();
    }

    texture = &text;
    texture->ref();

    group->unref();
    group = new SoGroup;
    group->ref();
    group->addChild( texture->getInventorNode() );
    texturecoords = new SoTextureCoordinate3;
    group->addChild( texturecoords );

    fieldsensorCB( this, 0 );
    dragger->setPart("prefixgroup", group );
}


void visBase::MovableTextureSlice::fieldsensorCB( void* inst, SoSensor* )
{
    visBase::MovableTextureSlice* myself = (visBase::MovableTextureSlice*) inst;

    float pos = myself->position();
    if ( !myself->dim_ )
    {
	const float tpos = (pos+1)/2;
	myself->texturecoords->point.set1Value(0, SbVec3f(tpos, 0, 0) );
	myself->texturecoords->point.set1Value(1, SbVec3f(tpos, 0, 1) );
	myself->texturecoords->point.set1Value(2, SbVec3f(tpos, 1, 1) );
	myself->texturecoords->point.set1Value(3, SbVec3f(tpos, 1, 0) );
	myself->texturecoords->point.set1Value(4, SbVec3f(tpos, 0, 0) );
    }
    else if ( myself->dim_==1 )
    {
	const float tpos = (pos+1)/2;
	myself->texturecoords->point.set1Value( 0, SbVec3f( 1, tpos, 0 ));
	myself->texturecoords->point.set1Value( 1, SbVec3f( 1, tpos, 1 ));
	myself->texturecoords->point.set1Value( 2, SbVec3f( 0, tpos, 1 ));
	myself->texturecoords->point.set1Value( 3, SbVec3f( 0, tpos, 0 ));
	myself->texturecoords->point.set1Value( 4, SbVec3f( 1, tpos, 0 ));
    }
    else
    {
	const float tpos = (pos+1)/2;
	myself->texturecoords->point.set1Value( 0, SbVec3f( 0, 0, 1-tpos ));
	myself->texturecoords->point.set1Value( 1, SbVec3f( 1, 0, 1-tpos ));
	myself->texturecoords->point.set1Value( 2, SbVec3f( 1, 1, 1-tpos ));
	myself->texturecoords->point.set1Value( 3, SbVec3f( 0, 1, 1-tpos ));
	myself->texturecoords->point.set1Value( 4, SbVec3f( 0, 0, 1-tpos ));
    }

}


void visBase::MovableTextureSlice::startCB( void* obj, SoDragger* )
{
    MovableTextureSlice* myself = (MovableTextureSlice*)obj;
    visBase::DM().selMan().select( myself->id() );
}


void visBase::MovableTextureSlice::motionCB( void* obj, SoDragger* )
{
    ( (visBase::MovableTextureSlice*)obj )->motion.trigger();
}

