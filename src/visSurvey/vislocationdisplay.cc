/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vislocationdisplay.h"

#include "draw.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "pickset.h"
#include "picksettr.h"
#include "selector.h"
#include "separstr.h"
#include "survinfo.h"
#include "viscoord.h"
#include "visevent.h"
#include "visdataman.h"
#include "vismaterial.h"
#include "vispickstyle.h"
#include "vispolyline.h"
#include "vissower.h"
#include "vistransform.h"
#include "zaxistransform.h"


namespace visSurvey {

const char* LocationDisplay::sKeyID()		{ return "Location.ID"; }
const char* LocationDisplay::sKeyMgrName()	{ return "Location.Manager"; }
const char* LocationDisplay::sKeyShowAll()	{ return "Show all"; }
const char* LocationDisplay::sKeyMarkerType()	{ return "Shape"; }
const char* LocationDisplay::sKeyMarkerSize()	{ return "Size"; }

static const float cDistEps = 0.1f;

static float findDistance( Coord3 p1, Coord3 p2, Coord3 p )
{
    const Coord3 vec = p2 - p1;
    const Coord3 newvec = p - p1;
    const float prod = (float) vec.dot(newvec);
    const float sq = (float) vec.sqAbs();
    if ( mIsZero(sq,cDistEps) ) return mUdf(float);	// p1 and p2 coincide.

    const float factor = prod / sq;
    if ( factor<0 || factor>1 )		// projected point outside the segment.
	return (float) mMIN( p1.distTo(p), p2.distTo(p) );

    const Coord3 proj = p1 + vec * factor;
    return (float) proj.distTo( p );
}


LocationDisplay::LocationDisplay()
    : VisualObjectImpl( true )
    , group_( visBase::DataObjectGroup::create() )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , showall_( true )
    , set_( 0 )
    , manip_( this )
    , picksetmgr_( 0 )
    , waitsfordirectionid_( -1 )
    , waitsforpositionid_( -1 )
    , datatransform_( 0 )
    , pickstyle_( 0 )
    , polyline_(0)
    , needline_(false)
    , pickedsobjid_(-1)
    , voiidx_(-1)
{
    group_->ref();
    addChild( group_->getInventorNode() );

    setSetMgr( &Pick::Mgr() );

    sower_ = new Sower( this );
    addChild( sower_->getInventorNode() );
}
    

LocationDisplay::~LocationDisplay()
{
    setSceneEventCatcher( 0 );
    removeChild( group_->getInventorNode() );
    group_->unRef();

    if ( polyline_ )
    {
	removeChild( polyline_->getInventorNode() );
	polyline_->unRef();
    }

    if ( transformation_ ) transformation_->unRef();
    setSetMgr( 0 );

    if ( pickstyle_ ) pickstyle_->unRef();

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB( this, LocationDisplay, fullRedraw) );
	datatransform_->unRef();
    }

    removeChild( sower_->getInventorNode() );
    delete sower_;
}


void LocationDisplay::setSet( Pick::Set* s )
{
    if ( set_ )
    {
	if ( set_!=s )
	    pErrMsg("Cannot set set_ twice");
	return;
    }

    set_ = s; 
    setName( set_->name() );
    fullRedraw();

    if ( !showall_ && scene_ )
	scene_->objectMoved( 0 );
}


void LocationDisplay::setSetMgr( Pick::SetMgr* mgr )
{
    if ( picksetmgr_ )
	picksetmgr_->removeCBs( this );

    picksetmgr_ = mgr;

    if ( picksetmgr_ )
    {
	picksetmgr_->locationChanged.notify( mCB(this,LocationDisplay,locChg) );
	picksetmgr_->setChanged.notify( mCB(this,LocationDisplay,setChg) );
	picksetmgr_->setDispChanged.notify( mCB(this,LocationDisplay,dispChg) );
    }
}


