/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vistexturerect.cc,v 1.50 2011-02-10 05:11:27 cvssatyaki Exp $";

#include "vistexturerect.h"
#include "iopar.h"
#include "visrectangle.h"
#include "arrayndimpl.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "vistexture2.h"
#include "viscoltabmod.h"

mCreateFactoryEntry( visBase::TextureRect );

namespace visBase
{

const char* TextureRect::rectangleidstr()  { return "Rectangle ID"; }
const char* TextureRect::textureidstr()    { return "Texture ID"; }

TextureRect::TextureRect()
    : VisualObjectImpl( false )
    , rectangle( 0 )
    , manipstartnotifier( this )
    , manipchnotifier( this )
    , manipendsnotifier( this )
{
    textureset = Texture2Set::create();
    textureset->ref();
    addChild( textureset->getInventorNode() );
    textureset->addTexture( Texture2::create() );
    textureset->setActiveTexture( 0 );
    useTexture( true );

    setRectangle( Rectangle::create() );
}


TextureRect::~TextureRect()
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


void TextureRect::addTexture()
{
    textureset->addTexture( Texture2::create() );
}


void TextureRect::showTexture( int idx )
{
    textureset->setActiveTexture( idx );
}


int TextureRect::shownTexture() const
{
    return textureset->activeTextureNr();
}


void TextureRect::removeAllTextures( bool keepfirst )
{
    textureset->removeAll( keepfirst );
}


void TextureRect::setRectangle( Rectangle* nr )
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


const Rectangle& TextureRect::getRectangle() const
{ return *rectangle; }
 
 
Rectangle& TextureRect::getRectangle()
{ return *rectangle; }


#define mTextureSet(fn,arg) \
    for ( int idx=0; idx<textureset->nrTextures(); idx++ ) \
        textureset->getTexture(idx)->fn( arg );

#define mTextureGet(fn) \
    return textureset->activeTexture()->fn();
 

void TextureRect::setColorTab( VisColorTab& ct )
{ mTextureSet(setColorTab,ct) }

const VisColorTab& TextureRect::getColorTab() const
{ mTextureGet(getColorTab) }
 
VisColorTab& TextureRect::getColorTab()
{ mTextureGet(getColorTab) }

void TextureRect::setClipRate( Interval<float> cr )
{ mTextureSet(setClipRate,cr) }

Interval<float> TextureRect::clipRate() const 
{ mTextureGet(clipRate) } 

void  TextureRect::setAutoScale( bool yn )
{ mTextureSet(setAutoScale,yn) }

bool TextureRect::autoScale() const
{ mTextureGet(autoScale) }

void TextureRect::useTexture( bool yn )
{ mTextureSet(turnOn,yn) }

bool TextureRect::usesTexture() const
{ mTextureGet(isOn) }

void TextureRect::setTextureQuality( float q )
{ mTextureSet(setTextureQuality,q) }

float TextureRect::getTextureQuality() const
{ mTextureGet(getTextureQuality) }

void TextureRect::setResolution( int res )
{ mTextureSet(setResolution,res) }

int TextureRect::getResolution() const
{ mTextureGet(getResolution) }

int TextureRect::getNrResolutions() const
{ return 3; }


void TextureRect::setData( const Array2D<float>* data, int textureidx, 
				    int datatype )
{
    Texture2* text = textureset->getTexture( textureidx );
    if ( !text ) return;

    text->setData( data, (Texture::DataType)datatype );
}


const TypeSet<float>& TextureRect::getHistogram() const
{
    return textureset->activeTexture()->getHistogram();
}


void TextureRect::setColorPars( bool rev, bool useclip, 
					 const Interval<float>& intv )
{
    VisColTabMod& ctm = textureset->activeTexture()->getColTabMod();
    ctm.doReverse( rev );
    ctm.useClipping( useclip );
    useclip ? ctm.setClipRate( intv.start, intv.stop ) : ctm.setRange( intv );
}


const Interval<float>& TextureRect::getColorDataRange() const
{
    return textureset->activeTexture()->getColTabMod().getRange();
}


void TextureRect::finishTextures()
{
    textureset->finishTextures();
}


void TextureRect::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    int rectid = rectangle->id();
    par.set( rectangleidstr(), rectid );

    int textureid = textureset->getTexture(0)->id();
    par.set( textureidstr(), textureid );

    if ( saveids.indexOf(rectid) == -1 ) saveids += rectid;
    if ( saveids.indexOf(textureid) == -1 ) saveids += textureid;
}


int TextureRect::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    int textureid;
    if ( !par.get( textureidstr(), textureid ) ) return -1;
    DataObject* dataobj = DM().getObject( textureid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(Texture2*,texture_,dataobj);
    if ( !texture_ ) return -1;
    removeAllTextures(false);
    textureset->addTexture( texture_ );

    int rectid;
    if ( !par.get( rectangleidstr(), rectid ) ) return -1;
    dataobj = DM().getObject( rectid );
    if ( !dataobj ) return 0;
    mDynamicCastGet( Rectangle*, rect, dataobj );
    if ( !rect ) return -1;

    setRectangle( rect );

    return 1;
}

}; // namespace visBase
