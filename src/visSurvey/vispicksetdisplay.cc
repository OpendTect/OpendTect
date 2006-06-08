/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID = "$Id: vispicksetdisplay.cc,v 1.85 2006-06-08 09:28:49 cvsnanne Exp $";

#include "vispicksetdisplay.h"

#include "color.h"
#include "iodir.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "pickset.h"
#include "picksettr.h"
#include "survinfo.h"
#include "visevent.h"
#include "visdataman.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "visdatagroup.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "separstr.h"
#include "trigonometry.h"


mCreateFactoryEntry( visSurvey::PickSetDisplay );

namespace visSurvey {

const char* PickSetDisplay::nopickstr = "No Picks";
const char* PickSetDisplay::pickprefixstr = "Pick ";
const char* PickSetDisplay::showallstr = "Show all";
const char* PickSetDisplay::shapestr = "Shape";
const char* PickSetDisplay::sizestr = "Size";
const char* PickSetDisplay::picksetidstr = "PickSet.ID";


PickSetDisplay::PickSetDisplay()
    : VisualObjectImpl(true)
    , group( visBase::DataObjectGroup::create() )
    , eventcatcher( 0 )
    , transformation( 0 )
    , showall(true)
    , set_(0)
    , visnotif_(this)
{
    group->ref();
    addChild( group->getInventorNode() );

    Pick::Mgr().locationChanged.notify( mCB(this,PickSetDisplay,locChg) );
    Pick::Mgr().setChanged.notify( mCB(this,PickSetDisplay,setChg) );
    Pick::Mgr().setDispChanged.notify( mCB(this,PickSetDisplay,dispChg) );
}
    

PickSetDisplay::~PickSetDisplay()
{
    setSceneEventCatcher( 0 );
    removeChild( group->getInventorNode() );
    group->unRef();

    if ( transformation ) transformation->unRef();
    Pick::Mgr().removeCBs( this );
}


void PickSetDisplay::setSet( Pick::Set* s )
{
    if ( set_ ) { pErrMsg("Cannot set set_ twice"); return; }
    set_ = s;
    if ( !set_ ) return;

    const int setidx = Pick::Mgr().indexOf( *set_ );
    if ( setidx>=0 )
	setid_ = Pick::Mgr().id( setidx );

    fullRedraw();
}


void PickSetDisplay::fullRedraw()
{
    group->removeAll();
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

    // if ( hasdir )
       //TODO: no longer necessary?  setType( (int)MarkerStyle3D::Arrow );
}


void PickSetDisplay::showAll( bool yn )
{
    showall = yn;
    if ( !showall ) return;

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*, marker, group->getObject( idx ) );
	if ( !marker ) continue;

	marker->turnOn( true );
    }
}


void PickSetDisplay::pickCB( CallBacker* cb )
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
	mousepressid = eventid;
	mousepressposition = eventid==-1 ? Coord3::udf() : eventinfo.pickedpos;
	eventcatcher->eventIsHandled();
    }
    else 
    {
	if ( eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid )
	    {
		int removeidx = group->getFirstIdx(mousepressid);
		if ( removeidx != -1 )
		{
		    Pick::SetMgr::ChangeData cd(
			    Pick::SetMgr::ChangeData::ToBeRemoved,
			    set_, &(*set_)[removeidx] );
		    group->removeObject( removeidx );
		    Pick::Mgr().reportChange( this, cd );
		    set_->remove( removeidx );
		}
	    }

	    eventcatcher->eventIsHandled();
	}
	else if ( !eventinfo.ctrl && !eventinfo.alt && !eventinfo.shift )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid )
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
		    if ( transformation )
			newpos = transformation->transformBack(newpos);
		    mDynamicCastGet(SurveyObject*,so,
			    	    visBase::DM().getObject(eventid))
		    if ( so ) so->snapToTracePos( newpos );
		    addDisplayPick( addPick(newpos,Sphere(0,0,0),true) );
		}
	    }

	    eventcatcher->eventIsHandled();
	}
    }
}


void PickSetDisplay::locChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::SetMgr::ChangeData*,cd,cb)
    if ( !cd )
	{ pErrMsg("Wrong pointer passed"); return; }
    else if ( cd->set_ != set_ || !cd->loc_ )
	return;

    if ( cd->ev_ == Pick::SetMgr::ChangeData::Added && cd->loc_ )
	addDisplayPick( *cd->loc_ );
    else if ( cd->ev_ == Pick::SetMgr::ChangeData::ToBeRemoved )
	group->removeObject( set_->indexOf(*cd->loc_) );

    visnotif_.trigger();
}


void PickSetDisplay::setChg( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps )
	{ pErrMsg("Wrong pointer passed"); return; }
    else if ( ps != set_ )
	return;

    visnotif_.trigger();
    fullRedraw();
}