void LocationDisplay::fullRedraw( CallBacker* )
{
    if ( !set_ ) return;
    

    if ( datatransform_ && datatransform_->needsVolumeOfInterest() )
    {
	CubeSampling cs( false );
	for ( int pidx=0; pidx<set_->size(); pidx++ )
	{
	    Pick::Location loc = (*set_)[pidx];
	    BinID bid = SI().transform( loc.pos );
	    const float zval = loc.pos.z;
	    cs.hrg.include( bid );
	    cs.zrg.include( zval, false );
	}

	if ( voiidx_<0 )
	    voiidx_ = datatransform_->addVolumeOfInterest( cs, true );
	else
	    datatransform_->setVolumeOfInterest( voiidx_, cs, true );

	datatransform_->loadDataIfMissing( voiidx_ );
    }
    
    getMaterial()->setColor( set_->disp_.color_ );
    const int nrpicks = set_->size();

    invalidpicks_.erase();

    int idx=0;
    for ( ; idx<nrpicks; idx++ )
    {
	bool turnon = true;
	Pick::Location loc = (*set_)[idx];
	if ( !transformPos( loc ) )
	{
	    invalidpicks_ += idx;
	    turnon = false;
	}
	else
	{
	    invalidpicks_ -= idx;
	}

	if ( idx<group_->size() )
	    setPosition( idx, loc );
	else
	    addDisplayPick( loc, group_->size() );

	mDynamicCastGet( visBase::VisualObject*, vo,
			 group_->getObject( idx ) );
	if ( vo ) vo->turnOn( turnon );
    }

    while ( idx<group_->size() )
	group_->removeObject( idx );

    showLine( set_->disp_.connect_ != Pick::Set::Disp::None );
}


void LocationDisplay::showAll( bool yn )
{
    showall_ = yn;
    if ( !showall_ && scene_ )
    {
	scene_->objectMoved(0);
	return;
    }

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::VisualObject*, vo, group_->getObject( idx ) );
	if ( !vo ) continue;

	vo->turnOn( true );
    }
}


void LocationDisplay::createLine()
{
    if ( !polyline_ ) 
    {
	polyline_ = visBase::PolyLine::create();
	addChild( polyline_->getInventorNode() );
	polyline_->setDisplayTransformation( transformation_ );
	polyline_->setMaterial( 0 );
	polyline_->ref();
    }

    int pixsize = set_->disp_.pixsize_;
    LineStyle ls;
    ls.width_ = pixsize;
    polyline_->setLineStyle( ls );
    while ( polyline_->size() > set_->size() )
	polyline_->removePoint( 0 );

    for ( int idx=0; idx<set_->size(); idx++ )
    {
	Coord3 pos = (*set_)[idx].pos;
	if ( datatransform_ )
	    pos.z = datatransform_->transform( pos );
	if ( !mIsUdf(pos.z) )
	    polyline_->setPoint( idx, pos );
    }

    int nrnodes = polyline_->size();
    if ( nrnodes && set_->disp_.connect_==Pick::Set::Disp::Close ) 
	polyline_->setPoint( nrnodes, polyline_->getPoint(0) );
} 


void LocationDisplay::showLine( bool yn )
{
    if ( yn ) needline_ = true;
    if ( !needline_ ) return;
    if ( !polyline_ || polyline_->size() <= set_->size() ) createLine();
    polyline_->turnOn( yn );
}


bool LocationDisplay::lineShown() const
{
    return polyline_ ? polyline_->isOn() : false;
}


