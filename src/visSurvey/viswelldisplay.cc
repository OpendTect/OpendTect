/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: viswelldisplay.cc,v 1.14 2003-08-22 11:31:39 nanne Exp $";

#include "vissurvwell.h"
#include "vispolyline.h"
#include "visdrawstyle.h"
#include "vistext.h"
#include "welldata.h"
#include "welltransl.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "executor.h"
#include "ptrman.h"
#include "survinfo.h"


mCreateFactoryEntry( visSurvey::WellDisplay );

const char* visSurvey::WellDisplay::earthmodelidstr 	= "EarthModel ID";
const char* visSurvey::WellDisplay::ioobjidstr		= "IOObj ID";
const char* visSurvey::WellDisplay::linestylestr 	= "Line style";
const char* visSurvey::WellDisplay::showwellnmstr 	= "Show name";


visSurvey::WellDisplay::WellDisplay()
    : line( visBase::PolyLine::create() )
    , drawstyle( visBase::DrawStyle::create() )
    , welltxt( visBase::Text::create() )
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
}


#define mErrRet(s) { errmsg = s; return false; }

bool visSurvey::WellDisplay::setWellId( const MultiID& multiid )
{
    Well::Data wd; //TODO
    PtrMan<IOObj> ioobj = IOM().get( multiid );
    if ( !ioobj ) mErrRet( "Cannot find object in objectmanager" );

    PtrMan<Translator> t = ioobj->getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,t.ptr())
    if ( !wtr ) mErrRet( "Object is not a well" );

    if ( !wtr->read(wd,*ioobj) ) mErrRet( "Cannot read well" );
    
    const Well::D2TModel* d2t = wd.d2TModel();
    const bool zistime = SI().zIsTime();
    if ( zistime && !d2t ) mErrRet( "No depth to time model defined" );

    while ( line->size() ) line->removePoint( 0 );
    wellid = multiid;
    setName( wd.name() );

    if ( wd.track().size() < 1 ) return true;
    Coord3 pt;
    for ( int idx=0; idx<wd.track().size(); idx++ )
    {
	pt = wd.track().pos( idx );
	if ( zistime )
	    pt.z = d2t->getTime( wd.track().dah(idx) );
	line->addPoint( pt );
    }

    welltxt->setText( wd.name() );
    welltxt->setPosition( line->getPoint(0) ); //TODO
    welltxt->setJustification( visBase::Text::Center );

    return true;
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


void visSurvey::WellDisplay::setTransformation( visBase::Transformation* nt )
{
    line->setTransformation( nt );
    welltxt->setTransformation( nt );
}


visBase::Transformation* visSurvey::WellDisplay::getTransformation()
{ return line->getTransformation(); }
