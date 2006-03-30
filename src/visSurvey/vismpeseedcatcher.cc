/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vismpeseedcatcher.cc,v 1.7 2006-03-30 16:13:05 cvsjaap Exp $";

#include "vismpeseedcatcher.h"

#include "emmanager.h"
#include "emobject.h"
#include "survinfo.h"
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
    , clickednode(-1,-1,-1)
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

    if ( !eventinfo.alt && (!eventinfo.ctrl || !eventinfo.shift) )
    {
	for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
	{
	    const int visid = eventinfo.pickedobjids[idx];
	    visBase::DataObject* dataobj = visBase::DM().getObject( visid );
	    if ( !dataobj ) 
		continue;

	    mDynamicCastGet( visSurvey::EMObjectDisplay*, emod, dataobj );
	    if ( emod )
	    {
		sendPlanesContainingNode( visid, emod, eventinfo );
		eventcatcher->eventIsHandled();
		break;
	    }

	    if ( eventinfo.ctrl || eventinfo.shift )
		continue;
	    
	    mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	    if ( plane && plane->getOrientation()!=PlaneDataDisplay::Timeslice )
	    {
		sendClickEvent( EM::PosID(-1,-1,-1), eventinfo.ctrl, 
				eventinfo.shift, eventinfo.pickedpos, 
				visid, plane->getCubeSampling(),
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
		sendClickEvent( EM::PosID(-1,-1,-1), eventinfo.ctrl, 
				eventinfo.shift, eventinfo.pickedpos, 
				visid, clickedcs, 0, 0 );
		eventcatcher->eventIsHandled();
		break;
	    }
	}
    }
}


void MPEClickCatcher::sendPlanesContainingNode( 
			    int visid, const visSurvey::EMObjectDisplay* emod,
			    const visBase::EventInfo& eventinfo )
{
    const EM::PosID nodepid = emod->getPosAttribPosID( EM::EMObject::sSeedNode,
						       eventinfo.pickedobjids );
    if ( nodepid.objectID()==-1 ) 
	return;

    const EM::EMObject* emobj = EM::EMM().getObject( emod->getObjectID() );
    if ( !emobj ) 
	return;

    const Coord3 nodepos = emobj->getPos( nodepid );
    const BinID nodebid = SI().transform( nodepos );
    
    TypeSet<int> planesinscene;
    visBase::DM().getIds( typeid(visSurvey::PlaneDataDisplay), planesinscene ); 
    
    for ( int idx=0; idx<planesinscene.size(); idx++ )
    {
	visBase::DataObject* planeobj = 
				visBase::DM().getObject( planesinscene[idx] );
	if ( !planeobj )
	    continue;

	mDynamicCastGet( PlaneDataDisplay*, plane, planeobj );
	if ( !plane || plane->getOrientation()==PlaneDataDisplay::Timeslice ) 
	    continue;

	const CubeSampling cs = plane->getCubeSampling();
	if ( cs.hrg.includes(nodebid) && cs.zrg.includes(nodepos.z) )
	{
	    sendClickEvent( nodepid, eventinfo.ctrl, eventinfo.shift,
			    eventinfo.pickedpos, visid, cs, 
			    plane->getCacheVolume(false), 
			    plane->getSelSpec(0) );
	}
    }
}


void MPEClickCatcher::sendClickEvent( const EM::PosID pid, bool ctrl, 
				      bool shift, const Coord3& coord, 
				      int visid, const CubeSampling& cs,
       				      const Attrib::DataCubes* ss,
				      const Attrib::SelSpec* selspec )
{
    clickednode = pid;
    ctrlclicked = ctrl;
    shiftclicked = shift;
    clickedcs = cs;
    clickedobjid = visid;
    clickedpos = coord;
    as = selspec;
    attrdata = ss;

    click.trigger();

    clickednode = EM::PosID(-1,-1,-1);
    ctrlclicked = false;
    shiftclicked = false;
    clickedobjid = -1;
    as = 0;
    attrdata = 0;
    clickedcs.init(false);
    clickedpos = Coord3::udf();
}

}; //namespce