void LocationDisplay::pickCB( CallBacker* cb )
{
    if ( !isSelected() || !isOn() || isLocked() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    const bool sowerenabled = set_->disp_.connect_ != Pick::Set::Disp::None;

    if ( waitsfordirectionid_!=-1 )
    {
	Coord3 newpos, normal;
	if ( getPickSurface(eventinfo,newpos,normal) )
	{
	    Coord3 dir = newpos - (*set_)[waitsfordirectionid_].pos;
	    const float zscale = scene_ ? scene_->getZScale(): SI().zScale();
	    dir.z *= -zscale; //convert to right dir-domain
	    if ( dir.sqAbs()>=0 )
	    {
		 (*set_)[waitsfordirectionid_].dir =
		     cartesian2Spherical( dir, true );
		Pick::SetMgr::ChangeData cd(
			Pick::SetMgr::ChangeData::Changed,
			set_, waitsfordirectionid_ );
		picksetmgr_->reportChange( 0, cd );
	    }
	}

	eventcatcher_->setHandled();
    }
    else if ( waitsforpositionid_!=-1 )
    {
	Coord3 newpos, normal;
	if ( getPickSurface(eventinfo,newpos,normal) )
	{
	    (*set_)[waitsforpositionid_].pos = newpos;
	    Pick::SetMgr::ChangeData cd(
		    Pick::SetMgr::ChangeData::Changed,
		    set_, waitsforpositionid_ );
	    picksetmgr_->reportChange( 0, cd );
	}

	eventcatcher_->setHandled();
    }
    else if ( sowerenabled && sower_->accept(eventinfo) )
	return;

    if ( eventinfo.type != visBase::MouseClick ||
	 !OD::leftMouseButton( eventinfo.buttonstate_ ) )
	return;

    int eventid = -1;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
	    		visBase::DM().getObject( eventinfo.pickedobjids[idx] );
	if ( dataobj == this )
	    continue;

	if ( dataobj->pickable() )
	    eventid = eventinfo.pickedobjids[idx];

	mDynamicCastGet(const SurveyObject*,so,dataobj);
	if ( so && so->allowsPicks() )
	    pickedsobjid_ = eventid;

	if ( eventid!=-1 )
	    break;
    }

    if ( eventid==-1 )
	return;

    if ( waitsforpositionid_!=-1 || waitsfordirectionid_!=-1 )
    {
	setUnpickable( false );
	waitsforpositionid_ = -1;
	waitsfordirectionid_ = -1;
	mousepressid_ = -1;
    }
    else if ( eventinfo.pressed )
    {
	mousepressid_ = eventid;
	if ( !OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    const int selfpickidx = isMarkerClick( eventinfo.pickedobjids );
	    if ( selfpickidx!=-1 )
	    {
		setUnpickable( true );
		waitsforpositionid_ = selfpickidx;
	    }
	    const int selfdirpickidx = isDirMarkerClick(eventinfo.pickedobjids);
	    if ( selfdirpickidx!=-1 )
	    {
		setUnpickable( true );
		waitsfordirectionid_ = selfpickidx;
	    }

	    //Only set handled if clicked on marker. Otherwise
	    //we may interfere with draggers.
	    if ( selfdirpickidx!=-1 || selfpickidx!=-1 )
		eventcatcher_->setHandled();
	    else
	    {
		const Color& color = set_->disp_.color_;
		if ( sowerenabled && sower_->activate(color, eventinfo) )
		    return;
	    }
	}
    }
    else
    {
	if ( OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid_ )
	    {
		const int removeidx = isMarkerClick( eventinfo.pickedobjids );
		if ( removeidx!=-1 ) removePick( removeidx );
	    }

	    eventcatcher_->setHandled();
	}
	else if ( !OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	          !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
		  !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid_ )
	    {
		Coord3 newpos, normal;
		if ( getPickSurface(eventinfo,newpos,normal) )
		{
		    const Sphere dir = normal.isDefined()
			? cartesian2Spherical(
				Coord3(normal.y,-normal.x,normal.z), true)
			: Sphere( 1, 0, 0 );

		    if ( addPick( newpos, dir, true ) )
		    {
			if ( hasDirection() )
			{
			    setUnpickable( true );
			    waitsfordirectionid_ = set_->size()-1;
			}

			eventcatcher_->setHandled();
		    }
		}
	    }
	}
    }
}


bool LocationDisplay::getPickSurface( const visBase::EventInfo& evi,
				      Coord3& newpos, Coord3& normal ) const
{
    const int sz = evi.pickedobjids.size();
    bool validpicksurface = false;
    int eventid = -1;

    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
	    visBase::DM().getObject( evi.pickedobjids[idx] );
	if ( pickedobj == this )
	    continue;

	if ( eventid==-1 && pickedobj->pickable() )
	{
	    eventid = evi.pickedobjids[idx];
	    if ( validpicksurface )
		break;
	}

	mDynamicCastGet(const SurveyObject*,so,pickedobj);
	if ( so && so->allowsPicks() )
	{
	    validpicksurface = true;
	    normal = so->getNormal( evi.displaypickedpos );
	    if ( eventid!=-1 )
		break;
	}
    }

    if ( !validpicksurface )
	return false;

    newpos = evi.worldpickedpos;
    if ( datatransform_ )
    {
	newpos.z = datatransform_->transformBack( newpos );
	if ( mIsUdf(newpos.z) )
	    return false;
    }

    mDynamicCastGet( SurveyObject*,so, visBase::DM().getObject(eventid))
    if ( so ) so->snapToTracePos( newpos );

    return true;
}


