/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: viswelldisplay.cc,v 1.17 2003-09-11 15:07:24 nanne Exp $";

#include "vissurvwell.h"
#include "vispolyline.h"
#include "visdrawstyle.h"
#include "vistext.h"
#include "vissceneobjgroup.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "wellman.h"
#include "welldata.h"
#include "welltransl.h"
#include "welltrack.h"
#include "wellmarker.h"
#include "welld2tmodel.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "executor.h"
#include "ptrman.h"
#include "survinfo.h"


mCreateFactoryEntry( visSurvey::WellDisplay );

namespace visSurvey
{

const char* WellDisplay::earthmodelidstr 	= "EarthModel ID";
const char* WellDisplay::ioobjidstr		= "IOObj ID";
const char* WellDisplay::linestylestr 		= "Line style";
const char* WellDisplay::showwellnmstr 		= "Show name";


WellDisplay::WellDisplay()
    : line( visBase::PolyLine::create() )
    , drawstyle( visBase::DrawStyle::create() )
    , welltxt( visBase::Text::create() )
    , markergroup( visBase::SceneObjectGroup::create() )
    , wellid( -1 )
{
    drawstyle->ref();
    addChild( drawstyle->getData() );
    line->ref();
    addChild( line->getData() );
    line->setMaterial( 0 );
    welltxt->ref();
    addChild( welltxt->getData() );
    markergroup->ref();
    addChild( markergroup->getData() );
}


WellDisplay::~WellDisplay()
{
    removeChild( welltxt->getData() );
    welltxt->unRef();
    removeChild( line->getData() );
    line->unRef();
    removeChild( drawstyle->getData() );
    drawstyle->unRef();
    removeChild( markergroup->getData() );
    markergroup->unRef();
}


#define mErrRet(s) { errmsg = s; return false; }

bool WellDisplay::setWellId( const MultiID& multiid )
{
    Well::Data* wd = Well::MGR().get( multiid );
    if ( !wd ) return false;
    
    const Well::D2TModel* d2t = wd->d2TModel();
    const bool zistime = SI().zIsTime();
    if ( zistime && !d2t ) mErrRet( "No depth to time model defined" );

    while ( line->size() ) line->removePoint( 0 );
    wellid = multiid;
    setName( wd->name() );

    if ( wd->track().size() < 1 ) return true;
    Coord3 pt;
    for ( int idx=0; idx<wd->track().size(); idx++ )
    {
	pt = wd->track().pos( idx );
	if ( zistime )
	    pt.z = d2t->getTime( wd->track().dah(idx) );
	if ( !mIsUndefined(pt.z) )
	    line->addPoint( pt );
    }

    welltxt->setText( wd->name() );
    welltxt->setPosition( line->getPoint(0) ); //TODO
    welltxt->setJustification( visBase::Text::Center );

    addMarkers();

    return true;
}


const LineStyle& WellDisplay::lineStyle() const
{
    return drawstyle->lineStyle();
}


void WellDisplay::setLineStyle( const LineStyle& lst )
{
    drawstyle->setLineStyle( lst );
}


void WellDisplay::showWellText( bool yn )
{
    welltxt->turnOn( yn );
}


bool WellDisplay::isWellTextShown() const
{
    return welltxt->isOn();
}


void WellDisplay::addMarkers()
{
// TODO: This is a quick solution for markerdisplay. Better to make a separate
//       marker object.
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd ) return;

    const bool zistime = SI().zIsTime();
    markergroup->removeAll();
    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	visBase::Marker* marker = visBase::Marker::create();
	markergroup->addObject( marker );

	Well::Marker* wellmarker = wd->markers()[idx];
	Coord3 pos = wd->track().getPos( wellmarker->dah );
	if ( zistime )
	    pos.z = wd->d2TModel()->getTime( pos.z );

	marker->setCenterPos( pos );
	marker->setScale( Coord3(4,4,1/SPM().getZScale()) );
	marker->setSize( 3 );
	marker->setType( visBase::Marker::Cube );
	marker->getMaterial()->setColor( wellmarker->color );
    }
}


void WellDisplay::showMarkers( bool yn )
{
    for ( int idx=0; idx<markergroup->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(idx))
	marker->turnOn( yn );
    }
}


bool WellDisplay::markersShown() const
{
    mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(0))
    return marker && marker->isOn();
}


void WellDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( earthmodelidstr, wellid );
    
    BufferString linestyle;
    drawstyle->lineStyle().toString( linestyle );
    par.set( linestylestr, linestyle );

    bool welltxtshown = welltxt->isOn();
    par.setYN( showwellnmstr, welltxtshown );
}


int WellDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    MultiID newwellid;
    if ( !par.get( earthmodelidstr, newwellid ))
	return -1;

    if ( !setWellId( newwellid ) )
	return -1;

    BufferString linestyle;
    if ( par.get( linestylestr, linestyle ) )
    {
	LineStyle lst;
	lst.fromString( linestyle );
	drawstyle->setLineStyle( lst );
    }

    bool welltxtshown = true;
    par.getYN( showwellnmstr, welltxtshown );
    showWellText( welltxtshown );

    return 1;
}


void WellDisplay::setTransformation( visBase::Transformation* nt )
{
    line->setTransformation( nt );
    welltxt->setTransformation( nt );
    for ( int idx=0; idx<markergroup->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,markergroup->getObject(idx))
	marker->setTransformation( nt );
    }
}



visBase::Transformation* WellDisplay::getTransformation()
{ return line->getTransformation(); }

}; // namespace visSurvey
