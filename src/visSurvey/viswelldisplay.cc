/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/


#include "viswelldisplay.h"


#include "dataclipper.h"
#include "draw.h"
#include "iopar.h"
#include "executor.h"
#include "ptrman.h"
#include "survinfo.h"
#include "refcount.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vistransform.h"

#include "welldisp.h"
#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welltransl.h"
#include "welltrack.h"
#include "wellmarker.h"
#include "welld2tmodel.h"
#include "zaxistransform.h"

#define	mPickSz	3
#define	mPickType 3

#define mGetWD(act) RefMan<Well::Data> wd = getWD(); if ( !wd ) { act; }
#define mMeter2Feet(val) val *= mToFeetFactorF;
#define mFeet2Meter(val) val *= mFromFeetFactorF;
#define mGetDispPar(param) wd->displayProperties().param
#define mGetTrackDispPar(fnnm) mGetDispPar( track().fnnm() )
#define mGetMarkersDispPar(fnnm) mGetDispPar( markers().fnnm() )
#define mGetLogDispPar(fnnm) mGetDispPar( log(isleft).fnnm() )


namespace visSurvey
{

const char* WellDisplay::sKeyEarthModelID = "EarthModel ID";
const char* WellDisplay::sKeyWellID = "Well ID";

WellDisplay::WellDisplay()
    : VisualObjectImpl(true)
    , well_(0)
    , wd_(0)
    , wellid_(-1)
    , zistime_( SI().zIsTime() )
    , zinfeet_( SI().zInFeet() )
    , eventcatcher_(0)
    , changed_(this)
    , logsnumber_(0)
    , transformation_(0)
    , picksallowed_(false)
    , pseudotrack_(0)
    , timetrack_(0)
    , needsave_(false)
    , dispprop_(0)
    , datatransform_(0)
    , markerset_(0)
{
    setMaterial(0);
    setWell( visBase::Well::create() );
}


WellDisplay::~WellDisplay()
{
    setZAxisTransform( 0, 0 );
    removeChild( well_->osgNode() );
    well_->unRef(); well_ = 0;
    setSceneEventCatcher(0);
    if ( transformation_ )
	transformation_->unRef();
    if ( wd_ )
    {
	wd_->trackchanged.remove( mCB(this,WellDisplay,fullRedraw) );
	wd_->markerschanged.remove( mCB(this,WellDisplay,updateMarkers) );
	wd_->disp3dparschanged.remove( mCB(this,WellDisplay,fullRedraw) );
	if ( zistime_ )
	    wd_->d2tchanged.remove( mCB(this,WellDisplay,fullRedraw) );
	wd_->unRef();
    }

    delete dispprop_;
    unRefAndZeroPtr( markerset_ );
    delete pseudotrack_;
    delete timetrack_;
}


void WellDisplay::welldataDelNotify( CallBacker* )
{
    saveDispProp( wd_ );
    wd_ = 0;
}


Well::Data* WellDisplay::getWD() const
{
    if ( !wd_ )
    {
	WellDisplay* self = const_cast<WellDisplay*>( this );
	RefMan<Well::Data> wd = Well::MGR().get( wellid_ );
	self->wd_ = wd;
	if ( wd )
	{
	    wd->trackchanged.notify( mCB(self,WellDisplay,fullRedraw) );
	    wd->markerschanged.notify( mCB(self,WellDisplay,updateMarkers) );
	    wd->disp3dparschanged.notify( mCB(self,WellDisplay,fullRedraw) );
	    if ( zistime_ )
		wd->d2tchanged.notify( mCB(self,WellDisplay,fullRedraw) );

	    wd_->ref();
	}
    }

    return wd_;
}


void WellDisplay::saveDispProp( const Well::Data* wd )
{
    if ( !wd ) return;
    dispprop_ = new Well::DisplayProperties( wd->displayProperties() );
}


void WellDisplay::restoreDispProp()
{
    if ( !dispprop_ )
	return;
    mGetWD( return );
    wd->displayProperties() = *dispprop_;
}


void WellDisplay::setWell( visBase::Well* well )
{
    if ( well_ )
    {
	removeChild( well_->osgNode() );
	well_->unRef();
    }
    well_ = well;
    well_->ref();
    addChild( well_->osgNode() );
}


void WellDisplay::fillTrackParams( visBase::Well::TrackParams& tp )
{
    mGetWD(return);
    tp.col_		= mGetTrackDispPar( color );
    tp.isdispabove_	= mGetTrackDispPar( dispAbove );
    tp.isdispbelow_	= mGetTrackDispPar( dispBelow );
    tp.font_		= mGetTrackDispPar( font );
    tp.size_		= mGetTrackDispPar( size );
}


void WellDisplay::fillMarkerParams( visBase::Well::MarkerParams& mp )
{
    mGetWD(return);
    mp.col_		= mGetMarkersDispPar( color  );
    mp.font_		= mGetMarkersDispPar( font );
    mp.size_		= mGetMarkersDispPar( size );
    mp.shapeint_	= mGetMarkersDispPar( shapeType );
    mp.cylinderheight_	= mGetMarkersDispPar( cylinderHeight );
    mp.namecol_		= mGetMarkersDispPar( nameColor );
}


void WellDisplay::fillLogParams(
		visBase::Well::LogParams& lp, visBase::Well::Side side )
{
    const bool isleft = side == 0;
    mGetWD(return);
    lp.cliprate_	= mGetLogDispPar( clipRate );
    lp.col_		= mGetLogDispPar( color );
    lp.fillname_	= mGetLogDispPar( fillName );
    lp.fillrange_	= mGetLogDispPar( fillRange );
    lp.isdatarange_	= mGetLogDispPar( isDataRange );
    lp.isleftfilled_	= mGetLogDispPar( fillLeft );
    lp.isrightfilled_	= mGetLogDispPar( fillRight );
    lp.issinglcol_	= mGetLogDispPar( singleColor);
    lp.islogarithmic_	= mGetLogDispPar( isLogarithmic );
    lp.logwidth_	= mGetLogDispPar( logWidth );
    lp.name_		= toUiString( mGetLogDispPar(logName) );
    lp.ovlap_		= mGetLogDispPar( repeatOverlap );
    lp.range_		= mGetLogDispPar( range );
    lp.repeat_		= mGetLogDispPar( repeat );
    lp.seqname_		= mGetLogDispPar( seqName );
    lp.size_		= mGetLogDispPar( size );
    lp.seiscolor_	= mGetLogDispPar( seisColor );
    lp.iscoltabflipped_	= mGetLogDispPar( colTabFlipped );
    const int style	= mGetLogDispPar( style );
    lp.style_		= (visBase::Well::LogStyle)style;
}


#define mDispLog( Side )\
{ \
    const BufferString logname = mGetLogDispPar( logName );\
    if ( wd->logs().indexOf(logname) >= 0 )\
	display##Side##Log();\
}

void WellDisplay::fullRedraw( CallBacker* )
{
    mGetWD(return);
    if ( !well_ ) return;

    TypeSet<Coord3> trackpos;
    getTrackPos( wd, trackpos );
    if ( trackpos.isEmpty() ) return;

    visBase::Well::TrackParams tp;
    fillTrackParams( tp );
    tp.toppos_ = &trackpos[0]; tp.botpos_ = &trackpos[trackpos.size()-1];
    tp.name_ = toUiString(wd->name());
    updateMarkers(0);

    well_->setTrack( trackpos );
    well_->setTrackProperties( tp.col_, tp.size_ );
    well_->setWellName( tp );
    well_->removeLogs();

    bool isleft = true; mDispLog( Left );
    isleft = false; mDispLog( Right );
}


#define mErrRet(s) { errmsg_ = s; return false; }
bool WellDisplay::setMultiID( const MultiID& multiid )
{
    wellid_ = multiid; wd_ = 0;
    mGetWD(return false);
    const Well::D2TModel& d2t = wd->d2TModel();
    const bool trackabovesrd = wd->track().zRange().stop <
			      -1.f * mCast(float,SI().seismicReferenceDatum());
    if ( zistime_ && d2t.isEmpty() && !trackabovesrd )
	mErrRet( "No depth to time model defined" )

    wellid_ = multiid;
    fullRedraw(0);
    changed_.trigger();
    return true;
}

bool WellDisplay::needsConversionToTime() const
{
    return zistime_ && (scene_ && !scene_->zDomainInfo().def_.isDepth());
}


void WellDisplay::getTrackPos( const Well::Data* wd,
			       TypeSet<Coord3>& trackpos )
{
    trackpos.erase();
    setName(  toUiString(wd->name()) );

    if ( wd->track().size() < 1 )
	return;

    const bool needsconversiontotime = needsConversionToTime();
    if ( needsconversiontotime && !timetrack_ )
    {
	timetrack_ = new Well::Track( wd->track() );
	timetrack_->toTime( *wd );
    }

    const Well::Track& track = needsconversiontotime
			     ? *timetrack_ : wd->track();

    Well::TrackIter iter( track );
    while ( iter.next() )
    {
	Coord3 pt = iter.pos();
	if ( !mIsUdf(pt.z) )
	    trackpos += pt;
    }
}


void WellDisplay::updateMarkers( CallBacker* )
{
    if ( !well_ ) return;
    well_->removeAllMarkers();
    mGetWD(return);

    visBase::Well::MarkerParams mp;
    fillMarkerParams( mp );
    well_->setMarkerSetParams( mp );

    const Well::Track& track = needsConversionToTime() ? *timetrack_
						       : wd->track();
    const BufferStringSet selnms(
		wd->displayProperties(false).markers().selMarkerNames() );
    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	Well::Marker* wellmarker = wd->markers()[idx];
	if ( !selnms.isPresent( wellmarker->name() ) )
	    continue;

	Coord3 pos = track.getPos( wellmarker->dah() );
	if ( pos.isUdf() )
	    continue;

	mp.pos_ = &pos;
	mp.name_ = toUiString(wellmarker->name());

	if ( !mGetMarkersDispPar(singleColor) )
	    mp.col_ = wellmarker->color();
	if ( mGetMarkersDispPar(sameNameCol) )
	    mp.namecol_  = mp.col_;

	well_->addMarker( mp );
    }
}