Coord3 LocationDisplay::display2World( const Coord3& pos ) const
{
    Coord3 res = pos;
    if ( scene_ )
	res = scene_->getZScaleTransform()->transformBack( pos );
    if ( transformation_ )
	res = transformation_->transformBack( res );
    return res;
}


Coord3 LocationDisplay::world2Display( const Coord3& pos ) const
{
    Coord3 res = transformation_ ? transformation_->transform( pos ) : pos;
    if ( scene_ )
	res = scene_->getZScaleTransform()->transform( res );
    return res;
}


bool LocationDisplay::transformPos( Pick::Location& loc ) const
{
    if ( !datatransform_ ) return true;

    const float newdepth = datatransform_->transform( loc.pos );
    if ( mIsUdf(newdepth) )
	return false;

    loc.pos.z = newdepth;

    if ( hasDirection() )
	pErrMsg("Direction not impl");

    return true;
}


void LocationDisplay::setUnpickable( bool yn )
{
    if ( yn && !pickstyle_ )
    {
	pickstyle_ = visBase::PickStyle::create();
	insertChild( 0, pickstyle_->getInventorNode() );
	pickstyle_->ref();
    }

    if ( pickstyle_ )
	pickstyle_->setStyle( yn ? visBase::PickStyle::Unpickable
				 : visBase::PickStyle::Shape );
}


void LocationDisplay::locChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb)
    if ( !cd )
	{ pErrMsg("Wrong pointer passed"); return; }
    else if ( cd->set_ != set_ )
	return;

    if ( cd->ev_==Pick::SetMgr::ChangeData::Added )
    {
	bool turnon = true;
	Pick::Location loc = (*set_)[cd->loc_];
	if ( !transformPos( loc ) )
	{
	    invalidpicks_ += cd->loc_;
	    turnon = false;
	}

	addDisplayPick( loc, cd->loc_ );

	mDynamicCastGet( visBase::VisualObject*, vo,
			 group_->getObject( cd->loc_ ) );
	if ( vo ) vo->turnOn( turnon );
    }
    else if ( cd->ev_==Pick::SetMgr::ChangeData::ToBeRemoved )
    {
	group_->removeObject( cd->loc_ );
	invalidpicks_ -= cd->loc_;
    }
    else if ( cd->ev_==Pick::SetMgr::ChangeData::Changed )
    {
	bool turnon = true;
	Pick::Location loc = (*set_)[cd->loc_];
	if ( !transformPos( loc ) )
	{
	    if ( invalidpicks_.indexOf(cd->loc_)==-1 )
		invalidpicks_ += cd->loc_;
	    turnon = false;
	}
	else
	{
	    invalidpicks_ -= cd->loc_;
	}

	mDynamicCastGet( visBase::VisualObject*, vo,
			 group_->getObject( cd->loc_ ) );
	if ( vo ) vo->turnOn( turnon );
	setPosition( cd->loc_, loc );
    }

    if ( needline_ ) createLine();
}


void LocationDisplay::setChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps )
	{ pErrMsg("Wrong pointer passed"); return; }
    else if ( ps != set_ )
	return;

    manip_.trigger();
    fullRedraw();
}


void LocationDisplay::dispChg( CallBacker* )
{
    getMaterial()->setColor( set_->disp_.color_ );
    showLine( set_->disp_.connect_ != Pick::Set::Disp::None );
}


void LocationDisplay::setColor( Color nc )
{
    if ( set_ )
    	set_->disp_.color_ = nc;
}


Color LocationDisplay::getColor() const
{
    if ( set_ )
    	return set_->disp_.color_;

    return Color::DgbColor();
}


bool LocationDisplay::isPicking() const
{
    return isSelected() && !isLocked();
}


