/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vistexture3viewer.cc,v 1.1 2002-11-08 12:21:17 kristofer Exp $";


#include "vistexture3viewer.h"
#include "Inventor/nodes/SoRotation.h"
#include "Inventor/nodes/SoTexture3.h"
#include "Inventor/nodes/SoGroup.h"
#include "Inventor/nodes/SoTextureCoordinate3.h"
#include "SoTranslateRectangleDragger.h"

mCreateFactoryEntry( visBase::MovableTextureSlice );
mCreateFactoryEntry( visBase::Texture3Viewer );

visBase::Texture3Viewer::Texture3Viewer()
    : texture( 0 )
{
    textureobjects.allowNull();
}


visBase::Texture3Viewer::~Texture3Viewer()
{
    for ( int idx=0; idx<textureobjects.size(); idx++ )
	removeSlice( idx );

    if ( texture ) texture->unref();
}


int visBase::Texture3Viewer::getNrSlices() const
{ return textureobjects.size(); }


int visBase::Texture3Viewer::addSlice( int dim, float origpos )
{
    MovableTextureSlice* slice = MovableTextureSlice::create();
    slice->setDim( dim );
    slice->setPosition( origpos );
    slice->ref();
    if ( texture ) slice->setTexture( texture );
    textureobjects += slice;
    addChild( slice->getData() );
    return textureobjects.size()-1;
}


void visBase::Texture3Viewer::removeSlice( int idx )
{
    if ( !textureobjects[idx] ) return;

    removeChild( textureobjects[idx]->getData() );
    textureobjects[idx]->unRef();

    textureobjects.replace( 0, idx );
}


void visBase::Texture3Viewer::showSlice( int idx, bool yn )
{
    if ( !textureobjects[idx] ) return;

    textureobjects[idx]->turnOn( yn );
}


bool visBase::Texture3Viewer::isSliceShown( int idx ) const
{
    if ( !textureobjects[idx] ) return false;

    return textureobjects[idx]->isOn();
}


void visBase::Texture3Viewer::setTexture( SoTexture3* nt )
{
    if ( texture ) texture->unref();
    texture = nt;
    texture->ref();

    for ( int idx=0; idx<textureobjects.size(); idx++ )
    {
	if ( !textureobjects[idx] ) continue;

	textureobjects[idx]->setTexture( nt );
    }
}


visBase::MovableTextureSlice::MovableTextureSlice()
    : rotation( new SoRotation )
    , dragger( new SoTranslateRectangleDragger )
    , group( new SoGroup )
    , dim_( 0 )
{
    addChild( rotation );
    addChild( dragger );
    group->ref();
    fieldsensor =
	new SoFieldSensor( &visBase::MovableTextureSlice::fieldsensorCB, this );
    fieldsensor->attach( &dragger->translation );
}


visBase::MovableTextureSlice::~MovableTextureSlice()
{
    group->unref();
}


int visBase::MovableTextureSlice::dim() const { return dim_; }


void visBase::MovableTextureSlice::setDim( int nd )
{
    dim_ = nd;
    if ( !nd ) rotation->rotation.setValue( SbVec3f( 1, 0, 0 ), 0 );
    else if ( nd==1 ) rotation->rotation.setValue( SbVec3f( 0, 0, 1 ), M_PI_2 );
    else rotation->rotation.setValue( SbVec3f( 0, 1, 0 ), M_PI_2 );
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


void visBase::MovableTextureSlice::setTexture( SoTexture3* text )
{
    group->unref();
    group = new SoGroup;
    group->ref();
    group->addChild( text );
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
	myself->texturecoords->point.set1Value( 0, SbVec3f( (pos+1)/2, 0, 0 ));
	myself->texturecoords->point.set1Value( 1, SbVec3f( (pos+1)/2, 0, 1 ));
	myself->texturecoords->point.set1Value( 2, SbVec3f( (pos+1)/2, 1, 1 ));
	myself->texturecoords->point.set1Value( 3, SbVec3f( (pos+1)/2, 1, 0 ));
	myself->texturecoords->point.set1Value( 4, SbVec3f( (pos+1)/2, 0, 0 ));
    }
    else if ( myself->dim_==1 )
    {
	myself->texturecoords->point.set1Value( 0, SbVec3f( 0, -(pos-1)/2, 0 ));
	myself->texturecoords->point.set1Value( 1, SbVec3f( 0, -(pos-1)/2, 1 ));
	myself->texturecoords->point.set1Value( 2, SbVec3f( 1, -(pos-1)/2, 1 ));
	myself->texturecoords->point.set1Value( 3, SbVec3f( 1, -(pos-1)/2, 0 ));
	myself->texturecoords->point.set1Value( 4, SbVec3f( 0, -(pos-1)/2, 0 ));
    }
    else
    {
	myself->texturecoords->point.set1Value( 0, SbVec3f( 0, 0, (pos+1)/2 ));
	myself->texturecoords->point.set1Value( 1, SbVec3f( 1, 0, (pos+1)/2 ));
	myself->texturecoords->point.set1Value( 2, SbVec3f( 1, 1, (pos+1)/2 ));
	myself->texturecoords->point.set1Value( 3, SbVec3f( 0, 1, (pos+1)/2 ));
	myself->texturecoords->point.set1Value( 4, SbVec3f( 0, 0, (pos+1)/2 ));
    }

}