void WellDisplay::setMarkerScreenSize( int sz )
{ well_->setMarkerScreenSize( sz ); }


int WellDisplay::markerScreenSize() const
{ return well_->markerScreenSize(); }


#define mShowFunction( showObj, objShown ) \
void WellDisplay::showObj( bool yn ) \
{ \
    well_->showObj( yn ); \
} \
\
bool WellDisplay::objShown() const \
{ \
    return well_->objShown(); \
}

bool WellDisplay::canShowMarkers() const
{ return well_->canShowMarkers(); }

mShowFunction( showWellTopName, wellTopNameShown )
mShowFunction( showWellBotName, wellBotNameShown )
mShowFunction( showMarkers, markersShown )
mShowFunction( showMarkerName, markerNameShown )
mShowFunction( showLogName, logNameShown )


const OD::LineStyle* WellDisplay::lineStyle() const
{
    return &well_->lineStyle();
}


void WellDisplay::setLineStyle( const OD::LineStyle& lst )
{
    well_->setLineStyle( lst );
}


#define getminVal(minval,val)  val < minval ? val : minval
#define getmaxVal(maxval,val)  val > maxval ? val : maxval

void WellDisplay::setLogData( visBase::Well::LogParams& lp, bool isfilled )
{
    mGetWD(return);

    ConstRefMan<Well::Log> shlogdata = wd->logs().getLogByIdx( lp.logidx_ );
    if ( !shlogdata )
	return;
    ConstRefMan<Well::Log> shlogfill = !isfilled ? 0
				   : wd->logs().getLogByIdx( lp.filllogidx_ );

    RefMan<Well::Log> logdata = new Well::Log( *shlogdata );
    RefMan<Well::Log> logfill = shlogfill ? new Well::Log( *shlogfill ) : 0;
    shlogdata.release();
    if ( shlogfill )
	shlogfill.release();
    if ( !upscaleLogs(wd->track(),*logdata,logfill) )
	return;

    const Well::Track& track = needsConversionToTime() ? *timetrack_
						       : wd->track();
    float minval=mUdf(float), maxval=-mUdf(float);
    float minvalF=mUdf(float), maxvalF=-mUdf(float);

    TypeSet<visBase::Well::Coord3Value> crdvals;
    TypeSet<visBase::Well::Coord3Value> crdvalsF;

    Well::LogIter iter( *logdata );
    while ( iter.next() )
    {
	const float dah = iter.dah();
	float val = iter.value();
	Coord3 pos = track.getPos( dah );
	if ( pos.isUdf() || mIsUdf(pos.z) || mIsUdf(val) )
	    continue;

	val = lp.range_.limitValue( val );
	minval = getminVal(minval,val);
	maxval = getmaxVal(maxval,val);
	crdvals += visBase::Well::Coord3Value( pos, val );

	if ( isfilled )
	{
	    const float valfill = logfill->valueAt( dah );
	    if ( !mIsUdf(valfill) )
	    {
		minvalF = getminVal(minvalF,valfill);
		maxvalF = getmaxVal(maxvalF,valfill);
	    }

	    crdvalsF += visBase::Well::Coord3Value( pos, valfill );
	}
    }

    if ( crdvals.isEmpty() && crdvalsF.isEmpty() )
	return;

    lp.valfillrange_ .set( minvalF, maxvalF );
    lp.valrange_.set( minval, maxval );
    well_->setLogData( crdvals, crdvalsF, lp, isfilled );
}