void PickSetDisplay::dispChg( CallBacker* )
{
    getMaterial()->setColor( set_->disp_.color_ );
    if ( set_->size() < 1 )
	{ visnotif_.trigger(); return; }

    mDynamicCastGet(visBase::Marker*,firstmarker,group->getObject(0));
    if ( !firstmarker ) return;

    const int oldpixsz = (int)(firstmarker->getScreenSize() + .5);
    if ( oldpixsz != set_->disp_.pixsize_ )
    {
	for ( int idx=0; idx<group->size(); idx++ )
	{
	    mDynamicCastGet(visBase::Marker*,marker,group->getObject(idx));
	    if ( marker )
		marker->setScreenSize( set_->disp_.pixsize_ );
	}
    }

    if ( (int)firstmarker->getType() != set_->disp_.markertype_ )
    {
	for ( int idx=0; idx<group->size(); idx++ )
	{
	    mDynamicCastGet(visBase::Marker*,marker,group->getObject(idx))
	    if ( marker )
		marker->setType( (MarkerStyle3D::Type)set_->disp_.markertype_ );
	}
    }

    visnotif_.trigger();
}


Color PickSetDisplay::getColor() const
{
    return set_->disp_.color_;
}


bool PickSetDisplay::isPicking() const
{
    return isSelected() && !isLocked();
}


Pick::Location& PickSetDisplay::addPick( const Coord3& pos, const Sphere& dir,
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


void PickSetDisplay::addDisplayPick( const Pick::Location& loc )
{
    visBase::Marker* marker = visBase::Marker::create();
    group->addObject( marker );

    marker->setDisplayTransformation( transformation );
    marker->setCenterPos( loc.pos );
    marker->setDirection( loc.dir );
    marker->setScreenSize( set_->disp_.pixsize_ );
    marker->setType( (MarkerStyle3D::Type)set_->disp_.markertype_ );
    marker->setMaterial( 0 );
}


BufferString PickSetDisplay::getManipulationString() const
{
    BufferString str = "Nr. of picks: ";
    str += set_->size();
    return str;
}


void PickSetDisplay::getMousePosInfo( const visBase::EventInfo&,
				      const Coord3& pos, BufferString& val,
				      BufferString& info ) const
{
    val = "";
    info = getManipulationString();
}


void PickSetDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs, int )
{
    if ( showall ) return;
    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,group->getObject(idx))
	if ( !marker ) continue;

	Coord3 pos = marker->centerPos(true);
	marker->turnOn( false );
	for ( int idy=0; idy<objs.size(); idy++ )
	{
	    const float dist = objs[idy]->calcDist(pos);
	    if ( dist < objs[idy]->maxDist() )
	    {
		marker->turnOn(true);
		break;
	    }
	}
    }
}


void PickSetDisplay::setDisplayTransformation( visBase::Transformation* newtr )
{
    if ( transformation==newtr )
	return;

    if ( transformation )
	transformation->unRef();

    transformation = newtr;

    if ( transformation )
	transformation->ref();

    for ( int idx=0; idx<group->size(); idx++ )
    {
	mDynamicCastGet( visBase::Marker*, marker, group->getObject(idx));
	marker->setDisplayTransformation( transformation );
    }
}


visBase::Transformation* PickSetDisplay::getDisplayTransformation()
{
    return transformation;
}


void PickSetDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher )
    {
	eventcatcher->eventhappened.remove(mCB(this,PickSetDisplay,pickCB));
	eventcatcher->unRef();
    }

    eventcatcher = nevc;

    if ( eventcatcher )
    {
	eventcatcher->eventhappened.notify(mCB(this,PickSetDisplay,pickCB));
	eventcatcher->ref();
    }

}


void PickSetDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visBase::VisualObjectImpl::fillPar( par, saveids );

    par.set( picksetidstr, Pick::Mgr().get(*set_) );
    par.setYN( showallstr, showall );
    par.set( shapestr, set_->disp_.markertype_ );
    par.set( sizestr, set_->disp_.pixsize_ );
}


int PickSetDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    int markertype = 0;
    int pixsize = 3;
    par.get( shapestr, markertype );
    par.get( sizestr, pixsize );

    bool shwallpicks = true;
    par.getYN( showallstr, shwallpicks );
    showAll( shwallpicks );

    group->removeAll();

    int nopicks = 0;
    if ( par.get(nopickstr,nopicks) ) // old format
    {
	Pick::Set* newps = new Pick::Set;
	for ( int idx=0; idx<nopicks; idx++ )
	{
	    BufferString str;
	    BufferString key = pickprefixstr; key += idx;
	    if ( !par.get(key,str) )
		return -1;

	    FileMultiString fms( str );
	    Coord3 pos( atof(fms[0]), atof(fms[1]), atof(fms[2]) );
	    Sphere dir;
	    if ( fms.size() > 3 )
		dir = Sphere( atof(fms[3]), atof(fms[4]), atof(fms[5]) );

	    *newps += Pick::Location( pos, dir );
	}

	newps->disp_.markertype_ = markertype;
	newps->disp_.pixsize_ = pixsize;
	newps->disp_.color_ = getMaterial()->getColor();

	BufferString psname;
	par.get( sKey::Name, psname );
	newps->setName( psname );
	setSet( newps );

	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
	IOM().to( ctio->ctxt.stdSelKey() );
	const IOObj* existioobj = IOM().getLocal( psname );
	if ( existioobj )
	    setid_ = existioobj->key();
	else
	{
	    ctio->setName( psname );
	    IOM().getEntry( *ctio );
	    setid_ = ctio->ioobj->key();
	}
    }
    else
    {
	if ( !par.get(picksetidstr,setid_) )
	    return -1;

	PtrMan<IOObj> ioobj = IOM().get( setid_ );
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
    }

    return 1;
}

}; // namespace visSurvey
