/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: viswell.cc,v 1.17 2004-05-24 16:37:40 bert Exp $
________________________________________________________________________

-*/

#include "viswell.h"
#include "vispolyline.h"
#include "visdrawstyle.h"
#include "vistext.h"
#include "visdatagroup.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "vismarker.h"
#include "survinfo.h"
#include "iopar.h"
#include "ranges.h"

#include "SoPlaneWellLog.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoSwitch.h>


mCreateFactoryEntry( visBase::Well );

static const int sMaxNrLogSamples = 2000;
static const int sDefaultMarkerSize = 10;

namespace visBase 
{

const char* Well::linestylestr	= "Line style";
const char* Well::showwellnmstr	= "Show name";
const char* Well::showmarkerstr = "Show markers";
const char* Well::showmarknmstr	= "Show markername";
const char* Well::markerszstr	= "Marker size";
const char* Well::showlogsstr	= "Show logs";
const char* Well::showlognmstr	= "Show logname";


Well::Well()
    : track( visBase::PolyLine::create() )
    , drawstyle( visBase::DrawStyle::create() )
    , welltxt( visBase::Text::create() )
    , markernmsw( new SoSwitch )
    , showmarkers(true)
    , markersize(sDefaultMarkerSize)
    , log( new SoPlaneWellLog )
    , transformation(0)
{
    SoSeparator* sep = new SoSeparator;
    addChild( sep );
    drawstyle->ref();
    sep->addChild( drawstyle->getInventorNode() );
    track->ref();
    track->setMaterial( visBase::Material::create() );
    sep->addChild( track->getInventorNode() );
    welltxt->ref();
    sep->addChild( welltxt->getInventorNode() );

    markergroup = visBase::DataObjectGroup::create();
    markergroup->ref();
    addChild( markergroup->getInventorNode() );
    addChild( markernmsw );
    markernames = visBase::DataObjectGroup::create();
    markernames->setSeparate(false);
    markernames->ref();
    markernmsw->addChild( markernames->getInventorNode() );
    markernmsw->whichChild = 0;

    addChild( log );
}


Well::~Well()
{
    if ( transformation ) transformation->unRef();
    removeChild( welltxt->getInventorNode() );
    welltxt->unRef();
    removeChild( track->getInventorNode() );
    track->unRef();
    removeChild( drawstyle->getInventorNode() );
    drawstyle->unRef();
    markergroup->removeAll();
    removeChild( markergroup->getInventorNode() );
    markergroup->unRef();
    markernames->removeAll();
    removeChild( markernames->getInventorNode() );
    markernames->unRef();
}


void Well::setTrack( const TypeSet<Coord3>& pts )
{
    while ( track->size()>pts.size() )
	track->removePoint( 0 );

    track->setTransformation( transformation );
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	const Coord3& crd = pts[idx];
	if ( idx>=track->size() )
	    track->addPoint( crd );
	else
	    track->setPoint( idx, crd );
    }
}


void Well::setLineStyle( const LineStyle& lst )
{
    track->getMaterial()->setColor( lst.color );
    drawstyle->setLineStyle( lst ); 
}


const LineStyle& Well::lineStyle() const
{
    static LineStyle ls;
    ls.type = drawstyle->lineStyle().type;
    ls.width = drawstyle->lineStyle().width;
    ls.color = track->getMaterial()->getColor();
    return ls;
}


void Well::setWellName( const char* nm, const Coord3& pos )
{
    welltxt->setTransformation( transformation );
    welltxt->setText( nm );
    Coord3 wp( pos );
    if ( !SI().zRange().includes(pos.z) )
	wp.z = SI().zRange().limitValue( pos.z );
    welltxt->setPosition( wp ); //TODO
    welltxt->setJustification( visBase::Text::Center );
}

void Well::showWellName( bool yn )
{ welltxt->turnOn( yn ); }

bool Well::wellNameShown() const
{ return welltxt->isOn(); }


void Well::addMarker( const Coord3& pos, const Color& color, const char* nm ) 
{
    visBase::Marker* marker = visBase::Marker::create();

    SoSeparator* markershapesep = new SoSeparator;
    markershapesep->ref();
    SoCoordinate3* markercoords= new SoCoordinate3;
    markershapesep->addChild(markercoords);
    markercoords->point.set1Value(0,-1,-1,0);
    markercoords->point.set1Value(1,-1, 1,0);
    markercoords->point.set1Value(2, 1, 1,0);
    markercoords->point.set1Value(3, 1,-1,0);
    SoFaceSet* markershape = new SoFaceSet;
    markershape->numVertices.setValue(4);
    markershapesep->addChild(markershape);

    marker->setMarkerShape(markershapesep);
    markershapesep->unref();

    marker->doRestoreProportions(false);
    markergroup->addObject( marker );
    marker->setTransformation( transformation );
    marker->setCenterPos( pos );
    //marker->setType( MarkerStyle3D::Cube );
    marker->getMaterial()->setColor( color );
    marker->setScreenSize( markersize );
    marker->turnOn( showmarkers );

    Text* markernm = Text::create();
    markernm->setTransformation( transformation );
    markernm->setText( nm );
    markernm->setPosition( pos );
    markernm->setJustification( visBase::Text::Left );
    markernames->addObject( markernm );
}