#define cMaxLogSamp 2000

bool WellDisplay::upscaleLogs( const Well::Track& track,
			       Well::Log& logdata, Well::Log* logfill ) const
{
    if ( track.size() < 2 )
	return false;

    ConstRefMan<Well::Log> logdatain = new Well::Log( logdata );
    ConstRefMan<Well::Log> logfillin = !logfill ? 0 : new Well::Log( *logfill );
    if ( logdatain->isEmpty() )
	return false;

    float start = logdatain->firstDah();
    if ( start < track.dahRange().start )
	start = track.dahRange().start;

    float stop = logdatain->lastDah();
    if ( stop > track.dahRange().stop )
	stop = track.dahRange().stop;

    StepInterval<float> dahrange( start, stop, mUdf(float) );
    dahrange.step = dahrange.width() / mCast(float, cMaxLogSamp-1 );

    logdata.setEmpty();
    if ( logfill )
	logfill->setEmpty();

    for ( int idah=0; idah<dahrange.nrSteps()+1; idah++ )
    {
	const float dah = dahrange.atIndex( idah );
	const float val = Well::LogDataExtracter::calcVal( *logdatain,
				dah, dahrange.step, Stats::UseAvg );
	logdata.setValueAt( dah, val );
	if ( logfill )
	{
	    const float fillval = Well::LogDataExtracter::calcVal( *logfillin,
					dah, dahrange.step, Stats::UseAvg );
	    logfill->setValueAt( dah, fillval );
	}
    }

    return true;
}


