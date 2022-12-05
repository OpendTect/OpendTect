/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "viswelldisplay.h"

#include "color.h"
#include "dataclipper.h"
#include "draw.h"
#include "iopar.h"
#include "ptrman.h"
#include "survinfo.h"
#include "refcount.h"
#include "uistrings.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vistransform.h"

#include "welldisp.h"
#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"
#include "wellmarker.h"
#include "welld2tmodel.h"
#include "zaxistransform.h"

#define	mPickSz	3
#define	mPickType 3

namespace visSurvey
{

static Well::LoadReqs defReqs()
{
    Well::LoadReqs lreqs( Well::D2T, Well::Trck, Well::DispProps3D );
    lreqs.add( Well::LogInfos );
    return lreqs;
}

#define mGetWD(act) RefMan<Well::Data> wd = getWD(defReqs()); \
					    if ( !wd ) { act; }
#define mGetDispPar(param) wd->displayProperties().param


#define mAutoRes 0

const char* WellDisplay::sKeyEarthModelID	= "EarthModel ID";
const char* WellDisplay::sKeyWellID		= "Well ID";

WellDisplay::WellDisplay()
    : VisualObjectImpl(true)
    , changed_(this)
    , zistime_( SI().zIsTime() )
    , zinfeet_( SI().zInFeet() )
{
    setMaterial( nullptr );
    setWell( visBase::Well::create() );
}


WellDisplay::~WellDisplay()
{
    detachAllNotifiers();
    setZAxisTransform( nullptr, nullptr );
    removeChild( well_->osgNode() );
    unRefPtr( well_ );
    setSceneEventCatcher( nullptr );
    unRefPtr( transformation_ );

    delete dispprop_;
    unRefPtr( markerset_ );
    delete pseudotrack_;
    delete timetrack_;
}


RefMan<Well::Data> WellDisplay::getWD( const Well::LoadReqs& reqs ) const
{
    Well::LoadReqs lreqs( reqs );
    Well::Man& wllmgr = Well::MGR();
    if ( !wd_ )
    {
	WellDisplay* self = const_cast<WellDisplay*>( this );
	lreqs.includes( Well::DispProps3D );
	const bool isloaded = wllmgr.isLoaded( wellid_ );
	if ( isloaded )
	    wllmgr.reload( wellid_, lreqs );

	self->wd_ = wllmgr.get( wellid_, lreqs );
	if ( wd_ )
	{
	    const auto& dispprop3d = wd_->displayProperties();
	    if ( !isloaded && !dispprop3d.isValid() )
	    { //Only for wells without pre-existing display properties
		if ( dispprop3d.getTrack().getColor() == OD::Color::NoColor() )
		{
		    self->wd_->displayProperties()
			   .ensureColorContrastWith( getBackgroundColor() );
		}
	    }
	    attachCB( wd_->trackchanged, mCB(self,WellDisplay,fullRedraw) );
	    attachCB( wd_->markerschanged,mCB(self,WellDisplay,updateMarkers));
	    attachCB( wd_->disp3dparschanged,mCB(self,WellDisplay,fullRedraw));
	    if ( zistime_ )
		attachCB( wd_->d2tchanged, mCB(self,WellDisplay,fullRedraw) );
	}
    }
    else
    {
	wllmgr.get( wellid_, lreqs );
    }

    return wd_;
}


void WellDisplay::saveDispProp( const Well::Data* wd )
{
    deleteAndZeroPtr( dispprop_ );
    if ( wd )
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
    tp.size_		= mGetDispPar( getTrack().getSize() );
    tp.col_		= mGetDispPar( getTrack().getColor() );
    tp.isdispabove_	= mGetDispPar( getTrack().dispabove_ );
    tp.isdispbelow_	= mGetDispPar( getTrack().dispbelow_ );
    tp.font_		= mGetDispPar( getTrack().font_ );
    tp.nmsizedynamic_	= mGetDispPar( getTrack().nmsizedynamic_ );
}


void WellDisplay::fillMarkerParams( visBase::Well::MarkerParams& mp )
{
    mGetWD(return);
    mp.size_		= mGetDispPar( getMarkers().getSize() );
    mp.col_		= mGetDispPar( getMarkers().getColor()  );
    mp.shapeint_	= mGetDispPar( getMarkers().shapeint_ );
    mp.cylinderheight_	= mGetDispPar( getMarkers().cylinderheight_ );
    mp.font_		= mGetDispPar( getMarkers().font_ );
    mp.namecol_		= mGetDispPar( getMarkers().nmcol_ );
    mp.nmsizedynamic_	= mGetDispPar( getMarkers().nmsizedynamic_ );
}


#define mGetLogPar(side,par)\
side==visBase::Well::Left ? mGetDispPar(getLogs().left_.par) :\
side==visBase::Well::Center ? mGetDispPar(getLogs().center_.par) :\
			      mGetDispPar(getLogs().right_.par)

void WellDisplay::fillLogParams(
		visBase::Well::LogParams& lp, visBase::Well::Side side )
{
    mGetWD(return);
    lp.size_		= mGetLogPar( side, getSize() );
    lp.col_		= mGetLogPar( side, getColor() );
    lp.cliprate_	= mGetLogPar( side, cliprate_ );
    lp.fillname_	= mGetLogPar( side, fillname_ );
    lp.fillrange_	= mGetLogPar( side, fillrange_ );
    lp.isdatarange_	= mGetLogPar( side, isdatarange_ );
    lp.isleftfilled_	= mGetLogPar( side, isleftfill_ );
    lp.isrightfilled_	= mGetLogPar( side, isrightfill_ );
    lp.issinglcol_	= mGetLogPar( side, issinglecol_);
    lp.islogarithmic_	= mGetLogPar( side, islogarithmic_ );
    lp.logwidth_	= mGetLogPar( side, logwidth_ );
    lp.name_		= mToUiStringTodo(mGetLogPar( side, name_ ));
    lp.ovlap_		= mGetLogPar( side, repeatovlap_ );
    lp.range_		= mGetLogPar( side, range_ );
    lp.repeat_		= mGetLogPar( side, repeat_);
    lp.seqname_		= mGetLogPar( side, seqname_ );
    lp.seiscolor_	= mGetLogPar( side, seiscolor_ );
    lp.iscoltabflipped_	= mGetLogPar( side, iscoltabflipped_ );
    int style		= mGetLogPar( side, style_ );
    lp.style_		= ( visBase::Well::LogStyle ) style;
}


#define mDispLog( dsplSide, Side )\
{ \
    const BufferString& logname = mGetLogPar( dsplSide, name_ );\
    if ( wd->getLog(logname) )\
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
    const Coord3 wellhead = trackpos.first();
    const Coord3 welltd = trackpos.last();
    tp.toppos_ = &wellhead;
    tp.botpos_ = &welltd;
    tp.name_ = mToUiStringTodo(wd->name());
    if ( wd_->displayProperties().isValid() ||
	 wd_->displayProperties().isModified() )
	updateMarkers( nullptr );

    well_->setTrack( trackpos );
    well_->setTrackProperties( tp.col_, tp.size_ );
    well_->setWellName( tp );

    well_->removeLogs();

    mDispLog( visBase::Well::Left, Left );
    mDispLog( visBase::Well::Right, Right );
    mDispLog( visBase::Well::Center, Center );
}


#define mErrRet(s) { errmsg_ = s; return false; }
bool WellDisplay::setMultiID( const MultiID& multiid )
{
    wd_ = nullptr;
    wellid_ = multiid;
    mGetWD(return false);
    const Well::D2TModel* d2t = wd->d2TModel();
    const bool trackabovesrd = wd->track().zRange().stop <
			      -1.f * float(SI().seismicReferenceDatum());
    if ( zistime_ && !d2t && !trackabovesrd )
	mErrRet( "No depth to time model defined" )

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
    if ( !wd )
	return;

    trackpos.erase();
    setName(  wd->name() );

    if ( wd->track().size() < 1 )
	return;

    const bool needsconversiontotime = needsConversionToTime();
    if ( needsconversiontotime )
    {
	if ( !timetrack_ )
	    timetrack_ = new Well::Track( wd->track() );
	else
	    *timetrack_ = wd->track();

	timetrack_->toTime( *wd );
    }

    const Well::Track& track = needsconversiontotime
			     ? *timetrack_ : wd->track();

    Coord3 pt;
    for ( int idx=0; idx<track.size(); idx++ )
    {
	pt = track.pos( idx );

	if ( !mIsUdf(pt.z) )
	    trackpos += pt;
    }
}


void WellDisplay::updateMarkers( CallBacker* )
{
    if ( !well_ )
	return;

    mGetWD(return);

    const Well::DisplayProperties& dispprops = wd_->displayProperties();
    const Well::DisplayProperties::Markers& markerdp = dispprops.getMarkers();
    if ( markerdp.isEmpty() )
	return;

    well_->removeAllMarkers();
    const Well::LoadReqs lreqs( Well::Mrkrs );
    wd = getWD( lreqs );
    if ( !wd )
	return;

    visBase::Well::MarkerParams mp;
    fillMarkerParams( mp );
    well_->setMarkerSetParams( mp );

    const Well::Track& track =
		needsConversionToTime() ? *timetrack_ : wd->track();
    const Well::MarkerSet& markers = wd->markers();
    for ( const auto* wellmarker : markers )
    {
	if ( !markerdp.isSelected(wellmarker->name()) )
	    continue;

	const Coord3 pos = track.getPos( wellmarker->dah() );
	if ( pos.isUdf() )
	    continue;

	mp.pos_ = &pos;
	mp.name_ = mToUiStringTodo( wellmarker->name() );

	if ( !mGetDispPar(getMarkers().issinglecol_) )
	    mp.col_ = wellmarker->color();

	if ( mGetDispPar(getMarkers().samenmcol_) )
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


void WellDisplay::setResolution( int res, TaskRunner* )
{
    logresolution_ = res;
    fullRedraw( nullptr );
}


int WellDisplay::getResolution() const
{
    return logresolution_;
}


int WellDisplay::nrResolutions() const
{
    return 6;
}


BufferString WellDisplay::getResolutionName( int res ) const
{
    return uiStrings::sResolutionValue(res).getFullString();
}


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
#define cMaxLogSamp 2000

void WellDisplay::setLogData( visBase::Well::LogParams& lp, bool isfilled )
{
    const Well::LoadReqs lreqs( Well::Trck,  Well::LogInfos );
    RefMan<Well::Data> wd = getWD( lreqs );
    if ( !wd )
	return;

    if (wd->track().size() < 2)
	return;

    const Well::Log& curlog = *wd->getLog( lp.name_ );
    const BufferString lognm = curlog.name();
    StepInterval<float> dahrg = curlog.dahRange();
    dahrg.step = curlog.dahStep( true );
    const float lognrsamples = dahrg.nrfSteps();

    int logres = getResolution();
    if ( logres == mAutoRes && lognrsamples > cMaxLogSamp )
	dahrg.step = dahrg.width() / float(cMaxLogSamp-1);
    else if ( logres > 1 )
    {
	const float fact = Math::PowerOf( 2.f, float(logres-1) );
	const float nrsamples = lognrsamples / fact;
	dahrg.step = dahrg.width() / nrsamples;
    }

    PtrMan<Well::Log> logdata = curlog.upScaleLog( dahrg );
    logdata->setName( lognm );

    PtrMan<Well::Log> logfill;
    if ( isfilled && lognm==lp.fillname_ )
	logfill = new Well::Log( *logdata );
    else if ( isfilled )
	logfill = wd->getLog( lp.fillname_ )->upScaleLog( dahrg );

    const Well::Track& track =
		needsConversionToTime() ? *timetrack_ : wd->track();
    float minval=mUdf(float), maxval=-mUdf(float);
    float minvalF=mUdf(float), maxvalF=-mUdf(float);

    TypeSet<visBase::Well::Coord3Value> crdvals;
    TypeSet<visBase::Well::Coord3Value> crdvalsF;

    const int logsz = logdata->size();
    for ( int idx=0; idx<logsz; idx++ )
    {
	const float dah = logdata->dah( idx );

	Coord3 pos = track.getPos( dah );
	if ( pos.isUdf() )
	    continue;

	if ( mIsUdf(pos.z) )
	    continue;

	float val = logdata->value(idx);
	if ( mIsUdf(val) )
	    continue;

	val = lp.range_.limitValue( val );
	minval = getminVal(minval,val);
	maxval = getmaxVal(maxval,val);
	crdvals += visBase::Well::Coord3Value( pos, val );

	if ( isfilled )
	{
	    const float valfill = logfill->value( idx );
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
    well_->setLogData( crdvals,crdvalsF,lp,isfilled );

}


static bool mustBeFilled( const visBase::Well::LogParams& lp )
{
    return (lp.isleftfilled_ || lp.isrightfilled_) && lp.filllogidx_>=0;
}


void WellDisplay::setLogDisplay( visBase::Well::Side side )
{
    const Well::LoadReqs lreqs( Well::LogInfos );
    RefMan<Well::Data> wd = getWD( lreqs );
    if ( !wd )
	return;

    const BufferString& logname = mGetLogPar( side, name_ );
    if ( wd->logs().isEmpty() ) return;
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


void WellDisplay::displayCenterLog()
{ setLogDisplay( visBase::Well::Center ); }


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
    well_->setLogWidth( float(lp.logwidth_), side );

    if ( lp.cliprate_ && lp.logidx_ >= 0 )
	calcClippedRange( lp.cliprate_, lp.range_, lp.logidx_ );

    requestSingleRedraw();
}


void WellDisplay::calcClippedRange( float rate, Interval<float>& rg, int lidx )
{
    mGetWD(return);

    Well::Log& wl = wd->logs().getLog( lidx );
    if ( rate > 100 ) rate = 100;
    if ( mIsUdf(rate) || rate < 0 ) rate = 0;
    rate /= 100;
    const int logsz = wl.size();
    DataClipper dataclipper;
    dataclipper.setApproxNrValues( logsz );
    dataclipper.putData( wl.valArr(), logsz );
    dataclipper.calculateRange( rate, rg );
}


const OD::Color& WellDisplay::logColor( visBase::Well::Side side ) const
{
    return well_->logColor( side );
}


void WellDisplay::setLogColor( const OD::Color& col, visBase::Well::Side side )
{
    well_->setLogColor( col, side );
}


float WellDisplay::getLogWidth( visBase::Well::Side side ) const
{
    return well_->getLogWidth( side );
}


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


OD::Color WellDisplay::getColor() const
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

    const Well::Track& track =
		needsconversiontotime ? *timetrack_ : wd->track();

    info = "Well: "; info += wd->name();
    info += ", MD ";

    info += zinfeet_ || SI().depthsInFeet() ? "(ft): " : "(m): ";
    const float zfac = SI().depthsInFeet() && SI().zIsTime() ?
							mToFeetFactorF : 1;

    Coord3 mouseworldpos = pos;
    const bool needsdatatransform = datatransform_ && ( !zistime_ ||
				!scene_->zDomainInfo().def_.isDepth() ) ;
    if ( needsdatatransform )
	mouseworldpos.z = datatransform_->transformBack( mouseworldpos );

    const float dah = track.nearestDah( mouseworldpos );

    info += toString( mNINT32(dah*zfac) );

    setLogInfo( info, val, dah, visBase::Well::Left );
    setLogInfo( info, val, dah, visBase::Well::Center );
    setLogInfo( info, val, dah, visBase::Well::Right );

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
				float dah, visBase::Well::Side side ) const
{
    mGetWD(return);

    BufferString lognm( mGetLogPar(side,name_) );
    if ( !lognm.isEmpty() && !lognm.isEqual("None") && !lognm.isEqual("none") )
    {
	if ( side==visBase::Well::Left )
	    info.add( ", Left: " );
	else if ( side==visBase::Well::Center )
	    info.add( ", Center: " );
	else
	    info.add( ", Right: ");

	info += lognm;
	const Well::Log* log = wd->logs().getLog( lognm.buf() );
	if (log)
	{
	    if ( val.size() ) val += " / ";
	    val += toString( log->getValue( dah ) );
	    val += " ";
	    val += log->unitMeasLabel();
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
    fullRedraw( nullptr );
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

    VisID eventid;
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
	mousepressposition_ = !eventid.isValid()
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
    if ( !mousepressid_.isValid() )
	mEventReturn

    if ( !evinfo.pickedobjids.isPresent(markerset_->id()) )
	return;

    const int markeridx =
	markerset_->findClosestMarker( evinfo.displaypickedpos, true );
    if ( markeridx<0 || markeridx>=markerset_->size() )
	return;

    markerset_->removeMarker( markeridx );
    pseudotrack_->removePoint( markeridx );

    TypeSet<Coord3> wcoords = getWellCoords();
    well_->setTrack( wcoords );
    needsave_ = true;
    changed_.trigger();
}


void WellDisplay::addPick( const visBase::EventInfo& eventinfo, VisID eventid )
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


void WellDisplay::addPick( Coord3 pos )
{
    if ( !pseudotrack_ )
	return;

    const double zfactor =
		double( scene_ ? scene_->getZScale() : SI().zScale() );
    const int insertidx = pseudotrack_->insertPoint(
					Coord3(pos.x,pos.y,pos.z*zfactor) );

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
	    markerset_->getMaterial()->setColor( lineStyle()->color_,
						 mid, false ) ;
	}
    }
}


void WellDisplay::addKnownPos()
{
    if ( !pseudotrack_ )
	return;

    TypeSet<Coord3> wcoords;
    for ( int idx=0; idx<pseudotrack_->size(); idx++ )
	wcoords += pseudotrack_->pos(idx);

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
	MarkerStyle3D markerstyle;
	markerstyle.size_ = mPickSz;
	markerstyle.type_ = (MarkerStyle3D::Type) mPickSz;
	markerset_->setMarkerStyle( markerstyle );
	mTryAlloc( pseudotrack_, Well::Track() );
    }

    markerset_->turnOn( yn );
}


void WellDisplay::showKnownPositions()
{
    mGetWD(return);
    setName( wd->name() );
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
    const double zfactor =
		double( scene_ ? scene_->getZScale() : SI().zScale() );
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

    fullRedraw( nullptr );
    return true;
}


const ZAxisTransform* WellDisplay::getZAxisTransform() const
{
    return datatransform_;
}


void WellDisplay::dataTransformCB( CallBacker* )
{
    fullRedraw( nullptr );
}


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
	return true;

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
