/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vistexturerect.cc,v 1.38 2003-06-06 14:09:04 nanne Exp $";

#include <Inventor/nodes/SoSwitch.h>

#include "vistexturerect.h"
#include "iopar.h"
#include "visrectangle.h"
#include "arrayndimpl.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "vistexture2.h"
#include "viscoltabmod.h"
#include "ptrman.h"
#include "position.h"

#include <math.h>

mCreateFactoryEntry( visBase::TextureRect );


const char* visBase::TextureRect::rectangleidstr = "Rectangle ID";
const char* visBase::TextureRect::textureidstr = "Texture ID";

visBase::TextureRect::TextureRect()
    : rectangle( 0 )
    , manipstartnotifier( this )
    , manipchnotifier( this )
    , manipendsnotifier( this )
    , textureswitch( new SoSwitch )
    , curidx(0)
{
    textureswitch->ref();
    insertChild( 1, textureswitch );

    visBase::Texture2* text = visBase::Texture2::create();
    setTexture( *text, 0 );
    
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
	rectangle->unRef();
    }

    for ( int idx=0; idx<textureset.size(); idx++ )
    {
	visBase::Texture2* text = textureset[idx];
	textureswitch->removeChild( text->getData() );
	text->unRef();
    }

    textureswitch->unref();
}


void visBase::TextureRect::setTexture( visBase::Texture2& newtext, int idx )
{
    visBase::Texture2* text = idx < textureset.size() ? textureset[idx] : 0;
    if ( text )
    {
	textureswitch->removeChild( text->getData() );
	text->unRef();
	textureset -= text;
    }
    
    if ( textureset.size() )
    {
	newtext.setResolution( textureset[0]->getResolution() );
//	newtext.setColorTab( textureset[0]->getColorTab() );
	newtext.getColorTab().setColorSeq( 
			    &textureset[0]->getColorTab().colorSeq() );
    }
    textureset += &newtext;

    text = &newtext;
    text->ref();
    textureswitch->insertChild( text->getData(), idx );
}


visBase::Texture2& visBase::TextureRect::getTexture( int idx )
{
    return *textureset[idx];
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
    for ( int idx=0; idx<textureset.size(); idx++ )
	textureset[idx]->setColorTab( ct );
}


const visBase::VisColorTab& visBase::TextureRect::getColorTab() const
{ return textureset[curidx]->getColorTab(); }
 
 
visBase::VisColorTab& visBase::TextureRect::getColorTab()
{ return textureset[curidx]->getColorTab(); }


void visBase::TextureRect::setClipRate( float cr )
{
    for ( int idx=0; idx<textureset.size(); idx++ )
	textureset[idx]->setClipRate( cr );
}


float visBase::TextureRect::clipRate() const 
{ 
    return textureset[curidx]->clipRate();
}


void  visBase::TextureRect::setAutoScale( bool yn )
{
    for ( int idx=0; idx<textureset.size(); idx++ )
	textureset[idx]->setAutoScale( yn );
}


bool visBase::TextureRect::autoScale() const
{
    return textureset[curidx]->autoScale();
}


void visBase::TextureRect::useTexture( bool yn )
{
    for ( int idx=0; idx<textureset.size(); idx++ )
	textureset[idx]->turnOn( yn );
}


bool visBase::TextureRect::usesTexture() const
{
    return textureset[curidx]->isOn();
}


void visBase::TextureRect::clear()
{
    while ( textureset.size() > 1 )
    {
	const int idx = textureset.size()-1;
	visBase::Texture2* text = textureset[idx];
	textureswitch->removeChild( text->getData() );
	text->unRef();
	textureset.remove( idx );
    }
}


void visBase::TextureRect::setData( const Array2D<float>* data, int idx, 
				    int datatype )
{
    visBase::Texture2* text;
    if ( !idx )
    {
	text = textureset[idx];
	if ( !text )
	    text = visBase::Texture2::create();
    }
    else
    {
	text = visBase::Texture2::create();
	setTexture( *text, idx );
    }

    text->setData( data, (Texture::DataType)datatype );
}


void visBase::TextureRect::showTexture( int idx )
{
    textureswitch->whichChild = idx < 0 ? SO_SWITCH_NONE : idx;
    curidx = idx;
}


void visBase::TextureRect::setTextureQuality( float q )
{
    for ( int idx=0; idx<textureset.size(); idx++ )
	textureset[idx]->setTextureQuality( q );
}


float visBase::TextureRect::getTextureQuality() const
{
    return textureset[curidx]->getTextureQuality();
}


int visBase::TextureRect::getNrResolutions() const
{
    return 3;
}


void visBase::TextureRect::setResolution( int res )
{
    for ( int idx=0; idx<textureset.size(); idx++ )
	textureset[idx]->setResolution( res );
}


int visBase::TextureRect::getResolution() const
{
    return textureset[curidx]->getResolution();
}


const TypeSet<float>& visBase::TextureRect::getHistogram() const
{
    return textureset[curidx]->getHistogram();
}


void visBase::TextureRect::setColorPars( bool rev, bool useclip, 
					 const Interval<float>& intv )
{
    VisColTabMod& ctm = textureset[curidx]->getColTabMod();
    ctm.doReverse( rev );
    ctm.useClipping( useclip );
    useclip ? ctm.setClipRate( intv.start, intv.stop ) : ctm.setRange( intv );
}


const Interval<float>& visBase::TextureRect::getColorDataRange() const
{
    return textureset[curidx]->getColTabMod().getRange();
}


void visBase::TextureRect::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    int rectid = rectangle->id();
    par.set( rectangleidstr, rectid );

    int textureid = textureset[0]->id();
    par.set( textureidstr, textureid );

    if ( saveids.indexOf(rectid) == -1 ) saveids += rectid;
    if ( saveids.indexOf(textureid) == -1 ) saveids += textureid;
}


int visBase::TextureRect::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    res = useOldPar( par );
    if ( res < 1 ) return res;

    if ( res == 2 )
    {
	int textureid;
	if ( !par.get( textureidstr, textureid ) ) return -1;
	DataObject* dataobj = DM().getObj( textureid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(Texture2*,texture_,dataobj);
	if ( !texture_ ) return -1;
	setTexture( *texture_, 0 );
    }

    int rectid;
    if ( !par.get( rectangleidstr, rectid ) ) return -1;
    DataObject* dataobj = DM().getObj( rectid );
    if ( !dataobj ) return 0;
    mDynamicCastGet( Rectangle*, rect, dataobj );
    if ( !rect ) return -1;

    setRectangle( rect );

    return 1;
}


int visBase::TextureRect::useOldPar( const IOPar& par )
{ 
    // Will be used in d-Tect 1.5 to be able to restore old sessions
    int coltabid;
    if ( !par.get( "ColorTable ID", coltabid ) ) return 2;
    // use new par;
    
    DataObject* dataobj = DM().getObj( coltabid );
    if ( !dataobj ) return 0;
    mDynamicCastGet( VisColorTab*, coltab, dataobj );
    if ( !coltab ) return -1;

    textureset[0]->setColorTab( *coltab );

    float cliprt = 0.025;
    if ( par.get( "Cliprate", cliprt ) )
	setClipRate( cliprt );

    bool autosc = true;
    if ( par.getYN( "Auto scale", autosc ) )
        setAutoScale( autosc );

    int newres = 0;
    par.get( "Resolution", newres );
    setResolution( newres );

    float texturequality = 1;
    par.get( "Texture quality", texturequality );
    setTextureQuality( texturequality );

    bool usetext = true;
    par.getYN( "Uses texture", usetext );
    useTexture( usetext );

    return 1;
}
