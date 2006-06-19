/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vislocationdisplay.cc,v 1.2 2006-06-19 21:34:22 cvskris Exp $";

#include "vislocationdisplay.h"

#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "pickset.h"
#include "picksettr.h"
#include "visevent.h"
#include "visdataman.h"
#include "vismaterial.h"
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
{
    group_->ref();
    addChild( group_->getInventorNode() );

    Pick::Mgr().locationChanged.notify( mCB(this,LocationDisplay,locChg) );
    Pick::Mgr().setChanged.notify( mCB(this,LocationDisplay,setChg) );
    Pick::Mgr().setDispChanged.notify( mCB(this,LocationDisplay,dispChg) );
}
    

LocationDisplay::~LocationDisplay()
{
    setSceneEventCatcher( 0 );
    removeChild( group_->getInventorNode() );
    group_->unRef();

    if ( transformation_ ) transformation_->unRef();
    Pick::Mgr().removeCBs( this );
}


void LocationDisplay::setSet( Pick::Set* s )
{
    if ( set_ ) { pErrMsg("Cannot set set_ twice"); return; }
    set_ = s;
    fullRedraw();
}


void LocationDisplay::fullRedraw()
{
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
	mousepressposition_ = eventid==-1 ? Coord3::udf() : eventinfo.pickedpos;
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
		if ( removeidx != -1 )
		{
		    Pick::SetMgr::ChangeData cd(
			    Pick::SetMgr::ChangeData::ToBeRemoved,
			    set_, &(*set_)[removeidx] );
		    group_->removeObject( removeidx );
		    Pick::Mgr().reportChange( this, cd );
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
		    Coord3 newpos = scene_->getZScaleTransform()->
			transformBack( eventinfo.pickedpos );
		    if ( transformation_ )
			newpos = transformation_->transformBack(newpos);
		    mDynamicCastGet(SurveyObject*,so,
			    	    visBase::DM().getObject(eventid))
		    if ( so ) so->snapToTracePos( newpos );
		    addDisplayPick( addPick(newpos,Sphere(0,0,0),true) );
		}
	    }

	    eventcatcher_->eventIsHandled();
	}
    }
}


void LocationDisplay::locChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb)
    if ( !cd )
	{ pErrMsg("Wrong pointer passed"); return; }
    else if ( cd->set_ != set_ || !cd->loc_ )
	return;

    if ( cd->ev_ == Pick::SetMgr::ChangeData::Added && cd->loc_ )
	addDisplayPick( *cd->loc_ );
    else if ( cd->ev_ == Pick::SetMgr::ChangeData::ToBeRemoved )
	group_->removeObject( set_->indexOf(*cd->loc_) );

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


Pick::Location& LocationDisplay::addPick( const Coord3& pos, const Sphere& dir,
					 bool notif )
{
    *set_ += Pick::Location( pos, dir );
    Pick::Location& pl = (*set_)[set_->size()-1];
    if ( notif )
    {
	Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Added,
				     set_, &pl);
	Pick::Mgr().reportChange( this, cd );
    }

    return pl;
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

    par.set( sKeyID(), Pick::Mgr().get(*set_) );
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
