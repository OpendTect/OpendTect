/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vistexturerect.cc,v 1.41 2004-01-29 10:11:04 nanne Exp $";

#include "vistexturerect.h"
#include "iopar.h"
#include "visrectangle.h"
#include "arrayndimpl.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "vistexture2.h"
#include "viscoltabmod.h"


mCreateFactoryEntry( visBase::TextureRect );


const char* visBase::TextureRect::rectangleidstr = "Rectangle ID";
const char* visBase::TextureRect::textureidstr = "Texture ID";

visBase::TextureRect::TextureRect()
    : rectangle( 0 )
    , manipstartnotifier( this )
    , manipchnotifier( this )
    , manipendsnotifier( this )
{
    textureset = visBase::Texture2Set::create();
    textureset->ref();
    addChild( textureset->getInventorNode() );
    textureset->addTexture( visBase::Texture2::create() );
    textureset->setActiveTexture( 0 );
    useTexture( true );

    setRectangle( visBase::Rectangle::create() );
}


visBase::TextureRect::~TextureRect()
{
    if ( rectangle )
    {
	rectangle->manipStarts()->remove(
				mCB(this,TextureRect,triggerManipStarts) );
	rectangle->manipChanges()->remove(
				mCB(this,TextureRect,triggerManipChanges) );
	rectangle->manipEnds()->remove(
				mCB(this,TextureRect,triggerManipEnds) );
	rectangle->selection()->remove(
				mCB(this,TextureRect,triggerSel) );
	rectangle->deSelection()->remove(
				mCB(this,TextureRect,triggerDeSel) );
	removeChild( rectangle->getInventorNode() );
	rectangle->unRef();
    }

    removeChild( textureset->getInventorNode() );
    textureset->unRef();
}


void visBase::TextureRect::addTexture()
{
    textureset->addTexture( visBase::Texture2::create() );
}


void visBase::TextureRect::showTexture( int idx )
{
    textureset->setActiveTexture( idx );
}


void visBase::TextureRect::removeAllTextures( bool keepfirst )
{
    textureset->removeAll( keepfirst );
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

	removeChild( rectangle->getInventorNode() );
	rectangle->unRef();
    }

    rectangle = nr;
    rectangle->ref();
    addChild( rectangle->getInventorNode() );
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


#define mTextureSet(fn,arg) \
    for ( int idx=0; idx<textureset->nrTextures(); idx++ ) \
        textureset->getTexture(idx)->fn( arg );

#define mTextureGet(fn) \
    return textureset->activeTexture()->fn();
 

void visBase::TextureRect::setColorTab( VisColorTab& ct )
{ mTextureSet(setColorTab,ct) }

const visBase::VisColorTab& visBase::TextureRect::getColorTab() const
{ mTextureGet(getColorTab) }
 
visBase::VisColorTab& visBase::TextureRect::getColorTab()
{ mTextureGet(getColorTab) }

void visBase::TextureRect::setClipRate( float cr )
{ mTextureSet(setClipRate,cr) }

float visBase::TextureRect::clipRate() const 
{ mTextureGet(clipRate) } 

void  visBase::TextureRect::setAutoScale( bool yn )
{ mTextureSet(setAutoScale,yn) }

bool visBase::TextureRect::autoScale() const
{ mTextureGet(autoScale) }

void visBase::TextureRect::useTexture( bool yn )
{ mTextureSet(turnOn,yn) }

bool visBase::TextureRect::usesTexture() const
{ mTextureGet(isOn) }

void visBase::TextureRect::setTextureQuality( float q )
{ mTextureSet(setTextureQuality,q) }

float visBase::TextureRect::getTextureQuality() const
{ mTextureGet(getTextureQuality) }

void visBase::TextureRect::setResolution( int res )
{ mTextureSet(setResolution,res) }

int visBase::TextureRect::getResolution() const
{ mTextureGet(getResolution) }

int visBase::TextureRect::getNrResolutions() const
{ return 3; }


void visBase::TextureRect::setData( const Array2D<float>* data, int textureidx, 
				    int datatype )
{
    visBase::Texture2* text = textureset->getTexture( textureidx );
    if ( !text ) return;

    text->setData( data, (Texture::DataType)datatype );
}


const TypeSet<float>& visBase::TextureRect::getHistogram() const
{
    return textureset->activeTexture()->getHistogram();
}


void visBase::TextureRect::setColorPars( bool rev, bool useclip, 
					 const Interval<float>& intv )
{
    VisColTabMod& ctm = textureset->activeTexture()->getColTabMod();
    ctm.doReverse( rev );
    ctm.useClipping( useclip );
    useclip ? ctm.setClipRate( intv.start, intv.stop ) : ctm.setRange( intv );
}


const Interval<float>& visBase::TextureRect::getColorDataRange() const
{
    return textureset->activeTexture()->getColTabMod().getRange();
}


void visBase::TextureRect::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    int rectid = rectangle->id();
    par.set( rectangleidstr, rectid );

    int textureid = textureset->getTexture(0)->id();
    par.set( textureidstr, textureid );

    if ( saveids.indexOf(rectid) == -1 ) saveids += rectid;
    if ( saveids.indexOf(textureid) == -1 ) saveids += textureid;
}


int visBase::TextureRect::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    int textureid;
    if ( !par.get( textureidstr, textureid ) ) return -1;
    DataObject* dataobj = DM().getObj( textureid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(Texture2*,texture_,dataobj);
    if ( !texture_ ) return -1;
    removeAllTextures(false);
    textureset->addTexture( texture_ );

    int rectid;
    if ( !par.get( rectangleidstr, rectid ) ) return -1;
    dataobj = DM().getObj( rectid );
    if ( !dataobj ) return 0;
    mDynamicCastGet( Rectangle*, rect, dataobj );
    if ( !rect ) return -1;

    setRectangle( rect );

    return 1;
}
