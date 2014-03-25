/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "viswelldisplay.h"


#include "basemap.h"
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
#include "welltransl.h"
#include "welltrack.h"
#include "wellmarker.h"
#include "welld2tmodel.h"
#include "zaxistransform.h"

#define		mPickSz	3
#define         mPickType	3

#define mGetWD(act) Well::Data* wd = getWD(); if ( !wd ) act;
#define mMeter2Feet(val) val *= mToFeetFactorF;
#define mFeet2Meter(val) val *= mFromFeetFactorF;
#define mGetDispPar(param) wd->displayProperties().param


namespace visSurvey
{

class WellDisplayBaseMapObject : public BaseMapObject
{
public:
			WellDisplayBaseMapObject(WellDisplay* wd);

    const char*		getType() const;
    void		updateGeometry();
    int			nrShapes() const;
    const char*		getShapeName(int) const;
    void		getPoints(int,TypeSet<Coord>& res) const;
    char		connectPoints(int) const;
    const MarkerStyle2D* getMarkerStyle(int) const
			{ return &markerstyle_; }

protected:
    WellDisplay*	wd_;
    MarkerStyle2D	markerstyle_;
};


WellDisplayBaseMapObject::WellDisplayBaseMapObject( WellDisplay* wd )
    : BaseMapObject( wd->name() )
    , wd_( wd )
{
    markerstyle_.color_ = wd->getColor();
}


const char* WellDisplayBaseMapObject::getType() const
{ return "Well"; }


void WellDisplayBaseMapObject::updateGeometry()
{ changed.trigger(); }


int WellDisplayBaseMapObject::nrShapes() const
{ return 1; }


const char* WellDisplayBaseMapObject::getShapeName(int) const
{ return wd_->name(); }


void WellDisplayBaseMapObject::getPoints( int, TypeSet<Coord>& res ) const
{
    if ( wd_->getWD() )
	res += wd_->getWD()->info().surfacecoord;
}


const char* WellDisplay::sKeyEarthModelID      = "EarthModel ID";
const char* WellDisplay::sKeyWellID	       = "Well ID";

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
    , needsave_(false)
    , dispprop_(0)
    , datatransform_(0)
{
    setMaterial(0);
    setWell( visBase::Well::create() );
    markerset_ = visBase::MarkerSet::create();
    markerset_->ref();
    addChild( markerset_->osgNode() );
    markerset_->setMaterial( new visBase::Material );
    MarkerStyle3D markerstyle;
    markerstyle.size_ = mPickSz;
    markerstyle.type_ = (MarkerStyle3D::Type) mPickSz;
    markerset_->setMarkerStyle( markerstyle );
}


WellDisplay::~WellDisplay()
{
    setZAxisTransform( 0, 0 );
    removeChild( well_->osgNode() );
    well_->unRef(); well_ = 0;
    setSceneEventCatcher(0);
    if ( transformation_ ) transformation_->unRef();

    wd_ = 0;
    mGetWD(return);
    wd->tobedeleted.remove( mCB(this,WellDisplay,welldataDelNotify) );
    delete dispprop_;
    delete Well::MGR().release( wellid_ );
    markerset_->unRef();
    setBaseMap( 0 );
}


BaseMapObject* WellDisplay::createBaseMapObject()
{ return new WellDisplayBaseMapObject( this ); }


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
	Well::Data* wd = Well::MGR().get( wellid_, false );
	self->wd_ = wd;
	if ( wd )
	{
	    wd->tobedeleted.notify(
		    mCB(self,WellDisplay,welldataDelNotify) );
	    wd->trackchanged.notify( mCB(self,WellDisplay,fullRedraw) );
	    wd->markerschanged.notify( mCB(self,WellDisplay,updateMarkers) );
	    wd->disp3dparschanged.notify( mCB(self,WellDisplay,fullRedraw) );
	    if ( zistime_ )
		wd->d2tchanged.notify( mCB(self,WellDisplay,fullRedraw) );
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
    tp.col_		= mGetDispPar( track_.color_ );
    tp.isdispabove_	= mGetDispPar( track_.dispabove_ );
    tp.isdispbelow_	= mGetDispPar( track_.dispbelow_ );
    tp.font_		= mGetDispPar( track_.font_ );
    tp.size_		= mGetDispPar( track_.size_ );
}


void WellDisplay::fillMarkerParams( visBase::Well::MarkerParams& mp )
{
    mGetWD(return);
    mp.col_		= mGetDispPar( markers_.color_  );
    mp.shapeint_	= mGetDispPar( markers_.shapeint_ );
    mp.cylinderheight_	= mGetDispPar( markers_.cylinderheight_ );
    mp.font_		= mGetDispPar( markers_.font_ );
    mp.namecol_	= mGetDispPar( markers_.nmcol_ );
    mp.size_		= mGetDispPar( markers_.size_ );
}


#define mGetLogPar(side,par) side==0 ? mGetDispPar(logs_[0]->left_.par)\
				       : mGetDispPar(logs_[0]->right_.par)
void WellDisplay::fillLogParams(
		visBase::Well::LogParams& lp, visBase::Well::Side side )
{
    mGetWD(return);
    lp.cliprate_	= mGetLogPar( side, cliprate_ );
    lp.col_		= mGetLogPar( side, color_);
    lp.fillname_	= mGetLogPar( side, fillname_ );
    lp.fillrange_	= mGetLogPar( side, fillrange_ );
    lp.isdatarange_	= mGetLogPar( side, isdatarange_ );
    lp.isleftfilled_	= mGetLogPar( side, isleftfill_ );
    lp.isrightfilled_	= mGetLogPar( side, isrightfill_ );
    lp.issinglcol_	= mGetLogPar( side, issinglecol_);
    lp.islogarithmic_	= mGetLogPar( side, islogarithmic_ );
    lp.logwidth_	= mGetLogPar( side, logwidth_ );
    lp.name_		= mGetLogPar( side, name_ );
    lp.ovlap_		= mGetLogPar( side, repeatovlap_ );
    lp.range_		= mGetLogPar( side, range_ );
    lp.repeat_		= mGetLogPar( side, repeat_);
    lp.seqname_		= mGetLogPar( side, seqname_ );
    lp.size_		= mGetLogPar( side, size_ );
    lp.seiscolor_	= mGetLogPar( side, seiscolor_ );
    lp.iscoltabflipped_	= mGetLogPar( side, iscoltabflipped_ );
    int style		= mGetLogPar( side, style_ );
    lp.style_		= ( visBase::Well::LogStyle ) style;
}


#define mDispLog( dsplSide, Side )\
{ \
    BufferString& logname = mGetLogPar( dsplSide, name_ );\
    if ( wd->logs().indexOf( logname ) >= 0 )\
	display##Side##Log();\
}
void WellDisplay::fullRedraw( CallBacker* )
{
    mGetWD(return);
    if ( !well_ ) return;

    const bool waslogconstsize = well_->logConstantSize();

    TypeSet<Coord3> trackpos;
    getTrackPos( wd, trackpos );
    if ( trackpos.isEmpty() ) return;

    visBase::Well::TrackParams tp;
    fillTrackParams( tp );
    tp.toppos_ = &trackpos[0]; tp.botpos_ = &trackpos[trackpos.size()-1];
    tp.name_ = wd->name();
    updateMarkers(0);

    well_->setTrack( trackpos );
    well_->setTrackProperties( tp.col_, tp.size_ );
    well_->setWellName( tp );
    well_->removeLogs();
    well_->setLogConstantSize( waslogconstsize );

    mDispLog( visBase::Well::Left, Left );
    mDispLog( visBase::Well::Right, Right );
}


#define mErrRet(s) { errmsg_ = s; return false; }
bool WellDisplay::setMultiID( const MultiID& multiid )
{
    Well::Data* oldwd = getWD();
    if ( oldwd )
	Well::MGR().release( wellid_ );

    wellid_ = multiid; wd_ = 0;
    mGetWD(return false);

    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	if ( !wd->markers()[idx] ) continue;

	const char* mrkrnm = wd->markers()[idx]->name();
	wd->displayProperties().markers_.selmarkernms_.add( mrkrnm );
    }

