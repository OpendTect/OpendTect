/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vistexturerect.cc,v 1.25 2003-01-02 14:22:18 kristofer Exp $";

#include "vistexturerect.h"
#include "iopar.h"
#include "visrectangle.h"
#include "arrayndimpl.h"
#include "dataclipper.h"
#include "visdataman.h"
#include "viscolortab.h"
#include "ptrman.h"
#include "position.h"
#include "simpnumer.h"

#include <math.h>

#include "Inventor/nodes/SoTexture2.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoComplexity.h"
#include "Inventor/nodes/SoMaterial.h"

mCreateFactoryEntry( visBase::TextureRect );


const char* visBase::TextureRect::texturequalitystr = "Texture quality";
const char* visBase::TextureRect::rectangleidstr = "Rectangle ID";
const char* visBase::TextureRect::colortabidstr = "ColorTable ID";
const char* visBase::TextureRect::usestexturestr = "Uses texture";
const char* visBase::TextureRect::clipratestr = "Cliprate";
const char* visBase::TextureRect::autoscalestr = "Auto scale";
const char* visBase::TextureRect::resolutionstr = "Resolution";

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
    , resolution(0)
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

    if ( colortable )
    {
	colortable->sequencechange.remove(
				mCB( this,visBase::TextureRect, updateTexture));
	colortable->rangechange.remove(
				mCB( this,visBase::TextureRect, updateTexture));
	colortable->unRef();
    }

    delete data;
}


float visBase::TextureRect::getValue( const Coord3& pos ) const
{
    if ( !data ) return mUndefValue;

    visBase::Rectangle::Orientation orientation = rectangle->orientation();
    Coord3 origo = rectangle->origo();
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
	colortable->rangechange.remove(
				mCB( this,visBase::TextureRect, updateTexture));
	colortable->sequencechange.remove(
				mCB( this,visBase::TextureRect, updateTexture));
	colortable->unRef();
    }

    colortable = nr;
    colortable->ref();
    colortable->rangechange.notify(
	    		mCB( this,visBase::TextureRect, updateTexture));
    colortable->sequencechange.notify(
			mCB( this,visBase::TextureRect, updateTexture));


    updateTexture(0);
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


void visBase::TextureRect::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    int rectid = rectangle->id();
    int ctid = colortable->id();

    par.set( texturequalitystr, quality->textureQuality.getValue() );
    par.set( rectangleidstr, rectid );
    par.set( colortabidstr, ctid );
    par.setYN( usestexturestr, usesTexture() );
    par.set( clipratestr, clipRate() );
    par.setYN( autoscalestr, autoScale() );
    par.set( resolutionstr, resolution );

    if ( saveids.indexOf( rectid )==-1 ) saveids += rectid;
    if ( saveids.indexOf( ctid )==-1 ) saveids += ctid;
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

    int coltabid;
    if ( !par.get( colortabidstr, coltabid ) ) return -1;
    dataobj = DM().getObj( coltabid );
    if ( !dataobj ) return 0;
    mDynamicCastGet( VisColorTab*, coltab, dataobj );
    if ( !coltab ) return -1;

    setColorTab( coltab );

    bool usetext;
    if ( !par.getYN( usestexturestr, usetext )) return -1;
    useTexture(usetext);

    float cliprt;
    if ( par.get( clipratestr, cliprt ) )
	setClipRate( cliprt );

    bool autosc;
    if ( par.getYN( autoscalestr, autosc ) )
	setAutoscale( autosc );

    return 1;
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

    updateTexture(0);
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


void visBase::TextureRect::updateTexture( CallBacker* )
{
    if ( !data ) return;

    bool isinl = getRectangle().orientation() == visBase::Rectangle::YZ;
    int ssize = data->info().getSize( isinl ? 0 : 1 );
    int tsize = data->info().getSize( isinl ? 1 : 0 );

//  const int nrofcomponents = 3;	// disable transparency
    const int nrofcomponents = 4;	// use transparency

    int newssize = !resolution ? ssize : ( resolution == 1 ? 1024 : 2048 );
    int newtsize = !resolution ? tsize : ( resolution == 1 ? 1024 : 2048 );
    ArrPtrMan<unsigned char> imagedata =
			new unsigned char[newssize*newtsize*nrofcomponents];

    int idx=0;
    float val00, val01, val10, val11;
    Color color;
    float sstep = (ssize-1) / (float)(newssize-1);
    float tstep = (tsize-1) / (float)(newtsize-1);
    for ( int t=0; t<newtsize; t++ )
    {
	for ( int s=0; s<newssize; s++ )
	{
	    float spos = s * sstep;
	    int snr = (int)spos;
	    bool onsedge = snr+1 == ssize;

	    float tpos = t * tstep;
	    int tnr = (int)tpos;
	    bool ontedge = tnr+1 == tsize;
	    spos -= snr;
	    tpos -= tnr;
	    val00 = isinl ? data->get( snr, tnr ) : data->get( tnr, snr );
	    if ( !onsedge )
		val10 = isinl ? data->get( snr+1, tnr ) 
			      : data->get( tnr, snr+1 );
	    if ( !ontedge )
		val01 = isinl ? data->get( snr, tnr+1 ) 
			      : data->get( tnr+1, snr );
	    if ( !onsedge && !ontedge )
		val11 = isinl ? data->get( snr+1, tnr+1 ) 
			      : data->get( tnr+1, snr+1 );
	    float val = 0;
	    if ( onsedge && ontedge )
		val = val11;
	    else if ( onsedge )
	    	val = linearInterpolate( val00, val01, tpos );
	    else if ( ontedge )
		val = linearInterpolate( val00, val10, spos );
	    else
		val = linearInterpolate2D(val00,val01,val10,val11,spos,tpos);

	    color = colortable->color(val);
	    imagedata[idx++] = color.r();
	    imagedata[idx++] = color.g();
	    imagedata[idx++] = color.b();
	    if ( nrofcomponents==4 )
	    	imagedata[idx++] = 255 - color.t();
	}
    }
    
    texture->image.setValue( SbVec2s(newssize, newtsize ),
			     nrofcomponents, imagedata );
}


void visBase::TextureRect::setTextureQuality( float q )
{
    quality->textureQuality.setValue( q );
}


int visBase::TextureRect::getNrResolutions() const
{
    return 3;
}


void visBase::TextureRect::setResolution( int res )
{
    resolution = res;
    updateTexture(0);
}
