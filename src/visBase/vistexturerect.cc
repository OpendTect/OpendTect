/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vistexturerect.cc,v 1.17 2002-04-29 09:03:33 kristofer Exp $";

#include "vistexturerect.h"
#include "visrectangle.h"
#include "arrayndimpl.h"
#include "dataclipper.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "ptrman.h"
#include "position.h"

#include <math.h>

#include "Inventor/nodes/SoTexture2.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoComplexity.h"
#include "Inventor/nodes/SoMaterial.h"

mCreateFactoryEntry( visBase::TextureRect );


visBase::TextureRect::TextureRect()
    : texture( new SoTexture2 )
    , quality( new SoComplexity )
    , textureswitch( new SoSwitch )
    , texturegrp( new SoGroup )
    , autoscale( true )
    , cliprate( 0.025 )
    , data( 0 )
    , rectangle( 0 )
    , colortable( 0 )
    , manipstartnotifier( this )
    , manipchnotifier( this )
    , manipendsnotifier( this )
{
    addChild( textureswitch );
    textureswitch->addChild( texturegrp );

    texturegrp->insertChild( quality, 0 );
    quality->textureQuality.setValue( 1 );

    texturegrp->addChild( texture );
    texture->wrapS = SoTexture2::CLAMP;
    texture->wrapT = SoTexture2::CLAMP;
    texture->model = SoTexture2::MODULATE;

    useTexture( true );

    setRectangle( visBase::Rectangle::create() );
    setColorTab( visBase::VisColorTab::create() );
}


visBase::TextureRect::~TextureRect()
{
    rectangle->unRef();
    colortable->unRef();
    delete data;
}


float visBase::TextureRect::getValue( const Geometry::Pos& pos ) const
{
    if ( !data ) return mUndefValue;

    visBase::Rectangle::Orientation orientation = rectangle->orientation();
    Geometry::Pos origo = rectangle->origo();
    Coord localpos;		

    if ( orientation==visBase::Rectangle::XY )
    {
	if ( fabs(pos.z-origo.z)> 1e-3 )
	    return mUndefValue;
	// x=inl y=crl
	localpos.x = pos.x-origo.x;
	localpos.y = pos.y-origo.y;
	localpos.x /= rectangle->width(0);
	localpos.y /= rectangle->width(1);

    }
    else if ( orientation==visBase::Rectangle::XZ )
    {
	if ( fabs(pos.y-origo.y)> 1e-3 )
	    return mUndefValue;
	// x=inline y=depth
	localpos.x = pos.x-origo.x;
	localpos.y = pos.z-origo.z;

	localpos.x /= rectangle->width(0);
	localpos.y /= rectangle->width(1);
    }
    else 
    {
	if ( fabs(pos.x-origo.x)> 1e-3 )
	    return mUndefValue;
	// x=crossline y=depth
	localpos.x = pos.y-origo.y;
	localpos.y = pos.z-origo.z;

	localpos.x /= rectangle->width(1);
	localpos.y /= rectangle->width(0);
    }

    if ( localpos.x>1 || localpos.y>1 )
	return mUndefValue;

    localpos.x *= (data->info().getSize(0)-1);
    localpos.y *= (data->info().getSize(1)-1);

    return data->get(mNINT(localpos.x), mNINT(localpos.y));
}


void visBase::TextureRect::setRectangle( Rectangle* nr )
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

	removeChild( rectangle->getData() );
	rectangle->unRef();
    }

    rectangle = nr;
    rectangle->ref();
    addChild( rectangle->getData() );
    rectangle->setMaterial( 0 );

    rectangle->manipStarts()->notify( mCB( this,TextureRect, triggerManipStarts));
    rectangle->manipChanges()->notify( mCB(this,TextureRect,triggerManipChanges));
    rectangle->manipEnds()->notify( mCB( this, TextureRect, triggerManipEnds ));
    rectangle->selection()->notify( mCB( this, TextureRect, triggerSel ));
    rectangle->deSelection()->notify( mCB( this, TextureRect, triggerDeSel ));
}


const visBase::Rectangle& visBase::TextureRect::getRectangle() const
{ return *rectangle; }
    
    
visBase::Rectangle& visBase::TextureRect::getRectangle()
{ return *rectangle; }
    
    
void visBase::TextureRect::setColorTab( VisColorTab* nr )
{
    if ( colortable )
    {
	colortable->change.remove(
				mCB( this,visBase::TextureRect, updateTexture));
	colortable->unRef();
    }

    colortable = nr;
    colortable->ref();
    colortable->change.notify(mCB( this,visBase::TextureRect, updateTexture));


    updateTexture();
}


const visBase::VisColorTab& visBase::TextureRect::getColorTab() const
{ return *colortable; }
    
    
visBase::VisColorTab& visBase::TextureRect::getColorTab()
{ return *colortable; }
    

void visBase::TextureRect::setClipRate(float n )
{
    cliprate = n;
    clipData();
    autoscale = true;
}


float visBase::TextureRect::clipRate() const { return cliprate; }


void visBase::TextureRect::useTexture( bool n )
{
    textureswitch->whichChild = n ? 0 : SO_SWITCH_NONE;
}


bool visBase::TextureRect::usesTexture() const
{
    return textureswitch->whichChild.getValue() == 0;
}


void visBase::TextureRect::setData( const Array2D<float>& d )
{
    delete data;
    data = new Array2DImpl<float>(d);

    if ( autoscale )
    {
	// This will trigger the CB to updateTexture so we can return
	clipData();
	return;
    }

    updateTexture();
}


void visBase::TextureRect::clipData()
{
    if ( !data ) return;
    int nrvalues = data->info().getTotalSz();

    DataClipper clipper( cliprate );
    clipper.setApproxNrValues( nrvalues, 10000 );
    clipper.putData( data->getData(), nrvalues );
    clipper.calculateRange();

    colortable->scaleTo( clipper.getRange() );
}


void visBase::TextureRect::updateTexture()
{
    if ( !data ) return;

    bool isinl = getRectangle().orientation() == visBase::Rectangle::YZ;
    int ssize = data->info().getSize( isinl ? 0 : 1 );
    int tsize = data->info().getSize( isinl ? 1 : 0 );

    const int nrofcomponents = 3;
    // set to 4 if transperancy should be used

    ArrPtrMan<unsigned char> imagedata =
			new unsigned char[ssize*tsize*nrofcomponents];

    int idx=0;
    float val;
    Color color;
    for ( int t=0; t<tsize; t++ )
    {
	for ( int s=0; s<ssize; s++ )
	{
	    val = isinl ? data->get( s, t ) : data->get( t, s );
	    color = colortable->color(val);
	    imagedata[idx++] = color.r();
	    imagedata[idx++] = color.g();
	    imagedata[idx++] = color.b();
	    if ( nrofcomponents==4 )
	    	imagedata[idx++] = 255 - color.t();
	}
    }
    
    texture->image.setValue( SbVec2s(ssize, tsize ),
			     nrofcomponents, imagedata );
}


void visBase::TextureRect::setTextureQuality( float q )
{
    quality->textureQuality.setValue( q );
}
