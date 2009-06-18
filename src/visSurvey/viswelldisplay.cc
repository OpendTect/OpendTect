/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: viswelldisplay.cc,v 1.99 2009-06-18 14:54:09 cvsbert Exp $";

#include "viswelldisplay.h"

#include "dataclipper.h"
#include "draw.h"
#include "iopar.h"
#include "executor.h"
#include "ptrman.h"
#include "survinfo.h"
#include "coltabsequence.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "viswell.h"

#include "welldisp.h"
#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"
#include "welldata.h"
#include "welltransl.h"
#include "welltrack.h"
#include "wellmarker.h"
#include "welld2tmodel.h"

#define		mPickSz 	3
#define         mPickType	3 


mCreateFactoryEntry( visSurvey::WellDisplay );

namespace visSurvey
{

const char* WellDisplay::sKeyEarthModelID      = "EarthModel ID";
const char* WellDisplay::sKeyWellID	       = "Well ID";
const char* WellDisplay::sKeyLog1Name	       = "Logname 1";
const char* WellDisplay::sKeyLog2Name	       = "Logname 2";
const char* WellDisplay::sKeyLog1Range	       = "Logrange 1";
const char* WellDisplay::sKeyLog2Range	       = "Logrange 2";
const char* WellDisplay::sKeyLog1Scale	       = "Loglogsc 1";
const char* WellDisplay::sKeyLog2Scale	       = "Loglogsc 2";
const char* WellDisplay::sKeyLog1Repeat	       = "Logrepeat 1";
const char* WellDisplay::sKeyLog2Repeat	       = "Logrepeat 2";
const char* WellDisplay::sKeyLog1Color	       = "Logcolor 1";
const char* WellDisplay::sKeyLog2Color	       = "Logcolor 2";
const char* WellDisplay::sKeyLog1FillColor     = "Logfillcolor 1";
const char* WellDisplay::sKeyLog2FillColor     = "Logfillcolor 2";
const char* WellDisplay::sKeyLog1SeisFillColor = "Seisfillcolor 1";
const char* WellDisplay::sKeyLog2SeisFillColor = "Seisfillcolor 2";
const char* WellDisplay::sKeyLog1Style	       = "Logstyle 1";
const char* WellDisplay::sKeyLog2Style	       = "Logstyle 2";
const char* WellDisplay::sKeyLog1Ovlap	       = "Logovlap 1";
const char* WellDisplay::sKeyLog2Ovlap	       = "Logovlap 2";
const char* WellDisplay::sKeyLog1Clip	       = "Cliprate 1";
const char* WellDisplay::sKeyLog2Clip	       = "Cliprate 2";

#define mMeter2Feet(val) \
   val *= mToFeetFactor;

#define dpp(param) wd->displayProperties().param

WellDisplay::WellDisplay()
    : VisualObjectImpl(true)
    , well_(0)
    , wellid_(-1)
    , zistime_(SI().zIsTime())
    , zinfeet_(SI().zInFeet())
    , eventcatcher_(0)
    , changed_(this)
    , transformation_(0)
    , picksallowed_(false)
    , group_(0)
    , pseudotrack_(0)
    , logparset_(*new Well::LogDisplayParSet)
    , needsave_(false)
{
    setMaterial(0);
    setWell( visBase::Well::create() );
}


WellDisplay::~WellDisplay()
{
    removeChild( well_->getInventorNode() );
    well_->unRef();
    setSceneEventCatcher(0);
    if ( transformation_ ) transformation_->unRef();
    if ( group_ )
	removeChild( group_->getInventorNode() );
    delete &logparset_;
}


void WellDisplay::setWell( visBase::Well* well )
{
    if ( well_ )
    {
	removeChild( well_->getInventorNode() );
	well_->unRef();
    }

    well_ = well;    
    well_->ref();
    addChild( well_->getInventorNode() );
}


void WellDisplay::fullRedraw( CallBacker* )
{
    if ( wellid_ < 0 ) return;
    Well::Data* wd = Well::MGR().get( wellid_, false );
    if ( !wd ) return;
   
    TypeSet<Coord3> trackpos = getTrackPos( wd );
    if ( trackpos.isEmpty() )
	return;

    well_->setTrack( trackpos );
    Color& tcolor = dpp(track_.color_);
    int twidth = dpp(track_.size_);
    well_->setTrackProperties( tcolor, twidth );
    well_->setWellName( wd->name(), trackpos[0], trackpos[trackpos.size()-1],
	   		 dpp( track_.dispabove_), dpp( track_.dispbelow_),
	                 dpp( track_.nmsize_ ) ); 
    updateMarkers(0);
   
    BufferString leftname = dpp( left_.name_ );  
    BufferString rightname = dpp( right_.name_ );  
    logsnumber_ = 0;

    if ( leftname.size() )
    {
	int repeatl = dpp( left_.repeat_);
	logsnumber_ = logsnumber_ >repeatl ? logsnumber_ : repeatl ;
	displayLeftLog();
    }

    if ( rightname.size() )
    {
	int repeatr = dpp( right_.repeat_);
	logsnumber_ = logsnumber_ >repeatr ? logsnumber_ : repeatr ;
	displayRightLog();
    }
}


TypeSet<Coord3> WellDisplay::getTrackPos( Well::Data* wd )
{
    TypeSet<Coord3> trackpos;
    const Well::D2TModel* d2t = wd->d2TModel();
    setName( wd->name() );

    if ( wd->track().size() < 1 ) return trackpos;
    PtrMan<Well::Track> ttrack = 0;
    if ( zistime_ )
    {
	mTryAlloc( ttrack, Well::Track( wd->track() ) );
	ttrack->toTime( *d2t );
    }
    Well::Track& track = zistime_ ? *ttrack : wd->track();

    Coord3 pt;
    for ( int idx=0; idx<track.size(); idx++ )
    {
	pt = track.pos( idx );
	if ( zinfeet_ )
	    mMeter2Feet(pt.z);

	if ( !mIsUdf(pt.z) )
	    trackpos += pt;
    }

    return trackpos;
}


#define mErrRet(s) { errmsg = s; return false; }

bool WellDisplay::setMultiID( const MultiID& multiid )
{
    Well::Data* wd = Well::MGR().get( multiid, true );
    if ( !wd ) return false;
    const Well::D2TModel* d2t = wd->d2TModel();
    if ( zistime_ )
    {
	if ( !d2t )
	    mErrRet( "No depth to time model defined" )
	wd->d2tchanged.notify( mCB(this,WellDisplay,fullRedraw) );
    }

    wellid_ = multiid;
    fullRedraw(0);
    wd->trackchanged.notify( mCB(this,WellDisplay,fullRedraw) );
    wd->markerschanged.notify( mCB(this,WellDisplay,updateMarkers) );
    wd->dispparschanged.notify( mCB(this,WellDisplay,fullRedraw) );

    return true;
}


Color WellDisplay::getColor() const
{
    return well_->lineStyle().color_;
}


const LineStyle* WellDisplay::lineStyle() const
{
    return &well_->lineStyle();
}


void WellDisplay::setLineStyle( LineStyle lst )
{
    well_->setLineStyle( lst );
}


void WellDisplay::updateMarkers( CallBacker* )
{
    Well::Data* wd = Well::MGR().get( wellid_ );
    if ( !wd ) return;

    well_->removeAllMarkers();
    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	Well::Marker* wellmarker = wd->markers()[idx];
	Coord3 pos = wd->track().getPos( wellmarker->dah() );
	if ( !pos.x && !pos.y && !pos.z ) continue;

	if ( zistime_ )
	    pos.z = wd->d2TModel()->getTime( wellmarker->dah() );
	else if ( zinfeet_ )
	    mMeter2Feet(pos.z)

	well_->markersize = dpp(markers_.size_);
	bool iscircular = !(dpp( markers_.circular_ ));
	bool issinglemcol = dpp( markers_.issinglecol_);
	int nmsize = dpp(markers_.nmsize_); 
	Color mcolor = issinglemcol ? dpp(markers_.color_) : wellmarker->color();
	well_->addMarker( pos, mcolor, wellmarker->name(), iscircular, nmsize );
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
     


void WellDisplay::setWellData( const int logidx, TypeSet<Coord3Value>& crdvals, 			      Interval<float>* range, Interval<float>& selrange			       	      ,bool& logrthm, Well::Log& wl )
{ 
    if ( logidx < 0 ) { pErrMsg("Logidx < 0"); return;}
    Well::Data* wd = Well::MGR().get( wellid_ );
    if ( !wd || wd->logs().isEmpty() ) return;

    wl = wd->logs().getLog( logidx );
    if ( wl.isEmpty() ) return;
    const int logsz = wl.size();
  
    const Well::Track& track = wd->track();

    for ( int idx=0; idx<logsz; idx++ )
    {
	const float dah = wl.dah(idx);
	Coord3 pos = track.getPos( dah );
	if ( !pos.x && !pos.y && !pos.z ) continue;

	if ( zistime_ )
	    pos.z = wd->d2TModel()->getTime( dah );
	else if ( zinfeet_ )
	    mMeter2Feet(pos.z)
	
	Coord3Value cv( pos, wl.value(idx) );
	crdvals += cv;
     }

    if ( range )
	selrange = *range;
}


void WellDisplay::createLogDisplay( int logidx, Interval<float>* range,
			            bool logrthm, int lognr)
{
    Interval<float> selrange;
    TypeSet<Coord3Value> crdvals;
    Well::Log wl;
    setWellData( logidx, crdvals, range, selrange, logrthm, wl);
    well_->setLogData( crdvals, wl.name(), selrange, logrthm, lognr );
}

void WellDisplay::createFillLogDisplay( int logidx, Interval<float>* range,
			            bool logrthm, int lognr)
{
    Interval<float> selrange;
    TypeSet<Coord3Value> crdvals;
    Well::Log wl;
    setWellData( logidx, crdvals, range, selrange, logrthm, wl ) ;
    well_->setFillLogData( crdvals, wl.name(), selrange, logrthm, lognr );
}


void WellDisplay::displayLog( const BufferString lognm, bool logrthm,
		        	  Interval<float>& range, int lognr )
{
    Well::Data* wd = Well::MGR().get( wellid_ );
    if ( !wd || wd->logs().isEmpty() ) return;

    int logidx = wd->logs().indexOf( lognm );
    if ( logidx < 0 ) return; // TODO: errmsg
   
    mDeclareAndTryAlloc( Interval<float>*, rgptr, Interval<float>( range ) );
    createLogDisplay( lognr, rgptr, logidx, logrthm );
    createFillLogDisplay( lognr, rgptr, logidx, logrthm );
}


void WellDisplay::setLogDisplay( Well::LogDisplayPars& dp, int lognr )
{
    Well::Data* wd = Well::MGR().get( wellid_ );
    if ( !wd || wd->logs().isEmpty() ) return;
    
    BufferString logname ;
    BufferString fillname ;
    Interval<float> range;
    Interval<float> fillrg;
    int repeat;
    bool islog;
    logname = ( lognr == 1 ? dpp( left_.name_ ) : dpp( right_.name_ ) ); 
    fillname = ( lognr == 1 ? dpp( left_.fillname_ ):dpp( right_.fillname_ ) ); 
    range = ( lognr == 1 ? dpp( left_.range_ ) : dpp( right_.range_ ) );
    fillrg = ( lognr == 1 ? dpp( left_.fillrange_ ) :dpp( right_.fillrange_ ) );
    repeat = ( lognr == 1 ? dpp( left_.repeat_ ) : dpp( right_.repeat_ ) );
    islog = ( lognr == 1 ? dpp( left_.islogarithmic_ ) :
	    		   dpp( right_.islogarithmic_ ) );
    
    const int logidx = wd->logs().indexOf( logname );
    const int filllogidx = wd->logs().indexOf( fillname );
   
    if( logidx<0 )
    {
	well_->clearLog( lognr );
	well_->showLog( false, lognr );
	return;
    }
  
    setWellProperties( lognr, range );
    mDeclareAndTryAlloc( Interval<float>*,rgptr,
	    		 Interval<float>( range ) );
    mDeclareAndTryAlloc( Interval<float>*,fillrgptr,
	    		 Interval<float>( fillrg ) );
    createLogDisplay( logidx, rgptr, islog, lognr );
    createFillLogDisplay( filllogidx, fillrgptr, islog, lognr );
    well_->showLog( true, lognr );
    if (repeat < logsnumber_) well_->hideUnwantedLogs( lognr, repeat  );
}


void WellDisplay::calcClippedRange(float cliprate, Interval<float>& range,
       							Well::Log& wl )
{
    cliprate = cliprate / 100;
    if ( mIsUdf(cliprate) || cliprate < 0 ) cliprate = 0;
    if ( cliprate > 100 ) cliprate = 100;
    int logsz=wl.size();
    DataClipper dataclipper;
    dataclipper.setApproxNrValues( logsz );
    dataclipper.putData( wl.valArr(), logsz );
    dataclipper.calculateRange( cliprate, range );
}


void WellDisplay::displayRightLog()
{
    setLogDisplay( *logparset_.getRight(), 2 ); 
}


void WellDisplay::displayLeftLog()
{   
    setLogDisplay( *logparset_.getLeft(), 1 ); 
}


void WellDisplay::setOneLogDisplayed(bool yn)
{
    onelogdisplayed_ = yn; 
}


#define mSetLogProp(side) \
{ \
    logname = dpp( side.name_ );\
    bool iswelllog = dpp(side.iswelllog_);\
    const char* seqname = dpp( side.seqname_);\
    int repeat = dpp( side.repeat_);\
    int logwidth = dpp( side.logwidth_);\
    float ovlap = dpp( side.repeatovlap_ );\
    Color& lcolor = dpp( side.color_);\
    Color& seiscolor = dpp( side.seiscolor_);\
    bool isfilled = dpp(side.islogfill_);\
    bool issinglefill = dpp(side.issinglecol_);\
    isdatarange = dpp(side.isdatarange_);\
    cliprate = dpp( side.cliprate_ );\
    if (iswelllog) \
	repeat = 1; \
    well_->removeLog( logsnumber_, lognr );\
    well_->setRepeat( logsnumber_ );\
    well_->setOverlapp( ovlap, lognr );\
    well_->setLogStyle( iswelllog, lognr ); \
    well_->setLogFill( isfilled, lognr ); \
    setLogColor( lcolor, lognr ); \
    setLogLineWidth( dpp(side.size_), lognr );\
    setLogWidth( logwidth, lognr );\
    setLogFillColor( seiscolor, lognr, seqname, iswelllog, issinglefill );\
}


void WellDisplay::setWellProperties( int lognr, Interval<float>& range)
{  
    BufferString logname;
    Well::Data* wd = Well::MGR().get( wellid_ );
    float cliprate;
    bool isdatarange;
    if ( lognr == 1 )  
	mSetLogProp( left_ ) 
    else
	mSetLogProp( right_ )
    
    const int logidx = wd->logs().indexOf( logname );
    if ( !isdatarange && logidx >= 0 ) 
    {
	Well::Log& wl = wd->logs().getLog( logidx );
	calcClippedRange( cliprate, range, wl );
	if ( lognr == 1)
	    dpp( left_.range_) = range; 
	else
	    dpp( right_.range_) = range;
    }
}


void WellDisplay::setLogColor( const Color& col, int lognr )
{
    Well::LogDisplayPars* par = lognr==1 ? logparset_.getLeft()
					 : logparset_.getRight();
    well_->setLogColor( col, lognr );
}


const Color& WellDisplay::logColor( int lognr ) const
{ return well_->logColor( lognr ); }


void WellDisplay::setLogFillColor(const Color& color, int lognr, 
				  const char* seqname, const bool iswelllog, 
				  const bool issinglecol)
{
    Well::LogDisplayPars* par = lognr==1 ? logparset_.getLeft()
					 : logparset_.getRight();
    well_->setLogFillColorTab(seqname, lognr, color, iswelllog, issinglecol); 
}


void WellDisplay::setLogLineWidth( float width, int lognr )
{ well_->setLogLineWidth( width, lognr ); }


float WellDisplay::logLineWidth( int lognr ) const
{ return well_->logLineWidth( lognr ); }


void WellDisplay::setLogWidth( int width, int lognr )
{ well_->setLogWidth( width, lognr ); }


int WellDisplay::logWidth() const
{ return well_->logWidth(); }


bool WellDisplay::logsShown() const
{
    return well_->logsShown();
}


void WellDisplay::showLogs( bool yn )
{
    well_->showLog( yn && !(logparset_.getLeft()->name_="None"), 1);
    well_->showLog( yn && !(logparset_.getRight()->name_="None"), 2);
}


void WellDisplay::getMousePosInfo( const visBase::EventInfo&,
				   const Coord3& pos,
				   BufferString& val,
				   BufferString& info ) const
{
    val = "";
    Well::Data* wd = Well::MGR().get( wellid_ );
    if ( !wd ) { info = ""; return; }

    const float zfactor = scene_ ? scene_->getZScale() : SI().zScale();
    const float mousez = pos.z * zfactor;
    const float zstep2 = zfactor * SI().zStep()/2;

    info = "Well: "; info += wd->name();
    for ( int idx=0; idx<wd->markers().size(); idx++ )
    {
	Well::Marker* wellmarker = wd->markers()[idx];
	Coord3 markerpos = wd->track().getPos( wellmarker->dah() );
	if ( !mIsEqual(markerpos.z,mousez,zstep2) )
	    continue;

	info += ", Marker: ";
	info += wellmarker->name();
	break;
    }
}


void WellDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyEarthModelID, wellid_ );