bool LocationDisplay::addPick( const Coord3& pos, const Sphere& dir,
			       bool notif )
{
    static TypeSet<Coord3> sowinghistory;

    int locidx = -1;
    bool insertpick = false;
    if ( set_->disp_.connect_ == Pick::Set::Disp::Close )
    { 
	sower_->alternateSowingOrder( true );
	Coord3 displaypos = world2Display( pos );
	if ( sower_->mode() == Sower::FirstSowing )
	{
	    displaypos = sower_->pivotPos();
	    sowinghistory.erase();
	}

	float mindist = mUdf(float);
	for ( int idx=0; idx<set_->size(); idx++ )
	{
	    int pidx = idx>0 ? idx-1 : set_->size()-1;

	    int nrmatches = sowinghistory.indexOf( (*set_)[idx].pos ) >= 0;
	    nrmatches += sowinghistory.indexOf( (*set_)[pidx].pos ) >= 0;
	    if ( nrmatches != sowinghistory.size() )
		continue;

	    const float dist = findDistance( world2Display((*set_)[pidx].pos),
		    			     world2Display((*set_)[idx].pos),
					     displaypos );
	    if ( mIsUdf(dist) ) continue;

	    if ( mIsUdf(mindist) || dist<mindist )
	    {
		mindist = dist;
		locidx = idx;
	    }
	}
	insertpick = locidx >= 0;

	sowinghistory.insert( 0, pos );
	sowinghistory.remove( 2 );
    }
    else
	sower_->alternateSowingOrder( false );

    if ( insertpick )
	set_->insert( locidx, Pick::Location(pos,dir) );
    else
    {
	*set_ += Pick::Location( pos, dir );
	locidx = set_->size()-1;
    }

    if ( notif && picksetmgr_ )
    {
	if ( picksetmgr_->indexOf(*set_)==-1 )
	    picksetmgr_->set( MultiID(), set_ );

	Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Added,
				     set_, locidx );
	picksetmgr_->reportChange( 0, cd );
    }

    if ( !hasText() ) return true;

    if ( !(*set_)[locidx].text || !(*set_)[locidx].text->size() )
    {
	removePick( locidx );
	return false;
    }

    return true;
}


void LocationDisplay::removePick( int removeidx )
{
    if ( !picksetmgr_ )
	return;

    Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::ToBeRemoved,
				 set_, removeidx );
    set_->remove( removeidx );
    picksetmgr_->reportChange( 0, cd );
    
    if ( needline_ ) createLine();
}


void LocationDisplay::addDisplayPick( const Pick::Location& loc, int idx )
{
    RefMan<visBase::VisualObject> visobj = createLocation();
    visobj->setDisplayTransformation( transformation_ );

    group_->insertObject( idx, visobj );
    setPosition( idx, loc );
}


BufferString LocationDisplay::getManipulationString() const
{
    BufferString str = "PickSet: "; str += name();
    return str;
}


void LocationDisplay::getMousePosInfo( const visBase::EventInfo&,
				      Coord3& pos, BufferString& val,
				      BufferString& info ) const
{
    val = "";
    info = getManipulationString();
}


void LocationDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs, int )
{
    if ( showall_ && invalidpicks_.isEmpty() ) return;

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	Coord3 pos = (*set_)[idx].pos;
	if ( datatransform_ ) pos.z = datatransform_->transform( pos );

	if ( scene_ )
	    pos = scene_->getUTM2DisplayTransform()->transform( pos );

	bool newstatus;
	if ( !pos.isDefined() )
	    newstatus = false;
	else if ( showall_ )
	    newstatus = true;
	else
	{
	    newstatus = false;

	    for ( int idy=0; idy<objs.size(); idy++ )
	    {
		const float dist = objs[idy]->calcDist(pos);
		if ( dist<objs[idy]->maxDist() )
		{
		    newstatus = true;
		    break;
		}
	    }
	}

	if ( newstatus && invalidpicks_.indexOf(idx)!=-1 )
	{
	    Pick::Location loc = (*set_)[idx];
	    if ( transformPos(loc) )
	    {
		invalidpicks_ -= idx;
		setPosition( idx, loc );
	    }
	    else
	    {
		newstatus = false;
	    }
	}

	mDynamicCastGet(visBase::VisualObject*,vo,group_->getObject(idx));
	vo->turnOn( newstatus );
    }
}


void LocationDisplay::setPosition(int idx, const Pick::Location& nl )
{
    if ( !set_ || idx<0 || idx>=(*set_).size() )
	return;
    
    (*set_)[idx] = nl;
}