void Well::removeAllMarkers() 
{
    markergroup->removeAll();
    markernames->removeAll();
}


void Well::setMarkerScreenSize( int size )
{
    markersize = size;
    for ( int idx=0; idx<markergroup->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(idx))
	marker->setScreenSize( size );
    }
}


int Well::markerScreenSize() const
{
    mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(0))
    return marker ? mNINT(marker->getScreenSize()) : sDefaultMarkerSize;
}


bool Well::canShowMarkers() const
{ return markergroup->size(); }


void Well::showMarkers( bool yn )
{
    showmarkers = yn;
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
{ markernmsw->whichChild = yn ? 0 : SO_SWITCH_NONE; }


bool Well::markerNameShown() const
{ return markernmsw->whichChild.getValue()==0; }


void Well::setLogData( const TypeSet<Coord3Value>& crdvals, const char* lognm,
		       const Interval<float>& range, bool sclog, int lognr )
{
    int nrsamp = crdvals.size();

    float step = 1;
    if ( nrsamp > sMaxNrLogSamples )
    {
	step = (float)nrsamp / sMaxNrLogSamples;
	nrsamp = sMaxNrLogSamples;
    }

    const bool rev = range.start > range.stop;
    log->setRevScale( rev );
    log->clearLog( lognr );
    Interval<float> rg = range; rg.sort();
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int index = mNINT(idx*step);
	const Coord3Value& cv = crdvals[index];
	Coord3 pos( cv.coord );
	if ( transformation )
	    pos = transformation->transform( pos );
	if ( mIsUndefined(pos.z) ) continue;

	float val = mIsUndefined(cv.value) ? 0 : cv.value;
	val -= rg.start;
	if ( val < 0 ) val = 0;
	else if ( !val > rg.stop )
	    val = rg.stop;
	if ( sclog )
	{
	    val += 1;
	    val = ::log( val );
	}
	log->setLogValue( idx, SbVec3f(pos.x,pos.y,pos.z), val, lognr );
    }

    showLogs( true );
}


void Well::setLogColor( const Color& col, int lognr )
{
    log->setLineColor( SbVec3f(col.r()/255,col.g()/255,col.b()/255), lognr );
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


void Well::setLogLineWidth( float width, int lognr )
{
    log->setLineWidth( width, lognr );
}


float Well::logLineWidth( int lognr ) const
{
    return log->lineWidth( lognr );
}


void Well::setLogWidth( int width )
{
    log->screenWidth.setValue( (float)width );
}


int Well::logWidth() const
{
    return mNINT(log->screenWidth.getValue());
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
{} 

bool Well::logNameShown() const
{ return false; }


void Well::setTransformation( visBase::Transformation* nt )
{
    if ( transformation )
	transformation->unRef();
    transformation = nt;
    if ( transformation )
	transformation->ref();
}


visBase::Transformation* Well::getTransformation()
{ return transformation; }


void Well::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    BufferString linestyle;
    lineStyle().toString( linestyle );
    par.set( linestylestr, linestyle );

    par.setYN( showwellnmstr, welltxt->isOn() );
    par.setYN( showmarkerstr, markersShown() );
    par.setYN( showmarknmstr, markerNameShown() );
    par.setYN( showlogsstr, logsShown() );
    par.setYN( showlognmstr, logNameShown() );
    par.set( markerszstr, markersize );
}


int Well::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    BufferString linestyle;
    if ( par.get(linestylestr,linestyle) )
    {
	LineStyle lst;
	lst.fromString( linestyle );
	setLineStyle( lst );
    }

#define mParGetYN(str,func) \
    doshow = true; \
    par.getYN(str,doshow); \
    func( doshow );

    bool doshow;
    mParGetYN(showwellnmstr,showWellName);
    mParGetYN(showmarkerstr,showMarkers);	showmarkers = doshow;
    mParGetYN(showmarknmstr,showMarkerName);
    mParGetYN(showlogsstr,showLogs);
    mParGetYN(showlognmstr,showLogName);

    par.get( markerszstr, markersize );
    setMarkerScreenSize( markersize );

    return 1;
}

}; // namespace visBase
