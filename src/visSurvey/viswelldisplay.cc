/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: viswelldisplay.cc,v 1.13 2003-06-03 12:46:12 bert Exp $";

#include "vissurvwell.h"
#include "vispolyline.h"
#include "visdrawstyle.h"
#include "vistext.h"
#include "emmanager.h"
#include "emwell.h"
#include "iopar.h"
#include "executor.h"
#include "ptrman.h"


mCreateFactoryEntry( visSurvey::WellDisplay );

const char* visSurvey::WellDisplay::earthmodelidstr = "EarthModel ID";
const char* visSurvey::WellDisplay::displayattribstr = "Attrib";
const char* visSurvey::WellDisplay::colortableidstr = "ColorTable";
const char* visSurvey::WellDisplay::linestylestr = "Line style";
const char* visSurvey::WellDisplay::showwellnmstr = "Show name";


visSurvey::WellDisplay::WellDisplay()
    : line( visBase::PolyLine::create() )
    , drawstyle( visBase::DrawStyle::create() )
    , welltxt( visBase::Text::create() )
    , displayedattrib( -1 )
    , wellid( -1 )
{
    drawstyle->ref();
    addChild( drawstyle->getData() );
    line->ref();
    addChild( line->getData() );
    line->setMaterial( 0 );
    welltxt->ref();
    addChild( welltxt->getData() );
}


visSurvey::WellDisplay::~WellDisplay()
{
    removeChild( welltxt->getData() );
    welltxt->unRef();
    removeChild( line->getData() );
    line->unRef();
    removeChild( drawstyle->getData() );
    drawstyle->unRef();

    if ( EM::EMM().getObject( wellid ) )
	EM::EMM().unRef( wellid );
}


bool visSurvey::WellDisplay::setWellId( const MultiID& multiid )
{
    const EM::EMManager& em = EM::EMM();

    mDynamicCastGet( const EM::Well*, well, em.getObject( multiid ) );
    if ( !well ) return false;

    while ( line->size() ) line->removePoint( 0 );
    displayedattrib = -1;

    if ( EM::EMM().getObject( wellid ) )
	EM::EMM().unRef( wellid );

    wellid = multiid;
    well->ref();

    setName( well->name() );

    if ( well->nrKnots() < 1 ) return true;
    for ( int idx=0; idx<well->nrKnots(); idx++ )
	line->addPoint( well->getKnot( idx ) );

    welltxt->setText( well->name() );
    welltxt->setPosition( well->getKnot(0) );
    welltxt->setJustification( visBase::Text::Center );

    return true;
}


int  visSurvey::WellDisplay::nrAttribs() const
{
    const EM::EMManager& em = EM::EMM();
    mDynamicCastGet( const EM::Well*, well, em.getObject( wellid ) );
    return well ? well->nrValues() : -1;
}


const char* visSurvey::WellDisplay::getAttribName(int idx) const
{
    const EM::EMManager& em = EM::EMM();
    mDynamicCastGet( const EM::Well*, well, em.getObject( wellid ) );
    return well ? well->valueName(idx) : 0;
}


bool visSurvey::WellDisplay::depthIsT() const
{
    const EM::EMManager& em = EM::EMM();
    mDynamicCastGet( const EM::Well*, well, em.getObject( wellid ) );
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


void visSurvey::WellDisplay::showWellText( bool yn )
{
    welltxt->turnOn( yn );
}


bool visSurvey::WellDisplay::isWellTextShown() const
{
    return welltxt->isOn();
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

    bool welltxtshown = welltxt->isOn();
    par.setYN( showwellnmstr, welltxtshown );
}


int visSurvey::WellDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    MultiID newwellid;
    if ( !par.get( earthmodelidstr, newwellid ))
	return -1;

    EM::EMManager& em = EM::EMM();
    if ( !em.isLoaded( newwellid ) )
    {
	PtrMan<Executor> exec = em.load( newwellid );
	if ( !exec ) return -1;
	exec->execute();
    }
	
    if ( !setWellId( newwellid ) )
	return -1;

    int var;
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

    bool welltxtshown = true;
    par.getYN( showwellnmstr, welltxtshown );
    showWellText( welltxtshown );

    return 1;
}


void visSurvey::WellDisplay::setTransformation( visBase::Transformation* nt )
{
    line->setTransformation( nt );
    welltxt->setTransformation( nt );
}


visBase::Transformation* visSurvey::WellDisplay::getTransformation()
{ return line->getTransformation(); }