static bool mustBeFilled( const visBase::Well::LogParams& lp )
{
    return (lp.isleftfilled_ || lp.isrightfilled_) && lp.filllogidx_>=0;
}


void WellDisplay::setLogDisplay( visBase::Well::Side side )
{
    mGetWD(return);

    const bool isleft = side == 0;
    const BufferString logname = mGetLogDispPar( logName );
    const int logidx = wd->logs().indexOf( logname );
    if ( logidx<0 )
    {
	well_->clearLog( side );
	well_->showLog( false, side );
	return;
    }

    visBase::Well::LogParams lp;
    fillLogParams( lp, side );
    lp.logidx_ = logidx;  lp.side_ = side;
    lp.filllogidx_ = wd->logs().indexOf( lp.fillname_ );

    setLogProperties( lp );
    setLogData( lp, mustBeFilled(lp) );
}


void WellDisplay::displayRightLog()
{ setLogDisplay( visBase::Well::Right ); }


void WellDisplay::displayLeftLog()
{ setLogDisplay( visBase::Well::Left ); }


void WellDisplay::setOneLogDisplayed(bool yn)
{ onelogdisplayed_ = yn; }


void WellDisplay::setLogProperties( visBase::Well::LogParams& lp )
{
    const visBase::Well::Side side = lp.side_;

    well_->setLogStyle( lp.style_, side );

    well_->setOverlapp( lp.ovlap_, side );
    well_->setRepeat( lp.repeat_,side );
    well_->setLogFill( mustBeFilled(lp), side );
    well_->setLogFillColorTab( lp, side );
    well_->setLogLineDisplayed( lp.size_ > 0, side );

    well_->setLogColor( lp.col_, side );
    well_->setLogLineWidth( lp.size_, side );
    well_->setLogWidth( mCast(float,lp.logwidth_), side );

    if ( lp.cliprate_ && lp.logidx_ >= 0 )
	calcClippedRange( lp.cliprate_, lp.range_, lp.logidx_ );

    requestSingleRedraw();
}


