/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: viswelldisplay.cc,v 1.46 2005-08-09 18:09:31 cvskris Exp $";

#include "viswelldisplay.h"
#include "viswell.h"
#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "welltransl.h"
#include "welltrack.h"
#include "wellmarker.h"
#include "welld2tmodel.h"
#include "iopar.h"
#include "executor.h"
#include "ptrman.h"
#include "survinfo.h"
#include "draw.h"
#include "visdataman.h"


mCreateFactoryEntry( visSurvey::WellDisplay );

namespace visSurvey
{

const char* WellDisplay::sKeyEarthModelID = "EarthModel ID";
const char* WellDisplay::sKeyWellID	 = "Well ID";
const char* WellDisplay::sKeyLog1Name	 = "Logname 1";
const char* WellDisplay::sKeyLog2Name	 = "Logname 2";
const char* WellDisplay::sKeyLog1Range	 = "Logrange 1";
const char* WellDisplay::sKeyLog2Range	 = "Logrange 2";
const char* WellDisplay::sKeyLog1Scale	 = "Loglogsc 1";
const char* WellDisplay::sKeyLog2Scale	 = "Loglogsc 2";
const char* WellDisplay::sKeyLog1Color	 = "Logcolor 1";
const char* WellDisplay::sKeyLog2Color	 = "Logcolor 2";

#define mMeter2Feet(val) \
   val /= 0.3048;

WellDisplay::WellDisplay()
    : well(0)
    , wellid(-1)
    , zistime(SI().zIsTime())
    , zinfeet(SI().zInFeet())
{
    setMaterial(0);
    setWell( visBase::Well::create() );
}


WellDisplay::~WellDisplay()
{
    removeChild( well->getInventorNode() );
    well->unRef();
}


void WellDisplay::setWell( visBase::Well* well_ )
{
    if ( well )
    {
	removeChild( well->getInventorNode() );
	well->unRef();
    }

    well = well_;    
    well->ref();
    addChild( well->getInventorNode() );
}


void WellDisplay::fullRedraw( CallBacker* )
{
    Well::Data* wd = Well::MGR().get( wellid, false );
    
    const Well::D2TModel* d2t = wd->d2TModel();
    setName( wd->name() );

    if ( wd->track().size() < 1 ) return;
    PtrMan<Well::Track> ttrack = 0;
    if ( zistime )
    {
	ttrack = new Well::Track( wd->track() );
	ttrack->toTime( *d2t );
    }
    Well::Track& track = zistime ? *ttrack : wd->track();

    TypeSet<Coord3> trackpos;
    Coord3 pt;
    StepInterval<double> sizrg;
    assign( sizrg, SI().zRange() );
    for ( int idx=0; idx<track.size(); idx++ )
    {
	pt = track.pos( idx );
	if ( zinfeet )
	    mMeter2Feet(pt.z);

	if ( !mIsUndefined(pt.z) && sizrg.includes(pt.z) )
	    trackpos += pt;
    }
    if ( !trackpos.size() )
	return;

    well->setTrack( trackpos );
    well->setWellName( wd->name(), trackpos[0] );
    updateMarkers(0);

    if ( log1nm.size() )
	displayLog( log1nm, log1logsc, log1rg, 1 );

    if ( log2nm.size())
	displayLog( log2nm, log2logsc, log2rg, 2 );
}


#define mErrRet(s) { errmsg = s; return false; }

bool WellDisplay::setWellId( const MultiID& multiid )
{
    Well::Data* wd = Well::MGR().get( multiid, true );
    if ( !wd ) return false;
    
    const Well::D2TModel* d2t = wd->d2TModel();
    if ( zistime )
    {
	if ( !d2t )
	    mErrRet( "No depth to time model defined" )
	wd->d2tchanged.notify( mCB(this,WellDisplay,fullRedraw) );
    }

    wellid = multiid;
    fullRedraw(0);
    wd->markerschanged.notify( mCB(this,WellDisplay,updateMarkers) );

    return true;
}


const LineStyle* WellDisplay::lineStyle() const
{
    return &well->lineStyle();
}


void WellDisplay::setLineStyle( const LineStyle& lst )
{
    well->setLineStyle( lst );
}


void WellDisplay::updateMarkers( CallBacker* )
{
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd ) return;

    well->removeAllMarkers();
    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	Well::Marker* wellmarker = wd->markers()[idx];
	Coord3 pos = wd->track().getPos( wellmarker->dah );
	if ( !pos.x && !pos.y && !pos.z ) continue;

	if ( zistime )
	    pos.z = wd->d2TModel()->getTime( wellmarker->dah );
	else if ( zinfeet )
	    mMeter2Feet(pos.z)

	well->addMarker( pos, wellmarker->color, wellmarker->name() );
    }
}


void WellDisplay::setMarkerScreenSize( int sz )
{ well->setMarkerScreenSize( sz ); }

int WellDisplay::markerScreenSize() const
{ return well->markerScreenSize(); }


#define mShowFunction( showObj, objShown ) \
void WellDisplay::showObj( bool yn ) \
{ \
    well->showObj( yn ); \
} \
\
bool WellDisplay::objShown() const \
{ \
    return well->objShown(); \
}

bool WellDisplay::canShowMarkers() const
{ return well->canShowMarkers(); }

mShowFunction( showWellName, wellNameShown )
mShowFunction( showMarkers, markersShown )
mShowFunction( showMarkerName, markerNameShown )
mShowFunction( showLogs, logsShown )
mShowFunction( showLogName, logNameShown )


