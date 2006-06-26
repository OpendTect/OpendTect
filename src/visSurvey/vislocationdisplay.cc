/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vislocationdisplay.cc,v 1.3 2006-06-26 21:50:37 cvskris Exp $";

#include "vislocationdisplay.h"

#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "pickset.h"
#include "picksettr.h"
#include "visevent.h"
#include "visdataman.h"
#include "vismaterial.h"
#include "vispickstyle.h"
#include "vistransform.h"

namespace visSurvey {

const char* LocationDisplay::sKeyID()		{ return "Location.ID"; }
const char* LocationDisplay::sKeyShowAll()	{ return "Show all"; }
const char* LocationDisplay::sKeyMarkerType()	{ return "Shape"; }
const char* LocationDisplay::sKeyMarkerSize()	{ return "Size"; }


LocationDisplay::LocationDisplay()
    : VisualObjectImpl(true)
    , group_( visBase::DataObjectGroup::create() )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , showall_(true)
    , set_(0)
    , visnotif_(this)
    , picksetmgr_( 0 )
    , waitsfordirectionid_( -1 )
    , pickstyle_( 0 )
{
    group_->ref();
    addChild( group_->getInventorNode() );

    setSetMgr( &Pick::Mgr() );
}
    

LocationDisplay::~LocationDisplay()
{
    setSceneEventCatcher( 0 );
    removeChild( group_->getInventorNode() );
    group_->unRef();

    if ( transformation_ ) transformation_->unRef();
    setSetMgr( 0 );

    if ( pickstyle_ ) pickstyle_->unRef();
}


void LocationDisplay::setSet( Pick::Set* s )
{
    if ( set_ ) { pErrMsg("Cannot set set_ twice"); return; }
    set_ = s;
    fullRedraw();
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

void LocationDisplay::fullRedraw()
{
    getMaterial()->setColor( set_->disp_.color_ );

    group_->removeAll();
    setName( set_->name() );
    dispChg( 0 );
    bool hasdir = false;
    const int nrpicks = set_->size();
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	const Pick::Location& loc = (*set_)[idx];
	addDisplayPick( loc );
	if ( loc.hasDir() ) hasdir = true;
    }
}


void LocationDisplay::showAll( bool yn )
{
    showall_ = yn;
    if ( !showall_ ) return;

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::VisualObject*, vo, group_->getObject( idx ) );
	if ( !vo ) continue;

	vo->turnOn( true );
    }
}


void LocationDisplay::pickCB( CallBacker* cb )
{
    if ( !isOn() || !isSelected() || isLocked() ) return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb);

    if ( waitsfordirectionid_!=-1 )
    {
	Coord3 newpos;
	if ( getPickSurface( eventinfo, newpos ) )
	{
	    const Coord3 dir = newpos - (*set_)[waitsfordirectionid_].pos;
	    if ( dir.sqAbs()>=0 )
	    {
		 (*set_)[waitsfordirectionid_].dir =
		     cartesian2Spherical( dir, false );
		Pick::SetMgr::ChangeData cd(
			Pick::SetMgr::ChangeData::Changed,
			set_, waitsfordirectionid_ );
		picksetmgr_->reportChange( 0, cd );
	    }
	}
    }

    if ( eventinfo.type != visBase::MouseClick ||
	 eventinfo.mousebutton != visBase::EventInfo::leftMouseButton() )
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
	eventcatcher_->eventIsHandled();
    }
    else 
    {
	if ( eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid_ )
	    {
		int removeidx = group_->getFirstIdx(mousepressid_);
		if ( removeidx!=-1 && picksetmgr_ )
		{
		    Pick::SetMgr::ChangeData cd(
			    Pick::SetMgr::ChangeData::ToBeRemoved,
			    set_, removeidx );
		    picksetmgr_->reportChange( 0, cd );
		    set_->remove( removeidx );
		}
	    }

	    eventcatcher_->eventIsHandled();
	}
	else if ( !eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid_ )
	    {
		Coord3 newpos;
		if ( getPickSurface( eventinfo, newpos ) )
		{
		    if ( waitsfordirectionid_!=-1 )
		    {
			pickstyle_->setStyle( visBase::PickStyle::Shape );
			waitsfordirectionid_ = -1;
		    }
		    else
		    {
			addPick( newpos, Sphere(1,0,0), true );

			if ( hasDirection() )
			{
			    if ( !pickstyle_ )
			    {
				pickstyle_ = visBase::PickStyle::create();
				insertChild( 0, pickstyle_->getInventorNode() );
			    }

			    pickstyle_->setStyle(
				    visBase::PickStyle::Unpickable );

			    waitsfordirectionid_ = set_->size()-1;
			}
		    }
		}
	    }

	    eventcatcher_->eventIsHandled();
	}
    }
}