void WellDisplay::calcClippedRange( float rate, Interval<float>& rg, int lidx )
{
    mGetWD(return);
    const Well::Log* wl = wd->logs().getLogByIdx( lidx );
    if ( !wl )
	return;

    TypeSet<float> dahs, vals;
    wl->getData( dahs, vals );
    if ( rate > 100 ) rate = 100;
    if ( mIsUdf(rate) || rate < 0 ) rate = 0;
    rate /= 100;
    DataClipper dataclipper;
    dataclipper.setApproxNrValues( vals.size() );
    dataclipper.putData( vals.arr(), vals.size() );
    dataclipper.calculateRange( rate, rg );
}


const Color& WellDisplay::logColor( visBase::Well::Side side ) const
{ return well_->logColor( side ); }


void WellDisplay::setLogColor( const Color& col, visBase::Well::Side side )
{ well_->setLogColor( col, side ); }


float WellDisplay::getLogWidth( visBase::Well::Side side ) const
{ return well_->getLogWidth( side ); }


void WellDisplay::setLogWidth( float width, visBase::Well::Side side )
{ well_->setLogWidth( width, side ); }


int WellDisplay::getLogLineWidth() const
{ return well_->getLogLineWidth(); }


void WellDisplay::setLogLineWidth( int width, visBase::Well::Side side )
{ well_->setLogLineWidth( width, side ); }


bool WellDisplay::logsShown() const
{ return well_->logsShown(); }


void WellDisplay::showLogs( bool yn )
{
    well_->showLogs( yn );
}


Color WellDisplay::getColor() const
{
    return well_->lineStyle().color_;
}


void WellDisplay::getMousePosInfo( const visBase::EventInfo&,
				   Coord3& pos,
				   BufferString& val,
				   BufferString& info ) const
{
    val.setEmpty(); info.setEmpty();
    mGetWD(return);

    const bool needsconversiontotime = needsConversionToTime();
    if ( needsconversiontotime && !timetrack_ )
	return;

    const Well::Track& track = needsconversiontotime
			     ? *timetrack_ : wd->track();

    info = "Well: "; info += wd->name();
    info += ", MD ";

    info += zinfeet_ || SI().depthsInFeet() ? "(ft): " : "(m): ";
    const float zfac = SI().depthsInFeet() && SI().zIsTime() ?
							mToFeetFactorF : 1;

    Coord3 mouseworldpos = pos;
    if ( datatransform_ )
	mouseworldpos.z = datatransform_->transformBack( mouseworldpos );

    const float dah = track.nearestDah( mouseworldpos );

    info += toString( mNINT32(dah*zfac) );

    setLogInfo( info, val, dah, true );
    setLogInfo( info, val, dah, false );

    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    const float zstep2 = zfactor*SI().zStep()/2;
    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	Well::Marker* wellmarker = wd->markers()[idx];
	if ( !mIsEqual(wellmarker->dah(),dah,zstep2) )
	    continue;

	info += ", Marker: ";
	info += wellmarker->name();
	break;
    }
}


