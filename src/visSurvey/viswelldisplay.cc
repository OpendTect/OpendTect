/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: viswelldisplay.cc,v 1.1 2002-05-16 14:00:12 kristofer Exp $";

#include "vissurvwell.h"
#include "vispolyline.h"
#include "visdrawstyle.h"
#include "emmanager.h"
#include "emwell.h"
#include "iopar.h"


mCreateFactoryEntry( visSurvey::WellDisplay );

const char* visSurvey::WellDisplay::earthmodelidstr = "EarthModel ID";
const char* visSurvey::WellDisplay::displayattribstr = "Attrib";
const char* visSurvey::WellDisplay::colortableidstr = "ColorTable";
const char* visSurvey::WellDisplay::linestylestr = "Line style";


visSurvey::WellDisplay::WellDisplay()
    : line( visBase::PolyLine::create() )
    , drawstyle( visBase::DrawStyle::create() )
    , displayedattrib( -1 )
    , wellid( -1 )
{
    drawstyle->ref();
    addChild( drawstyle->getData() );
    line->ref();
    addChild( line->getData() );
}


visSurvey::WellDisplay::~WellDisplay()
{
    removeChild( line->getData() );
    line->unRef();
    removeChild( drawstyle->getData() );
    drawstyle->unRef();
}


bool visSurvey::WellDisplay::setWellId( int id )
{
    const EarthModel::EMManager& em = EarthModel::EMM();
    mDynamicCastGet( const EarthModel::Well*, well, em.getObject( id ) );
    if ( !well ) return false;

    while ( line->size() ) line->removePoint( 0 );
    displayedattrib = -1;

    wellid = id;

    if ( well->nrKnots() < 1 ) return true;
    for ( int idx=0; idx<well->nrKnots(); idx++ )
	line->addPoint( well->getKnot( idx ) );

    setName( well->name() );

    return true;
}


int  visSurvey::WellDisplay::nrAttribs() const
{
    const EarthModel::EMManager& em = EarthModel::EMM();
    mDynamicCastGet( const EarthModel::Well*, well, em.getObject( wellid ) );
    return well ? well->nrValues() : -1;
}


const char* visSurvey::WellDisplay::getAttribName(int idx) const
{
    const EarthModel::EMManager& em = EarthModel::EMM();
    mDynamicCastGet( const EarthModel::Well*, well, em.getObject( wellid ) );
    return well ? well->valueName(idx) : 0;
}


bool visSurvey::WellDisplay::depthIsT() const
{
    const EarthModel::EMManager& em = EarthModel::EMM();
    mDynamicCastGet( const EarthModel::Well*, well, em.getObject( wellid ) );
    return well ? well->zIsTime() : true;
}


const LineStyle& visSurvey::WellDisplay::lineStyle() const
{
    return drawstyle->lineStyle();
}


void visSurvey::WellDisplay::setLineStyle( const LineStyle& lst )
{
    drawstyle->setLineStyle( lst );
}


void visSurvey::WellDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( earthmodelidstr, wellid );
    par.set( displayattribstr, displayedattrib );
    // par.set( colortableidstr, 
    
    BufferString linestyle;
    drawstyle->lineStyle().toString( linestyle );
    par.set( linestylestr, linestyle );
}


int visSurvey::WellDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    int var;
    if ( !par.get( earthmodelidstr, var ))
	return -1;

    if ( !setWellId( var ) )
	return -1;

    if ( par.get( displayattribstr, var ) )
    {
	displayAttrib( var );
    }

    BufferString linestyle;
    if ( par.get( linestylestr, linestyle ) )
    {
	LineStyle lst;
	lst.fromString( linestyle );
	drawstyle->setLineStyle( lst );
    }

    return 1;
}