bool LocationDisplay::getPickSurface( const visBase::EventInfo& evi,
				      Coord3& newpos ) const
{
    const int sz = evi.pickedobjids.size();
    bool validpicksurface = false;
    int eventid = -1;

    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
	    visBase::DM().getObject( evi.pickedobjids[idx] );

	if ( eventid==-1 && pickedobj->selectable() )
	{
	    eventid = evi.pickedobjids[idx];
	    if ( validpicksurface )
		break;
	}

	mDynamicCastGet( const SurveyObject*, so, pickedobj );
	if ( so && so->allowPicks() )
	{
	    validpicksurface = true;
	    if ( eventid!=-1 )
		break;
	}
    }

    if ( !validpicksurface )
	return false;

    newpos = scene_->getZScaleTransform()->transformBack( evi.pickedpos );
    if ( transformation_ ) newpos = transformation_->transformBack(newpos);

    mDynamicCastGet( SurveyObject*,so, visBase::DM().getObject(eventid))
    if ( so ) so->snapToTracePos( newpos );

    return true;
}


void LocationDisplay::locChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb)
    if ( !cd )
	{ pErrMsg("Wrong pointer passed"); return; }
    else if ( cd->set_ != set_ )
	return;

    if ( cd->ev_==Pick::SetMgr::ChangeData::Added )
	addDisplayPick( (*cd->set_)[cd->loc_] );
    else if ( cd->ev_==Pick::SetMgr::ChangeData::ToBeRemoved )
	group_->removeObject( cd->loc_ );
    else if ( cd->ev_==Pick::SetMgr::ChangeData::Changed )
    {
	const Pick::Location& loc = (*cd->set_)[cd->loc_];
	setPosition( cd->loc_, loc.pos, loc.dir );
    }

    visnotif_.trigger();
}


void LocationDisplay::setChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps )
	{ pErrMsg("Wrong pointer passed"); return; }
    else if ( ps != set_ )
	return;

    visnotif_.trigger();
    fullRedraw();
}


void LocationDisplay::dispChg( CallBacker* )
{
    getMaterial()->setColor( set_->disp_.color_ );
    if ( set_->size() < 1 )
	{ visnotif_.trigger(); return; }

    visnotif_.trigger();
}


Color LocationDisplay::getColor() const
{
    return set_->disp_.color_;
}


bool LocationDisplay::isPicking() const
{
    return isSelected() && !isLocked();
}


void LocationDisplay::addPick( const Coord3& pos, const Sphere& dir,
			       bool notif )
{
    *set_ += Pick::Location( pos, dir );
    const int locidx = set_->size()-1;
    if ( notif && picksetmgr_ )
    {
	Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Added,
				     set_, locidx );
	picksetmgr_->reportChange( 0, cd );
    }
}


void LocationDisplay::addDisplayPick( const Pick::Location& loc )
{
    RefMan<visBase::VisualObject> visobj = createLocation();
    visobj->setDisplayTransformation( transformation_ );

    const int idx = group_->size();
    group_->addObject( visobj );
    setPosition( idx, loc.pos, loc.dir );
}


BufferString LocationDisplay::getManipulationString() const
{
    BufferString str = "Nr. of picks: ";
    str += set_->size();
    return str;
}


void LocationDisplay::getMousePosInfo( const visBase::EventInfo&,
				      const Coord3& pos, BufferString& val,
				      BufferString& info ) const
{
    val = "";
    info = getManipulationString();
}


void LocationDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs, int )
{
    if ( showall_ ) return;
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::VisualObject*,vo,group_->getObject(idx))

	const Coord3 pos = getPosition(idx);
	vo->turnOn( false );
	for ( int idy=0; idy<objs.size(); idy++ )
	{
	    const float dist = objs[idy]->calcDist(pos);
	    if ( dist<objs[idy]->maxDist() )
	    {
		vo->turnOn(true);
		break;
	    }
	}
    }
}


void LocationDisplay::setDisplayTransformation( visBase::Transformation* newtr )
{
    if ( transformation_==newtr )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = newtr;

    if ( transformation_ )
	transformation_->ref();

    for ( int idx=0; idx<group_->size(); idx++ )
	group_->getObject(idx)->setDisplayTransformation( transformation_ );
}


visBase::Transformation* LocationDisplay::getDisplayTransformation()
{
    return transformation_;
}


void LocationDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nevc;

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->ref();
    }

}


void LocationDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( sKeyID(), picksetmgr_->get(*set_) );
    par.setYN( sKeyShowAll(), showall_ );
    par.set( sKeyMarkerType(), set_->disp_.markertype_ );
    par.set( sKeyMarkerSize(), set_->disp_.pixsize_ );
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

    if ( !par.get(sKeyID(),storedmid_) )
	return -1;

    PtrMan<IOObj> ioobj = IOM().get( storedmid_ );
    if ( !ioobj ) return -1;

    Pick::Set* newps = new Pick::Set;
    BufferString bs;
    if ( PickSetTranslator::retrieve(*newps,ioobj,bs) )
    {
	newps->disp_.markertype_ = markertype;
	newps->disp_.pixsize_ = pixsize;
	setSet( newps );
    }
    else
	return -1;

    return 1;
}

}; // namespace visSurvey