void WellDisplay::setLogInfo( BufferString& info, BufferString& val,
				float dah, bool isleft ) const
{
    mGetWD(return);

    const BufferString lognm = mGetLogDispPar( logName );
    if ( !lognm.isEmpty() && !lognm.isEqual("None") && !lognm.isEqual("none") )
    {
	info += isleft ? ", Left: " : ", Right: ";
	info += lognm;
	const Well::Log* log = wd->logs().getLogByName( lognm );
	if ( log )
	{
	    if ( !val.isEmpty() )
		val.add( " / " );
	    val.add( toString(log->valueAt(dah)) ).add( " " )
		.add( log->unitMeasLabel() );
	}
    }
}


void WellDisplay::setDisplayTransformation( const mVisTrans* nt )
{
     if ( transformation_==nt  )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();

    well_->setDisplayTransformation( transformation_ );
    setDisplayTransformForPicks( transformation_ );
    fullRedraw(0);
}


const mVisTrans* WellDisplay::getDisplayTransformation() const
{ return well_->getDisplayTransformation(); }

#define mEventReturn \
{ \
    eventcatcher_->setHandled(); \
    return; \
}

void WellDisplay::pickCB( CallBacker* cb )
{
    if ( !isSelected() || !picksallowed_ || !markerset_ || isLocked() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);
    if ( eventinfo.type != visBase::MouseClick ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    int eventid = -1;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
	    visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	if ( dataobj->selectable() )
	{
	    eventid = eventinfo.pickedobjids[idx];
	    break;
	}
    }

    if ( eventinfo.pressed )
    {
	mousepressid_ = eventid;
	mousepressposition_ = eventid == -1
			    ? Coord3::udf() : eventinfo.displaypickedpos;
	mEventReturn
    }

    if ( OD::altKeyboardButton(eventinfo.buttonstate_) ||
	 OD::shiftKeyboardButton(eventinfo.buttonstate_) ||
	 eventid != mousepressid_ ||
	 eventinfo.pickedobjids.isEmpty() )
	mEventReturn

    if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
	removePick( eventinfo );
    else if ( !OD::ctrlKeyboardButton(eventinfo.buttonstate_) )
	addPick( eventinfo, eventid );

    mEventReturn
}


void WellDisplay::removePick( const visBase::EventInfo& evinfo )
{
    if ( mousepressid_ == -1 )
	mEventReturn

    if ( !evinfo.pickedobjids.isPresent(markerset_->id()) )
	return;

    const int markeridx =
	markerset_->findClosestMarker( evinfo.displaypickedpos, true );
    if ( markeridx<0 || markeridx>=markerset_->size() )
	return;

    markerset_->removeMarker( markeridx );
    pseudotrack_->removeByIdx( markeridx );

    TypeSet<Coord3> wcoords = getWellCoords();
    well_->setTrack( wcoords );
    needsave_ = true;
    changed_.trigger();
}


void WellDisplay::addPick( const visBase::EventInfo& eventinfo, int eventid )
{
    const int sz = eventinfo.pickedobjids.size();
    bool validpicksurface = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
	    visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	mDynamicCastGet(const SurveyObject*,so,pickedobj)
	if ( so && so->allowsPicks() )
	{
	    validpicksurface = true;
	    break;
	}
    }

    if ( !validpicksurface )
	return;

    Coord3 newpos = eventinfo.worldpickedpos;
    mDynamicCastGet(SurveyObject*,so,visBase::DM().getObject(eventid))
    if ( so ) so->snapToTracePos( newpos );
    addPick( newpos );
}


