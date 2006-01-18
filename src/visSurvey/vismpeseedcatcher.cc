/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismpeseedcatcher.cc,v 1.6 2006-01-18 22:58:59 cvskris Exp $";

#include "vismpeseedcatcher.h"

#include "emobject.h"
#include "visdataman.h"
#include "visemobjdisplay.h"
#include "visevent.h"
#include "vismpe.h"
#include "vistransform.h"
#include "visplanedatadisplay.h"

mCreateFactoryEntry( visSurvey::MPEClickCatcher );


namespace visSurvey
{

MPEClickCatcher::MPEClickCatcher()
    : click( this )
    , eventcatcher( 0 )
    , transformation( 0 )
    , ctrlclicknode(-1,-1,-1)
{ }


MPEClickCatcher::~MPEClickCatcher()
{
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );
}


void MPEClickCatcher::setSceneEventCatcher( visBase::EventCatcher* nev )
{
    if ( eventcatcher )
    {
	eventcatcher->eventhappened.remove( mCB(this,MPEClickCatcher,clickCB) );
	eventcatcher->unRef();
    }

    eventcatcher = nev;

    if ( eventcatcher )
    {
	eventcatcher->eventhappened.notify( mCB(this,MPEClickCatcher,clickCB) );
	eventcatcher->ref();
    }
}


void MPEClickCatcher::setDisplayTransformation( visBase::Transformation* nt )
{
    if ( transformation )
	transformation->unRef();

    transformation = nt;
    if ( transformation )
	transformation->ref();
}


const Coord3& MPEClickCatcher::clickedPos() const { return clickedpos; }


void MPEClickCatcher::clickCB( CallBacker* cb )
{
    if ( eventcatcher->isEventHandled() || !isOn() )
	return;

    mCBCapsuleUnpack(const visBase::EventInfo&,eventinfo,cb );

    if ( eventinfo.type!=visBase::MouseClick || !eventinfo.pressed )
	return;

    if ( eventinfo.mousebutton!=visBase::EventInfo::leftMouseButton() )
	return;

    if ( !eventinfo.shift && !eventinfo.alt && !eventinfo.ctrl )
    {
	for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
	{
	    const int visid = eventinfo.pickedobjids[idx];
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	    mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	    if ( plane )
	    {
		sendClickEvent( eventinfo.pickedpos, visid,
				plane->getCubeSampling(),
				plane->getCacheVolume(false),
				plane->getSelSpec(0) );
		eventcatcher->eventIsHandled();
		break;
	    }

	    mDynamicCastGet( MPEDisplay*, mpedisplay, dataobj );
	    if ( mpedisplay && mpedisplay->isDraggerShown() &&
		 !mpedisplay->isManipulatorShown() &&
		 mpedisplay->getPlanePosition(clickedcs) ) 
	    {
		sendClickEvent( eventinfo.pickedpos, visid, clickedcs, 0, 0 );
		eventcatcher->eventIsHandled();
		break;
	    }
	}
    }
    else if ( !eventinfo.shift && !eventinfo.alt && eventinfo.ctrl )
    {
	for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
	{
	    const int visid = eventinfo.pickedobjids[idx];
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	    mDynamicCastGet( visSurvey::EMObjectDisplay*, emod, dataobj );
	    if ( !emod ) continue;

	    ctrlclicknode =
		emod->getPosAttribPosID( EM::EMObject::sSeedNode,
					 eventinfo.pickedobjids );

	    if ( ctrlclicknode.objectID()!=-1 )
	    {
		click.trigger();
		eventcatcher->eventIsHandled();
	    }

	    break;
	}
    }
}


void MPEClickCatcher::sendClickEvent( const Coord3& coord, int visid, 
				      const CubeSampling& cs,
       				      const Attrib::DataCubes* ss,
				      const Attrib::SelSpec* selspec )
{
    ctrlclicknode = EM::PosID(-1,-1,-1);
    clickedcs = cs;
    clickedobjid = visid;
    clickedpos = coord;
    as = selspec;
    attrdata = ss;

    click.trigger();

    clickedobjid = -1;
    as = 0;
    attrdata = 0;
    clickedcs.init(false);
    clickedpos = Coord3::udf();
}


}; //namespce