void WellDisplay::displayLog( int logidx, int lognr, bool logrthm,
			      const Interval<float>* range )
{
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd || !wd->logs().size() ) return;

    Well::Log& log = wd->logs().getLog(logidx);
    const int logsz = log.size();
    if ( !logsz ) return;

    Well::Track& track = wd->track();
    TypeSet<Coord3Value> crdvals;
    StepInterval<double> sizrg;
    assign( sizrg, SI().zRange() );
    for ( int idx=0; idx<logsz; idx++ )
    {
	const float dah = log.dah(idx);
	Coord3 pos = track.getPos( dah );
	if ( !pos.x && !pos.y && !pos.z ) continue;

	if ( zistime )
	    pos.z = wd->d2TModel()->getTime( dah );
	else if ( zinfeet )
	    mMeter2Feet(pos.z)

	if ( !sizrg.includes(pos.z) )
	{
	    if ( pos.z > sizrg.stop ) break;
	    continue;
	}

	Coord3Value cv( pos, log.value(idx) );
	crdvals += cv;
    }

    Interval<float> selrange;
    assign( selrange, range ? *range : log.selValueRange() );
    if ( !range )
	logrthm = log.dispLogarithmic();
    well->setLogData( crdvals, log.name(), selrange, logrthm, lognr );

    if ( lognr == 1 )
	{ log1nm = log.name(); assign(log1rg,selrange); log1logsc = logrthm; }
    else
	{ log2nm = log.name(); assign(log2rg,selrange); log2logsc = logrthm; }
}


void WellDisplay::displayLog( const char* lognm, bool logarthm,
			      const Interval<float>& range, int lognr )
{
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd || !wd->logs().size() ) return;

    int logidx = -1;
    for ( int idx=0; idx<wd->logs().size(); idx++ )
    {
	const char* nm = wd->logs().getLog(idx).name();
	if ( !strcmp(lognm,nm) ) { logidx = idx; break; }
    }

    if ( logidx < 0 ) return; // TODO: errmsg
    
    displayLog( logidx, lognr, logarthm, &range );
}


void WellDisplay::setLogColor( const Color& col, int lognr )
{ well->setLogColor( col, lognr ); }


const Color& WellDisplay::logColor( int lognr ) const
{ return well->logColor( lognr ); }


void WellDisplay::setLogLineWidth( float width, int lognr )
{ well->setLogLineWidth( width, lognr ); }


float WellDisplay::logLineWidth( int lognr ) const
{ return well->logLineWidth( lognr ); }


void WellDisplay::setLogWidth( int width )
{ well->setLogWidth( width ); }


int WellDisplay::logWidth() const
{ return well->logWidth(); }


void WellDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyEarthModelID, wellid );

    int viswellid = well->id();
    par.set( sKeyWellID, viswellid );
    if ( saveids.indexOf(viswellid) == -1 ) saveids += viswellid;
    
    BufferString colstr;

#define mStoreLogPars( num ) \
    par.set( sKeyLog##num##Name, log##num##nm ); \
    par.set( sKeyLog##num##Range, log##num##rg.start, log##num##rg.stop ); \
    par.setYN( sKeyLog##num##Scale, log##num##logsc ); \
    logColor(num).fill( colstr.buf() ); \
    par.set( sKeyLog##num##Color, colstr )

    mStoreLogPars(1);
    mStoreLogPars(2);
}


int WellDisplay::usePar( const IOPar& par )
{
    int res = visBase::VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    int viswellid;
    if ( par.get(sKeyWellID,viswellid) )
    {
	DataObject* dataobj = visBase::DM().getObject( viswellid );
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::Well*,well_,dataobj)
	if ( !well_ ) return -1;
	setWell( well_ );
    }
    else
    {
	setWell( visBase::Well::create() );
	viswellid = well->id();
    }
    
    setDisplayTransformation( SPM().getUTM2DisplayTransform() );

    MultiID newmid;
    if ( !par.get(sKeyEarthModelID,newmid) )
	return -1;

    if ( !setWellId(newmid) )
    {
	return 1;
    }

    BufferString lognm_;
    BufferString colstr;
    Color col;

#define mRetrieveLogPars( num ) \
    par.get( sKeyLog##num##Name, lognm_ ); \
    par.get( sKeyLog##num##Range, log##num##rg.start, log##num##rg.stop ); \
    par.getYN( sKeyLog##num##Scale, log##num##logsc ); \
    if ( *lognm_.buf() ) \
	displayLog( lognm_, log##num##logsc, log##num##rg, num ); \
    par.get( sKeyLog##num##Color, colstr ); \
    if ( col.use(colstr.buf()) ) \
	setLogColor( col, num )

    mRetrieveLogPars( 1 );
    mRetrieveLogPars( 2 );

// Support for old sessions
    BufferString linestyle;
    if ( par.get(visBase::Well::linestylestr,linestyle) )
    {
	LineStyle lst;
	lst.fromString( linestyle );
	setLineStyle( lst );
    }

    bool wellnmshown;
    if ( par.getYN(visBase::Well::showwellnmstr,wellnmshown) )
	showWellName( wellnmshown );

    return 1;
}


void WellDisplay::setDisplayTransformation( visBase::Transformation* nt )
{
    well->setDisplayTransformation( nt );
}


visBase::Transformation* WellDisplay::getDisplayTransformation()
{ return well->getDisplayTransformation(); }

}; // namespace visSurvey
