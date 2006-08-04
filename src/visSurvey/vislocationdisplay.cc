/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vislocationdisplay.cc,v 1.12 2006-08-04 21:19:59 cvskris Exp $";

#include "vislocationdisplay.h"

#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "pickset.h"
#include "picksettr.h"
#include "survinfo.h"
#include "visevent.h"
#include "visdataman.h"
#include "vismaterial.h"
#include "vispickstyle.h"
#include "vistransform.h"
#include "zaxistransform.h"

namespace visSurvey {

const char* LocationDisplay::sKeyID()		{ return "Location.ID"; }
const char* LocationDisplay::sKeyShowAll()	{ return "Show all"; }
const char* LocationDisplay::sKeyMarkerType()	{ return "Shape"; }
const char* LocationDisplay::sKeyMarkerSize()	{ return "Size"; }


LocationDisplay::LocationDisplay()
    : VisualObjectImpl( true )
    , group_( visBase::DataObjectGroup::create() )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , showall_( true )
    , set_(0)
    , manip_( this )
    , picksetmgr_( 0 )
    , waitsfordirectionid_( -1 )
    , waitsforpositionid_( -1 )
    , datatransform_( 0 )
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

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB( this, LocationDisplay, fullRedraw) );
	datatransform_->unRef();
    }
}


void LocationDisplay::setSet( Pick::Set* s )
{
    if ( set_ ) { pErrMsg("Cannot set set_ twice"); return; }
    set_ = s;
    setName( set_->name() );
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


void LocationDisplay::fullRedraw( CallBacker* )
{
    if ( !set_ ) return;

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
	    addDisplayPick( loc );

	mDynamicCastGet( visBase::VisualObject*, vo,
			 group_->getObject( idx ) );
	if ( vo ) vo->turnOn( turnon );
    }

    while ( idx<group_->size() )
	group_->removeObject( idx );

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
	    Coord3 dir = newpos - (*set_)[waitsfordirectionid_].pos;
	    dir.z *= -SI().zFactor(); //convert to right dir-domain
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
    }
    else if ( waitsforpositionid_!=-1 )
    {
	Coord3 newpos;
	if ( getPickSurface( eventinfo, newpos ) )
	{
	    (*set_)[waitsforpositionid_].pos = newpos;
	    Pick::SetMgr::ChangeData cd(
		    Pick::SetMgr::ChangeData::Changed,
		    set_, waitsforpositionid_ );
	    picksetmgr_->reportChange( 0, cd );
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

	const int selfpickidx = isMarkerClick( eventinfo.pickedobjids );
	if ( selfpickidx!=-1 )
	{
	    setUnpickable( true );
	    waitsforpositionid_ = selfpickidx;
	}
	const int selfdirpickidx = isDirMarkerClick( eventinfo.pickedobjids );
	if ( selfdirpickidx!=-1 )
	{
	    setUnpickable( true );
	    waitsfordirectionid_ = selfpickidx;
	}
    }
    else 
    {
	if ( eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid_ )
	    {
		int removeidx = group_->getFirstIdx(mousepressid_);
		if ( removeidx!=-1 )
		    removePick( removeidx );
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
		    if ( addPick( newpos, Sphere(1,0,0), true ) )
		    {
			if ( hasDirection() )
			{
			    setUnpickable( true );
			    waitsfordirectionid_ = set_->size()-1;
			}
		    }
		}
	    }
	}
    }

    eventcatcher_->eventIsHandled();
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

    newpos = display2World( evi.pickedpos );
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
    Coord3 res  = scene_->getZScaleTransform()->transformBack( pos );
    if ( transformation_ ) res = transformation_->transformBack( res );
    return res;
}


Coord3 LocationDisplay::world2Display( const Coord3& pos ) const
{
    Coord3 res = transformation_ ? transformation_->transform( pos ) : pos;
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

	addDisplayPick( loc );

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
}


Color LocationDisplay::getColor() const
{
    return set_->disp_.color_;
}


bool LocationDisplay::isPicking() const
{
    return isSelected() && !isLocked();
}


bool LocationDisplay::addPick( const Coord3& pos, const Sphere& dir,
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
    picksetmgr_->reportChange( 0, cd );
    set_->remove( removeidx );
}


void LocationDisplay::addDisplayPick( const Pick::Location& loc )
{
    RefMan<visBase::VisualObject> visobj = createLocation();
    visobj->setDisplayTransformation( transformation_ );

    const int idx = group_->size();
    group_->addObject( visobj );
    setPosition( idx, loc );
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
    if ( showall_ && !invalidpicks_.size() ) return;

    for ( int idx=0; idx<group_->size(); idx++ )
    {
	Coord3 pos = (*set_)[idx].pos;
	if ( datatransform_ ) pos.z = datatransform_->transform( pos );

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
	    if ( transformPos( loc ) )
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


int LocationDisplay::getPickIdx( visBase::DataObject* dataobj ) const
{
    return group_->getFirstIdx( dataobj );
}


bool LocationDisplay::setDataTransform( ZAxisTransform* zat )
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
    showAll( !datatransform_ ); 
    return true;
}


const ZAxisTransform* LocationDisplay::getDataTransform() const
{
    return datatransform_;
}


int LocationDisplay::isMarkerClick(const TypeSet<int>&) const
{ return -1; }


int LocationDisplay::isDirMarkerClick(const TypeSet<int>&) const
{ return -1; }

}; // namespace visSurvey
