/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vistexturerect.cc,v 1.5 2002-04-05 15:14:59 nanne Exp $";

#include "vistexturerect.h"
#include "visrectangle.h"
#include "arrayndimpl.h"
#include "dataclipper.h"
#include "visdataman.h"
#include "viscolortab.h"

#include "Inventor/nodes/SoTexture2.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoComplexity.h"
#include "Inventor/nodes/SoMaterial.h"


visBase::TextureRect::TextureRect( bool usermanip )
    : texture( new SoTexture2 )
    , quality( new SoComplexity )
    , textureswitch( new SoSwitch )
    , texturegrp( new SoGroup )
    , modcolor( new SoMaterial )
    , autoscale( true )
    , cliprate( 0.025 )
    , data( 0 )
    , rectangle( 0 )
    , colortable( 0 )
    , manipstartnotifier( this )
    , manipchnotifier( this )
    , manipendsnotifier( this )
    , selnotifier( this )
    , deselnotifier( this )
{
    addChild( textureswitch );
    textureswitch->addChild( texturegrp );

    texturegrp->addChild( modcolor );
    modcolor->ambientColor.setValue( 1, 1, 1 );
    modcolor->diffuseColor.setValue( 1, 1, 1 );

    texturegrp->insertChild( quality, 0 );
    quality->textureQuality.setValue( 1 );

    texturegrp->addChild( texture );
    texture->wrapS = SoTexture2::CLAMP;
    texture->wrapT = SoTexture2::CLAMP;
    texture->model = SoTexture2::MODULATE;

    useTexture( true );

    setRectangle( visBase::Rectangle::create( usermanip ) );
    setColorTab( visBase::VisColorTab::create() );
}


visBase::TextureRect::~TextureRect()
{
    rectangle->unRef();
    colortable->unRef();
    delete data;
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
	visBase::DataManager::manager.unRef( rectangle );
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
	colortable->change.remove(mCB( this,visBase::TextureRect, updateTexture));
	visBase::DataManager::manager.unRef( colortable );
    }

    colortable = nr;
    visBase::DataManager::manager.ref( colortable );
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
    int nrvalues = data->info().getTotalSz();

    DataClipper clipper( cliprate );

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

    unsigned char imagedata[ssize*tsize*4];

    int idx=0;
    
    for ( int t=0; t<tsize; t++ )
    {
	for ( int s=0; s<ssize; s++ )
	{
	    float val = isinl ? data->get( s, t ) : data->get( t, s );
	    const Color color = colortable->color(val);
	    imagedata[idx++] = color.r();
	    imagedata[idx++] = color.g();
	    imagedata[idx++] = color.b();
	    imagedata[idx++] = 255 - color.t();
	}
    }
    
	
    texture->image.setValue( SbVec2s(ssize, tsize ),
			     4, imagedata );
}


void visBase::TextureRect::setTextureQuality( float q )
{
    quality->textureQuality.setValue( q );
}