    const int viswellid = well_->id();
    par.set( sKeyWellID, viswellid );
    if ( saveids.indexOf(viswellid) == -1 ) saveids += viswellid;
    
    fillSOPar( par );
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
	mDynamicCastGet(visBase::Well*,well,dataobj)
	if ( !well ) return -1;
	setWell( well );
    }
    else
    {
	setWell( visBase::Well::create() );
	viswellid = well_->id();
    }
    
    MultiID newmid;
    if ( !par.get(sKeyEarthModelID,newmid) )
	return -1;

    if ( !setMultiID(newmid) )
    {
	return 1;
    }
    
    BufferString logname;
    Color col;
    Interval<float> rg;
    bool logsc;
    bool stl;
    displayLeftLog();
    displayRightLog();

// Support for old sessions
    BufferString linestyle;
    if ( par.get(visBase::Well::linestylestr(),linestyle) )
    {
	LineStyle lst;
	lst.fromString( linestyle );
	setLineStyle( lst );
    }

    bool wellnmshown;
    if ( par.getYN(visBase::Well::showwelltopnmstr(),wellnmshown) )
	showWellTopName( wellnmshown );
    if ( par.getYN(visBase::Well::showwellbotnmstr(),wellnmshown) )
	showWellBotName( wellnmshown );

    useSOPar( par );
    return 1;
}


