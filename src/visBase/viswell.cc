/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: viswell.cc,v 1.3 2003-10-22 15:26:59 nanne Exp $
________________________________________________________________________

-*/

#include "viswell.h"
#include "vispolyline.h"
#include "visdrawstyle.h"
#include "vistext.h"
#include "vissceneobjgroup.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "iopar.h"

#include "SoPlaneWellLog.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>


mCreateFactoryEntry( visBase::Well );

static const int sMaxNrLogSamples = 2000;
static const int sDefaultMarkerSize = 10;

namespace visBase 
{

const char* Well::linestylestr	= "Line style";
const char* Well::showwellnmstr	= "Show name";
const char* Well::showmarkerstr = "Show markers";


Well::Well()
    : track( visBase::PolyLine::create() )
    , drawstyle( visBase::DrawStyle::create() )
    , welltxt( visBase::Text::create() )
    , markergroup( visBase::SceneObjectGroup::create() )
    , marktxtsw( new SoSwitch )
    , log( new SoPlaneWellLog )
{
    SoSeparator* sep = new SoSeparator;
    addChild( sep );
    drawstyle->ref();
    sep->addChild( drawstyle->getData() );
    track->ref();
    track->setMaterial( visBase::Material::create() );
    sep->addChild( track->getData() );
    welltxt->ref();
    sep->addChild( welltxt->getData() );

    markergroup->ref();
    addChild( markergroup->getData() );
    addChild( marktxtsw );
    marktexts = visBase::SceneObjectGroup::create();
    marktexts->setSeparate(false);
    marktexts->ref();
    marktxtsw->addChild( marktexts->getData() );
    marktxtsw->whichChild = 0;

    addChild( log );
}


Well::~Well()
{
    removeChild( welltxt->getData() );
    welltxt->unRef();
    removeChild( track->getData() );
    track->unRef();
    removeChild( drawstyle->getData() );
    drawstyle->unRef();
    removeChild( markergroup->getData() );
    markergroup->unRef();
    marktexts->unRef();
}


void Well::setTrack( const TypeSet<Coord3>& pts )
{
    while ( track->size() )
	track->removePoint( 0 );

    for ( int idx=0; idx<pts.size(); idx++ )
    {
	const Coord3& crd = pts[idx];
	track->addPoint( crd );
    }
}


void Well::setLineStyle( const LineStyle& lst )
{
    track->getMaterial()->setColor( lst.color );
    drawstyle->setLineStyle( lst ); 
}


const LineStyle& Well::lineStyle() const
{
    const LineStyle& curls = drawstyle->lineStyle();
    static LineStyle ls( curls.type, curls.width, 
	    		 track->getMaterial()->getColor() );
    return ls;
}


void Well::setWellName( const char* nm, const Coord3& pos )
{
    welltxt->setText( nm );
    welltxt->setPosition( pos ); //TODO
    welltxt->setJustification( visBase::Text::Center );
}

void Well::showWellName( bool yn )
{ welltxt->turnOn( yn ); }

bool Well::wellNameShown() const
{ return welltxt->isOn(); }


void Well::addMarker( const Coord3& pos, const Color& color, const char* nm ) 
{
    visBase::Marker* marker = visBase::Marker::create();
    markergroup->addObject( marker );
    marker->setCenterPos( pos );
    marker->setType( visBase::Marker::Cube );
    marker->getMaterial()->setColor( color );
    marker->setSize( sDefaultMarkerSize );

    Text* markername = Text::create();
    markername->setText( nm );
    markername->setPosition( pos );
    markername->setJustification( visBase::Text::Left );
    marktexts->addObject( markername );
}


void Well::removeAllMarkers() 
{
    markergroup->removeAll();
}


void Well::setMarkerScale( const Coord3& scale )
{
    for ( int idx=0; idx<markergroup->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(idx))
	marker->setScale( scale );
    }
}


void Well::setMarkerSize( int size )
{
    for ( int idx=0; idx<markergroup->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(idx))
	marker->setSize( size );
    }
}


int Well::markerSize() const
{
    mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(0))
    return marker ? mNINT(marker->getSize()) : sDefaultMarkerSize;
}


void Well::showMarkers( bool yn )
{
    for ( int idx=0; idx<markergroup->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(idx))
	marker->turnOn( yn );
    }
}


bool Well::markersShown() const
{
    mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(0))
    return marker && marker->isOn();
}


void Well::showMarkerName( bool yn )
{ marktxtsw->whichChild = yn ? 0 : SO_SWITCH_NONE; }


bool Well::markerNameShown() const
{ return marktxtsw->whichChild.getValue()==0; }


void Well::setLog( const TypeSet<Coord3>& coords, const TypeSet<float>& vals,
       		   int lognr )
{
    int nrsamp = coords.size();
    if ( nrsamp != vals.size() ) return;

    float step = 1;
    if ( nrsamp > sMaxNrLogSamples )
    {
	step = (float)nrsamp / sMaxNrLogSamples;
	nrsamp = sMaxNrLogSamples;
    }

    log->clearLog( lognr );
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int index = mNINT(idx*step);
	Coord3 pos(coords[index]);
	if ( getTransformation() )
	    pos = getTransformation()->transform( pos );
	float val = mIsUndefined(vals[index]) ? 0 : vals[index];
	log->setLogValue( idx, SbVec3f(pos.x,pos.y,pos.z), val, lognr );
    }

    showLogs( true );
}


const Color& Well::logColor( int lognr ) const
{
    static Color color;
    const SbVec3f& col = log->lineColor( lognr );
    const int r = mNINT(col[0]*255);
    const int g = mNINT(col[1]*255);
    const int b = mNINT(col[2]*255);
    color.set( (unsigned char)r, (unsigned char)g, (unsigned char)b );
    return color;
}


void Well::setLogColor( const Color& col, int lognr )
{
    log->setLineColor( SbVec3f(col.r()/255,col.g()/255,col.b()/255), lognr );
}


void Well::showLogs( bool yn )
{
    log->showLog( yn, 1 );
    log->showLog( yn, 2 );
}


bool Well::logsShown() const
{
    return log->logShown( 1 );
}


void Well::showLogName( bool yn )
{
}


bool Well::logNameShown() const
{
    return false;
}


void Well::setTransformation( visBase::Transformation* nt )
{
    track->setTransformation( nt );
    welltxt->setTransformation( nt );
    for ( int idx=0; idx<markergroup->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(idx))
	marker->setTransformation( nt );
    }

    for ( int idx=0; idx<marktexts->size(); idx++ )
	((visBase::Text*)(marktexts->getObject(idx)))->setTransformation( nt );
}


visBase::Transformation* Well::getTransformation()
{ return track->getTransformation(); }


void Well::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    BufferString linestyle;
    drawstyle->lineStyle().toString( linestyle );
    par.set( linestylestr, linestyle );

    bool welltxtshown = welltxt->isOn();
    par.setYN( showwellnmstr, welltxtshown );

    bool markershown = markersShown();
    par.setYN( showmarkerstr, markershown );
}


int Well::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    BufferString linestyle;
    if ( par.get( linestylestr, linestyle ) )
    {
	LineStyle lst;
	lst.fromString( linestyle );
	drawstyle->setLineStyle( lst );
    }

    bool welltxtshown = true;
    par.getYN( showwellnmstr, welltxtshown );
    showWellName( welltxtshown );

    bool markershown = true;
    par.getYN( showmarkerstr, markershown );
    showMarkers( markershown );

    return 1;
}

}; // namespace visBase