void LocationDisplay::setDisplayTransformation( const mVisTrans* newtr )
{
    if ( transformation_==newtr )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = newtr;

    if ( transformation_ )
	transformation_->ref();
    
    if ( polyline_ )
	polyline_->setDisplayTransformation( transformation_ );

    for ( int idx=0; idx<group_->size(); idx++ )
	group_->getObject(idx)->setDisplayTransformation( transformation_ );

    sower_->setDisplayTransformation( newtr );
}


const mVisTrans* LocationDisplay::getDisplayTransformation() const
{
    return transformation_;
}


void LocationDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
}


void LocationDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nevc;
    sower_->setEventCatcher( nevc );

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->ref();
    }

}


int LocationDisplay::getPickIdx( visBase::DataObject* dataobj ) const
{
    return group_->getFirstIdx( dataobj );
}


bool LocationDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    if ( datatransform_==zat )
	return true;

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB( this, LocationDisplay, fullRedraw) );
	datatransform_->unRef();
    }

    datatransform_ = zat;

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		mCB( this, LocationDisplay, fullRedraw) );

	datatransform_->ref();
    }

    
    fullRedraw();
    showAll( datatransform_ && datatransform_->needsVolumeOfInterest() ); 
    return true;
}


const ZAxisTransform* LocationDisplay::getZAxisTransform() const
{
    return datatransform_;
}


int LocationDisplay::isMarkerClick(const TypeSet<int>&) const
{ return -1; }


int LocationDisplay::isDirMarkerClick(const TypeSet<int>&) const
{ return -1; }


void LocationDisplay::triggerDeSel()
{
    setUnpickable( false );
    waitsfordirectionid_ = -1;
    waitsforpositionid_ = -1;
    VisualObject::triggerDeSel();
}


const SurveyObject* LocationDisplay::getPickedSurveyObject() const
{
    const DataObject* pickedobj = visBase::DM().getObject( pickedsobjid_ );
    mDynamicCastGet(const SurveyObject*,so,pickedobj);
    return so;
}


void LocationDisplay::removeSelection( const Selector<Coord3>& selector,
	TaskRunner* tr )
{
    if ( !selector.isOK() )
	return;

    for ( int idx=set_->size()-1; idx>=0; idx-- )
    {
	const Pick::Location& loc = (*set_)[idx];
	if ( selector.includes( loc.pos ) )
	    removePick( idx );
    }
}


void LocationDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    const int setidx = picksetmgr_->indexOf( *set_ );
    par.set( sKeyID(), setidx>=0 ? picksetmgr_->get(*set_) : "" );
    par.set( sKeyMgrName(), picksetmgr_->name() );
    par.setYN( sKeyShowAll(), showall_ );
    par.set( sKeyMarkerType(), set_->disp_.markertype_ );
    par.set( sKeyMarkerSize(), set_->disp_.pixsize_ );

    fillSOPar( par, saveids );
}


int LocationDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    int markertype = 0;
    int pixsize = 3;
    par.get( sKeyMarkerType(), markertype );
    par.get( sKeyMarkerSize(), pixsize );

    bool shwallpicks = true;
    par.getYN( sKeyShowAll(), shwallpicks );
    showAll( shwallpicks );

    BufferString setmgr;
    if ( par.get(sKeyMgrName(),setmgr) )
	setSetMgr( &Pick::SetMgr::getMgr(setmgr.buf()) );

    if ( !par.get(sKeyID(),storedmid_) )
	return -1;

    const int setidx = picksetmgr_ ? picksetmgr_->indexOf( storedmid_ ) : -1;
    if ( setidx==-1 )
    {
	mDeclareAndTryAlloc( Pick::Set*, newps, Pick::Set );

	BufferString bs;
	PtrMan<IOObj> ioobj = IOM().get( storedmid_ );
	if ( ioobj )
	    PickSetTranslator::retrieve( *newps, ioobj, true, bs );

	if ( !newps->name() || !*newps->name() )
	    newps->setName( name() );

	newps->disp_.markertype_ = markertype;
	newps->disp_.pixsize_ = pixsize;

	if ( picksetmgr_ ) picksetmgr_->set( storedmid_, newps );
	setSet( newps );
    }
    else
	setSet( &picksetmgr_->get( storedmid_ ) );

    return useSOPar( par );
}



}; // namespace visSurvey