void WellDisplay::setDisplayTransformation( visBase::Transformation* nt )
{
    well_->setDisplayTransformation( nt );
    setDisplayTransformForPicks( nt );
    fullRedraw(0);
}


visBase::Transformation* WellDisplay::getDisplayTransformation()
{ return well_->getDisplayTransformation(); }


void WellDisplay::pickCB( CallBacker* cb )
{
    if ( !isSelected() || !picksallowed_ || !group_ || isLocked() ) return;

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
		int removeidx = group_->getFirstIdx(mousepressid_);
		if ( removeidx != -1 )
		{
		    group_->removeObject( removeidx );
		    pseudotrack_->removePoint( removeidx );
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
		    if ( so && so->allowPicks() )
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
					       pos.z * zfactor  );
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
	visBase::Marker* marker = visBase::Marker::create();
	group_->insertObject( insertidx, marker );

	marker->setDisplayTransformation( transformation_ );
	marker->setCenterPos( pos );
        marker->setScreenSize( mPickSz );
	marker->setType( (MarkerStyle3D::Type)mPickSz );
	marker->getMaterial()->setColor( lineStyle()->color_ );
    }
}


void WellDisplay::setDisplayTransformForPicks( visBase::Transformation* newtr )
{
    if ( transformation_==newtr )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = newtr;

    if ( transformation_ )
	transformation_->ref();

    if ( !group_ ) return;
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,group_->getObject(idx));
	marker->setDisplayTransformation( transformation_ );
    }
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
    if ( !group_ )
    {
	group_ = visBase::DataObjectGroup::create();
	mTryAlloc( pseudotrack_, Well::Track() );
	addChild( group_->getInventorNode() );
    }

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,group_->getObject(idx));
	marker->turnOn( yn );
    }
}


void WellDisplay::showKnownPositions()
{
    Well::Data* wd = Well::MGR().get( wellid_, false );
    if ( !wd ) return;
   
    TypeSet<Coord3> trackpos = getTrackPos( wd );
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

}; // namespace visSurvey