void WellDisplay::addPick( const Coord3& pos )
{
    if ( !pseudotrack_ )
	return;

    const double zfactor = mCast(double,
			   scene_ ? scene_->getZScale() : SI().zScale() );
    const Well::Track::PointID ptid = pseudotrack_->insertPoint(
					Coord3(pos.x,pos.y,pos.z*zfactor) );
    const int insertidx = pseudotrack_->indexOf( ptid );

    TypeSet<Coord3> wcoords = getWellCoords();
    well_->setTrack( wcoords );
    needsave_ = true;
    changed_.trigger();

    if ( insertidx > -1 )
    {
	markerset_->clearMarkers();
	for ( int idx=0; idx<wcoords.size(); idx++ )
	{
	    const int mid = markerset_->addPos( wcoords[idx] );
	    markerset_->getMaterial()->setColor( lineStyle()->color_, mid, false ) ;
	}
    }
}


void WellDisplay::addKnownPos()
{
    if ( !pseudotrack_ )
	return;

    TypeSet<Coord3> wcoords;
    Well::TrackIter iter( *pseudotrack_ );
    while ( iter.next() )
	wcoords += iter.pos();
    iter.retire();

    well_->setTrack( wcoords );
    needsave_ = true;
    changed_.trigger();

    for ( int idx=0; idx<pseudotrack_->size(); idx++ )
	markerset_->addPos( wcoords[idx] );
}


void WellDisplay::setDisplayTransformForPicks( const mVisTrans* newtr )
{
    if ( markerset_ )
	markerset_->setDisplayTransformation( newtr );
}


void WellDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,WellDisplay,pickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nevc;

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,WellDisplay,pickCB));
	eventcatcher_->ref();
    }
}


void WellDisplay::setupPicking( bool yn )
{
    picksallowed_ = yn;
    if ( !markerset_ )
    {
	markerset_ = visBase::MarkerSet::create();
	refPtr( markerset_ );
	setDisplayTransformForPicks( transformation_ );
	addChild( markerset_->osgNode() );
	markerset_->setMaterial( new visBase::Material );
	OD::MarkerStyle3D markerstyle;
	markerstyle.size_ = mPickSz;
	markerstyle.type_ = (OD::MarkerStyle3D::Type) mPickSz;
	markerset_->setMarkerStyle( markerstyle );
	mTryAlloc( pseudotrack_, Well::Track() );
    }

    markerset_->turnOn( yn );

}


void WellDisplay::showKnownPositions()
{
    mGetWD(return);
    setName( toUiString(wd->name()) );
    if ( !pseudotrack_ )
	return;

    *pseudotrack_ = wd->track();
    if ( zistime_ )
	pseudotrack_->toTime( *wd );

    if ( pseudotrack_->isEmpty() )
	return;

    addKnownPos();
}


TypeSet<Coord3> WellDisplay::getWellCoords() const
{
    const double zfactor = mCast( double,
			   scene_ ? scene_->getZScale() : SI().zScale() );

    TypeSet<Coord3> coords = pseudotrack_->getAllPos();
    for ( int idx=0; idx<coords.size(); idx++ )
	coords[idx].z /= zfactor;

    return coords;
}


bool WellDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    if ( datatransform_==zat )
	return true;

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB(this,WellDisplay,dataTransformCB) );
	datatransform_->unRef();
    }

    datatransform_ = zat;
    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		mCB(this,WellDisplay,dataTransformCB) );

	datatransform_->ref();
    }

    if ( well_ )
	well_->setZAxisTransform( zat, tr );
    fullRedraw(0);
    return true;
}


const ZAxisTransform* WellDisplay::getZAxisTransform() const
{ return datatransform_; }


void WellDisplay::dataTransformCB( CallBacker* )
{ fullRedraw(0); }


void WellDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );

    par.set( sKeyEarthModelID, wellid_ );

    mGetWD(return);
    wd->displayProperties().fillPar( par );

}


bool WellDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	  !visSurvey::SurveyObject::usePar( par ) )
	  return false;

    MultiID newmid;
    if ( !par.get(sKeyEarthModelID,newmid) )
	return false;

    if ( !setMultiID(newmid) )
    {
	return 1;
    }

    mGetWD(return false);
    wd->displayProperties().usePar( par );
    displayLeftLog();
    displayRightLog();

    return true;
}


void WellDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( markerset_ )
	markerset_->setPixelDensity( dpi );
    if ( well_ )
	well_->setPixelDensity( dpi );
}


} // namespace visSurvey
