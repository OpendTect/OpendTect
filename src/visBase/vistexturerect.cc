/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vistexturerect.cc,v 1.27 2003-02-14 11:49:54 nanne Exp $";

#include "vistexturerect.h"
#include "iopar.h"
#include "visrectangle.h"
#include "arrayndimpl.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "vistexture2.h"
#include "ptrman.h"
#include "position.h"

#include <math.h>

mCreateFactoryEntry( visBase::TextureRect );


const char* visBase::TextureRect::texturequalitystr = "Texture quality";
const char* visBase::TextureRect::rectangleidstr = "Rectangle ID";
const char* visBase::TextureRect::usestexturestr = "Uses texture";
const char* visBase::TextureRect::resolutionstr = "Resolution";

visBase::TextureRect::TextureRect()
    : texture(0)
    , rectangle( 0 )
    , manipstartnotifier( this )
    , manipchnotifier( this )
    , manipendsnotifier( this )
    , resolution(0)
{
    setTexture( *visBase::Texture2::create() );
    useTexture( true );

    setRectangle( visBase::Rectangle::create() );
}


visBase::TextureRect::~TextureRect()
{
    if ( rectangle )
    {
	rectangle->manipStarts()->remove(
				mCB( this, TextureRect, triggerManipStarts ));
	rectangle->manipChanges()->remove(
				mCB( this, TextureRect, triggerManipChanges ));
	rectangle->manipEnds()->remove(
				mCB( this, TextureRect, triggerManipEnds ));
	rectangle->selection()->remove(
				mCB( this, TextureRect, triggerSel ));
	rectangle->deSelection()->remove(
				mCB( this, TextureRect, triggerDeSel ));
	rectangle->unRef();
    }

    if ( texture )
    {
	removeChild( texture->getData() );
	texture->unRef();
    }
}


void visBase::TextureRect::setTexture( visBase::Texture2& newtext )
{
    if ( texture )
    {
	removeChild( texture->getData() );
	texture->unRef();
    }

    texture = &newtext;
    texture->ref();
    insertChild( 1, texture->getData() );
}


visBase::Texture2& visBase::TextureRect::getTexture()
{
    return *texture;
}


void visBase::TextureRect::setRectangle( Rectangle* nr )
{
    if ( rectangle )
    {
	rectangle->manipStarts()->remove(
				mCB(this,TextureRect,triggerManipStarts) );
	rectangle->manipChanges()->remove(
				mCB(this,TextureRect,triggerManipChanges) );
	rectangle->manipEnds()->remove(
				mCB(this,TextureRect,triggerManipEnds) );
	rectangle->selection()->remove( mCB(this,TextureRect,triggerSel) );
	rectangle->deSelection()->remove( mCB(this,TextureRect,triggerDeSel) );

	removeChild( rectangle->getData() );
	rectangle->unRef();
    }

    rectangle = nr;
    rectangle->ref();
    addChild( rectangle->getData() );
    rectangle->setMaterial( 0 );

    rectangle->manipStarts()->notify( mCB(this,TextureRect,triggerManipStarts));
    rectangle->manipChanges()->notify( 
	    		       mCB(this,TextureRect,triggerManipChanges) );
    rectangle->manipEnds()->notify( mCB(this,TextureRect,triggerManipEnds) );
    rectangle->selection()->notify( mCB(this,TextureRect,triggerSel) );
    rectangle->deSelection()->notify( mCB(this,TextureRect,triggerDeSel) );
}


const visBase::Rectangle& visBase::TextureRect::getRectangle() const
{ return *rectangle; }
 
 
visBase::Rectangle& visBase::TextureRect::getRectangle()
{ return *rectangle; }
 

void visBase::TextureRect::setColorTab( VisColorTab& ct )
{
    texture->setColorTab( ct );
}


const visBase::VisColorTab& visBase::TextureRect::getColorTab() const
{ return texture->getColorTab(); }
 
 
visBase::VisColorTab& visBase::TextureRect::getColorTab()
{ return texture->getColorTab(); }


void visBase::TextureRect::setClipRate( float cr )
{
    texture->setClipRate( cr );
}


float visBase::TextureRect::clipRate() const 
{ 
    return texture->clipRate();
}


void  visBase::TextureRect::setAutoScale( bool yn )
{
    texture->setAutoScale( yn );
}


bool visBase::TextureRect::autoScale() const
{
    return texture->autoScale();
}


void visBase::TextureRect::useTexture( bool yn )
{
    texture->turnOn( yn );
}


bool visBase::TextureRect::usesTexture() const
{
    return texture->isOn();
}


void visBase::TextureRect::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    int rectid = rectangle->id();

    par.set( texturequalitystr, getTextureQuality() );
    par.set( rectangleidstr, rectid );
    par.setYN( usestexturestr, usesTexture() );
    par.set( resolutionstr, resolution );

    if ( saveids.indexOf( rectid )==-1 ) saveids += rectid;
}


int visBase::TextureRect::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!= 1 ) return res;

    int newres = 0;
    par.get( resolutionstr, newres );
    resolution = newres;

    float texturequality;
    if ( !par.get( texturequalitystr, texturequality )) return -1;
    setTextureQuality( texturequality );

    int rectid;
    if ( !par.get( rectangleidstr, rectid ) ) return -1;
    DataObject* dataobj = DM().getObj( rectid );
    if ( !dataobj ) return 0;
    mDynamicCastGet( Rectangle*, rect, dataobj );
    if ( !rect ) return -1;

    setRectangle( rect );

    bool usetext;
    if ( !par.getYN( usestexturestr, usetext )) return -1;
    useTexture(usetext);

    return 1;
}
    

void visBase::TextureRect::setData( const Array2D<float>& data )
{
    texture->setData( &data );
}


void visBase::TextureRect::setTextureQuality( float q )
{
    texture->setTextureQuality( q );
}


float visBase::TextureRect::getTextureQuality() const
{
    return texture->getTextureQuality();
}


int visBase::TextureRect::getNrResolutions() const
{
    return 3;
}


void visBase::TextureRect::setResolution( int res )
{
    resolution = res;
}