    const Well::D2TModel* d2t = wd->d2TModel();
    if ( zistime_ )
    {
	if ( !d2t )
	    mErrRet( "No depth to time model defined" )
    }

    wellid_ = multiid;
    fullRedraw(0);

    if ( basemapobj_ )
	basemapobj_->updateGeometry();

    return true;
}


void WellDisplay::getTrackPos( const Well::Data* wd,
			       TypeSet<Coord3>& trackpos )
{
    trackpos.erase();
    const Well::D2TModel* d2t = wd->d2TModel();
    setName( wd->name() );

    if ( wd->track().size() < 1 )
	return;

    PtrMan<Well::Track> ttrack = 0;
    if ( zistime_ )
    {
	mTryAlloc( ttrack, Well::Track( wd->track() ) );
	ttrack->toTime( *d2t , wd->track() );
    }
    const Well::Track& track = zistime_ ? *ttrack : wd->track();

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
    well_->removeAllMarkers();
    mGetWD(return);

    visBase::Well::MarkerParams mp;
    fillMarkerParams( mp );
    well_->setMarkerSetParams( mp );

    const BufferStringSet selnms(
		wd->displayProperties(false).markers_.selmarkernms_ );
    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	Well::Marker* wellmarker = wd->markers()[idx];
	if ( !selnms.isPresent( wellmarker->name() ) )
	    continue;

	Coord3 pos = wd->track().getPos( wellmarker->dah() );
	if ( !pos.x && !pos.y && !pos.z ) continue;

	if ( zistime_ )
	    pos.z = wd->d2TModel()->getTime( wellmarker->dah(), wd->track() );

	if ( mIsUdf( pos.z ) )
	    continue;

	mp.pos_ = &pos;	mp.name_ = wellmarker->name();

	if ( !mGetDispPar( markers_.issinglecol_ ) )
	    mp.col_ = wellmarker->color();
	if ( mGetDispPar( markers_.samenmcol_ ) ) mp.namecol_  = mp.col_;

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


const LineStyle* WellDisplay::lineStyle() const
{
    return &well_->lineStyle();
}


void WellDisplay::setLineStyle( const LineStyle& lst )
{
    well_->setLineStyle( lst );
}


#define getminVal(minval,val)  val < minval ? val : minval
#define getmaxVal(maxval,val)  val > maxval ? val : maxval

void WellDisplay::setLogData( visBase::Well::LogParams& lp, bool isfilled )
{
    mGetWD(return);

    Well::Log& wlF = wd->logs().getLog( lp.filllogidx_ );
    Well::Log& wl  = wd->logs().getLog( lp.logidx_ );

    if ( wl.isEmpty() ) return;
    const int logsz = wl.size();
    const int logszf = wlF.size();
    const int longSize = logsz >= logszf ? logsz : logszf;

    const Well::Track& track = wd->track();
    float minval=mUdf(float), maxval=-mUdf(float);
    float minvalF=mUdf(float), maxvalF=-mUdf(float);

    TypeSet<visBase::Well::Coord3Value> crdvals;
    TypeSet<visBase::Well::Coord3Value> crdvalsF;

    for ( int idx=0; idx<longSize; idx++ )
    {
	if( idx < logsz )
	{
	    const float dah = wl.dah(idx);
	    Coord3 pos = track.getPos( dah );
	    if ( !pos.x && !pos.y && !pos.z )
		continue;
	    if ( zistime_ )
		pos.z = wd->d2TModel()->getTime( dah, wd->track() );
	    if ( mIsUdf( pos.z ) )
		continue;
	    float val = wl.value(idx);
	    if ( mIsUdf(val) )
		continue;
	    val = lp.range_.limitValue( val );
	    minval = getminVal(minval,val);
	    maxval = getmaxVal(maxval,val);
	    crdvals += visBase::Well::Coord3Value( pos, val );
	}

	if( isfilled && logszf !=0 && idx < logszf )
	{
	    const float dah = wlF.dah(idx);
	    Coord3 pos = track.getPos( dah );
	    if ( !pos.x && !pos.y && !pos.z )
		continue;
	    if ( zistime_ )
		pos.z = wd->d2TModel()->getTime( dah, wd->track() );
	    if ( mIsUdf( pos.z ) )
		continue;
	    float valF = wlF.value(idx);
	    if ( mIsUdf(valF) )
		continue;
	    minvalF = getminVal(minvalF,valF);
	    maxvalF = getmaxVal(maxvalF,valF);
	    crdvalsF += visBase::Well::Coord3Value( pos, valF );
	}

    }

    if ( crdvals.isEmpty() && crdvalsF.isEmpty() )
	return;

    lp.valfillrange_ .set( minvalF, maxvalF );
    lp.valrange_.set( minval, maxval );
    well_->setLogData( crdvals,crdvalsF,lp,isfilled );

}

void WellDisplay::setLogDisplay( visBase::Well::Side side )
{
    mGetWD(return);

    BufferString& logname = mGetLogPar( side, name_);
    if ( wd->logs().isEmpty() ) return;
    const int logidx = wd->logs().indexOf( logname );
    if( logidx<0 )
    {
	well_->clearLog( side );
	well_->showLog( false, side );
	return;
    }

    visBase::Well::LogParams lp;
    fillLogParams( lp, side );
    lp.logidx_ = logidx;  lp.side_ = side;
    setLogProperties( lp );

    lp.filllogidx_ = wd->logs().indexOf( lp.fillname_ );
    bool beFilled =( ( lp.isleftfilled_ || lp.isrightfilled_ ) &&
			    lp.filllogidx_  >= 0 ) ? true : false;

    setLogData( lp, beFilled );
}


void WellDisplay::displayRightLog()
{ setLogDisplay( visBase::Well::Right ); }


void WellDisplay::displayLeftLog()
{ setLogDisplay( visBase::Well::Left ); }


void WellDisplay::setOneLogDisplayed(bool yn)
{ onelogdisplayed_ = yn; }


void WellDisplay::setLogConstantSize(bool yn)
{ well_->setLogConstantSize( yn ); }


bool WellDisplay::logConstantSize() const
{ return well_->logConstantSize(); }


void WellDisplay::setLogProperties( visBase::Well::LogParams& lp )
{
    const visBase::Well::Side side = lp.side_;

    well_->setLogStyle( lp.style_, side );

    well_->setOverlapp( lp.ovlap_, side );
    well_->setRepeat( lp.repeat_,side );
    well_->setLogFill( ( lp.isleftfilled_ || lp.isrightfilled_ ), side );
    well_->setLogFillColorTab( lp, side );
    well_->setLogLineDisplayed( lp.size_ > 0, side );

    well_->setLogColor( lp.col_, side );
    well_->setLogLineWidth( lp.size_, side );
    well_->setLogScreenWidth( mCast(float,lp.logwidth_), side );

    if ( lp.cliprate_ && lp.logidx_ >= 0 )
	calcClippedRange( lp.cliprate_, lp.range_, lp.logidx_ );
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


const Color& WellDisplay::logColor( visBase::Well::Side side ) const
{ return well_->logColor( side ); }


void WellDisplay::setLogColor( const Color& col, visBase::Well::Side side )
{ well_->setLogColor( col, side ); }


float WellDisplay::getLogScreenWidth( visBase::Well::Side side ) const
{ return well_->getLogScreenWidth( side ); }


void WellDisplay::setLogScreenWidth( float width, visBase::Well::Side side )
{ well_->setLogScreenWidth( width, side ); }


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

    PtrMan<Well::Track> ttrack = 0;
    if ( zistime_ && wd->haveD2TModel() )
    {
	mTryAlloc( ttrack, Well::Track( wd->track() ) );
	ttrack->toTime( *wd->d2TModel(), wd->track() );
    }
    const Well::Track& track = zistime_ ? *ttrack : wd->track();

    info = "Well: "; info += wd->name();
    info += ", MD ";

    info += zinfeet_ || SI().depthsInFeet() ? "(ft): " : "(m): ";
    const float zfac = SI().depthsInFeet() && SI().zIsTime() ?
							mToFeetFactorF : 1;
    const float dah = track.nearestDah(pos);
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

    const visBase::Well::Side side  =
			isleft ? visBase::Well::Left : visBase::Well::Right;

    BufferString lognm( mGetLogPar( side , name_ ) );
    if ( !lognm.isEmpty() && !lognm.isEqual("None") && !lognm.isEqual("none") )
    {
	info += isleft ? ", Left: " : ", Right: ";
	info += lognm;
	const Well::Log* log = wd->logs().getLog( lognm );
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
    well_->setDisplayTransformation( nt );
    setDisplayTransformForPicks( nt );
    fullRedraw(0);
}


const mVisTrans* WellDisplay::getDisplayTransformation() const
{ return well_->getDisplayTransformation(); }


void WellDisplay::pickCB( CallBacker* cb )
{
    if ( !isSelected() || !picksallowed_ || !markerset_ || isLocked() ) return;

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
	mousepressposition_ = eventid==-1
	    ? Coord3::udf() : eventinfo.displaypickedpos;
	eventcatcher_->setHandled();
    }
    else
    {
	if ( OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::altKeyboardButton(eventinfo.buttonstate_) &&
	     !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
	{
	    const float zfactor = scene_ ? scene_->getZScale(): SI().zScale();
	    if ( eventinfo.pickedobjids.size() && eventid==mousepressid_ )
	    {
		if ( mousepressid_!= -1 )
		{
		    markerset_->removeMarker( mousepressid_ );
		    pseudotrack_->removePoint( mousepressid_ );
		    TypeSet<Coord3> wcoords;
		    for ( int idx=0; idx<pseudotrack_->nrPoints(); idx++ )
		    {
			wcoords += pseudotrack_->pos(idx);
			wcoords[idx].z /= zfactor;
		    }

		    well_->setTrack(wcoords);
		    needsave_ = true;
		    changed_.trigger();
		}
	    }

	    eventcatcher_->setHandled();
	}
	else if ( !OD::ctrlKeyboardButton(eventinfo.buttonstate_) &&
		  !OD::altKeyboardButton(eventinfo.buttonstate_) &&
		  !OD::shiftKeyboardButton(eventinfo.buttonstate_) )
	{
	    if ( eventinfo.pickedobjids.size() && eventid==mousepressid_ )
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

		if ( validpicksurface )
		{
		    Coord3 newpos = eventinfo.worldpickedpos;
		    mDynamicCastGet(SurveyObject*,so,
				    visBase::DM().getObject(eventid))
		    if ( so ) so->snapToTracePos( newpos );
		    addPick( newpos );
		}
	    }

	    eventcatcher_->setHandled();
	}
    }
}


void WellDisplay::addPick( Coord3 pos )
{
    int insertidx = -1;
    if ( pseudotrack_ )
    {
	TypeSet<Coord3> wcoords;
	const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
	insertidx = pseudotrack_->insertPoint( Coord(pos.x, pos.y),
					       (float) (pos.z * zfactor)  );
	for ( int idx=0; idx<pseudotrack_->nrPoints(); idx++ )
	{
	    wcoords += pseudotrack_->pos(idx);
	    wcoords[idx].z /= zfactor;
	}

	well_->setTrack(wcoords);
	needsave_ = true;
	changed_.trigger();
    }

    if ( insertidx > -1 )
    {
	const int markerid = markerset_->getCoordinates()->addPos( pos );
	markerset_->getMaterial()->setColor( lineStyle()->color_,markerid ) ;
    }
}


void WellDisplay::setDisplayTransformForPicks( const mVisTrans* newtr )
{
    if ( transformation_==newtr || !markerset_ )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = newtr;

    if ( transformation_ )
	transformation_->ref();

    markerset_->setDisplayTransformation( transformation_ );
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
	markerset_->ref();
	addChild( markerset_->osgNode() );
	markerset_->setMaterial( new visBase::Material );
	mTryAlloc( pseudotrack_, Well::Track() );
    }

    markerset_->turnOn( yn );

}


void WellDisplay::showKnownPositions()
{
    mGetWD(return);

    TypeSet<Coord3> trackpos;
    getTrackPos( wd, trackpos );
    if ( trackpos.isEmpty() )
	return;

    for ( int idx=0; idx<trackpos.size(); idx++ )
	addPick( trackpos[idx] );
}


TypeSet<Coord3> WellDisplay::getWellCoords() const
{
    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();

    TypeSet<Coord3> coords;
    for ( int idx=0; idx<pseudotrack_->nrPoints(); idx++ )
    {
	coords += pseudotrack_->pos(idx);
	coords[idx].z /= zfactor;
    }

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

} // namespace visSurvey

