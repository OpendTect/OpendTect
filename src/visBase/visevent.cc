/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: visevent.cc,v 1.19 2005-02-07 12:45:40 nanne Exp $
________________________________________________________________________

-*/

#include "visevent.h"
#include "visdetail.h"
#include "visdataman.h"
#include "vistransform.h"
#include "iopar.h"

#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbLinear.h>

mCreateFactoryEntry( visBase::EventCatcher );

namespace visBase
{

const char* EventCatcher::eventtypestr = "EventType";

EventInfo::EventInfo()
    : objecttoworldtrans( Transformation::create() )
    , detail(0)
{ objecttoworldtrans->ref(); }


EventInfo::~EventInfo()
{
    objecttoworldtrans->unRef();
    delete detail;
}


EventCatcher::EventCatcher()
    : node( new SoEventCallback )
    , eventhappened( this )
    , nothandled( this )
    , type( Any )
{
    node->ref();

    setCBs();
}


void EventCatcher::_init()
{
    DataObject::_init();
    SO_ENABLE( SoHandleEventAction, SoModelMatrixElement );
    SO_ENABLE( SoHandleEventAction, SoViewVolumeElement );
}


void EventCatcher::setEventType( int type_ )
{
    removeCBs();
    type = type_;
    setCBs();
}


void EventCatcher::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( eventtypestr, (int) type );
}


int EventCatcher::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res!= 1 ) return res;

    int inttype;
    if ( !par.get( eventtypestr, inttype ))
	return -1;

    setEventType( (EventType) inttype );

    return 1;
}
	
    
void EventCatcher::setCBs()
{
    if ( type==MouseClick || type==Any )
	node->addEventCallback( SoMouseButtonEvent::getClassTypeId(),
				   internalCB, this );
    if ( type==Keyboard || type==Any )
	node->addEventCallback( SoKeyboardEvent::getClassTypeId(),
				   internalCB, this );
    if ( type==MouseMovement || type==Any )
	node->addEventCallback( SoLocation2Event::getClassTypeId(),
				    internalCB, this );
}


void EventCatcher::removeCBs()
{
    if ( type==MouseClick || type==Any )
	node->removeEventCallback( SoMouseButtonEvent::getClassTypeId(),
				   internalCB, this );
    if ( type==Keyboard || type==Any )
	node->removeEventCallback( SoKeyboardEvent::getClassTypeId(),
				   internalCB, this );
    if ( type==MouseMovement || type==Any )
	node->removeEventCallback( SoLocation2Event::getClassTypeId(),
				    internalCB, this );
}


EventCatcher::~EventCatcher()
{
    removeCBs();
    node->unref();
}


bool EventCatcher::isEventHandled() const
{
/*
    For some reason, the action associated with some events
    is NULL on Mac, which causes an undesired effect...
*/
    if ( !node || !node->getAction() ) return true;

    return node->isHandled();
}


void EventCatcher::eventIsHandled()
{
/*
    For some reason, the action associated with some events
    is NULL on Mac, which causes an undesired effect...
*/
    if ( node && node->getAction() ) node->setHandled();
}


SoNode* EventCatcher::getInventorNode()
{ return node; }


void EventCatcher::internalCB( void* userdata, SoEventCallback* evcb )
{
    EventCatcher* eventcatcher = (EventCatcher*) userdata;
    if ( eventcatcher->isEventHandled() ) return;
    const SoEvent* event = evcb->getEvent();

    EventInfo eventinfo;
    eventinfo.shift = event->wasShiftDown();
    eventinfo.ctrl = event->wasCtrlDown();
    eventinfo.alt = event->wasAltDown();

    const SbVec2s mousepos = event->getPosition();
    eventinfo.mousepos.x = mousepos[0];
    eventinfo.mousepos.y = mousepos[1];

    const SoHandleEventAction* action = evcb->getAction();
    SoState* state = action->getState();

    const SoPickedPoint* pickedpoint = evcb->getPickedPoint();
    const SbViewVolume &vv = SoViewVolumeElement::get(state);
    const SbViewportRegion& viewportregion = action->getViewportRegion();

    const SbVec2f normmousepos = event->getNormalizedPosition(viewportregion);
    SbVec3f startpos, stoppos;
    vv.projectPointToLine( normmousepos, startpos, stoppos );
    const Coord3 startcoord(startpos[0], startpos[1], startpos[2] );
    const Coord3 stopcoord(stoppos[0], stoppos[1], stoppos[2] );
    eventinfo.mouseline = Line3( startcoord, stopcoord-startcoord );

    const SbMatrix& mat = SoModelMatrixElement::get(state);
    eventinfo.objecttoworldtrans->setA(mat);

    if ( pickedpoint && pickedpoint->isOnGeometry() )
    {
	const SoPath* path = pickedpoint->getPath();
	if ( path )
	{
	    DM().getIds( path, eventinfo.pickedobjids );
	    if ( eventinfo.pickedobjids.size() )
	    {
		SbVec3f pos3d = pickedpoint->getPoint();
		eventinfo.pickedpos.x = pos3d[0];
		eventinfo.pickedpos.y = pos3d[1];
		eventinfo.pickedpos.z = pos3d[2];
		SbVec3f localpos;
		pickedpoint->getWorldToObject().multVecMatrix(pos3d, localpos );
		eventinfo.localpickedpos.x = localpos[0];
		eventinfo.localpickedpos.y = localpos[1];
		eventinfo.localpickedpos.z = localpos[2];

		const SoDetail* detail = pickedpoint->getDetail();
		mDynamicCastGet( const SoFaceDetail*, facedetail, detail );
		if ( facedetail )
		{
		    SoFaceDetail* fd = const_cast< SoFaceDetail* >(facedetail);
		    eventinfo.detail = new FaceDetail( fd );
		}
	    }
	}
    }
    else
	eventinfo.pickedobjids.erase();

    SoType eventtype =  event->getTypeId();
    if ( eventtype==SoKeyboardEvent::getClassTypeId() )
    {
	const SoKeyboardEvent* kbevent = (const SoKeyboardEvent*) event;
	eventinfo.key = (int)kbevent->getKey();
	eventinfo.type = Keyboard;
	eventinfo.pressed = kbevent->getState()==SoButtonEvent::DOWN;
    }
    else if ( eventtype==SoLocation2Event::getClassTypeId() )
    {
	eventinfo.type = MouseMovement;
    }
    else if ( eventtype==SoMouseButtonEvent::getClassTypeId() )
    {
	const SoMouseButtonEvent* mbevent = (const SoMouseButtonEvent*) event;
	SoMouseButtonEvent::Button button = mbevent->getButton();
	if ( button==SoMouseButtonEvent::BUTTON1 ) eventinfo.mousebutton = 0;
	if ( button==SoMouseButtonEvent::BUTTON2 ) eventinfo.mousebutton = 1;
	if ( button==SoMouseButtonEvent::BUTTON3 ) eventinfo.mousebutton = 2;

	if ( SoMouseButtonEvent::isButtonPressEvent( mbevent, button ) )
	    eventinfo.pressed = true;
	else
	    eventinfo.pressed = false;

	eventinfo.type = MouseClick;
    }

    eventcatcher->eventhappened.trigger( eventinfo, eventcatcher );
    if ( !eventcatcher->isEventHandled() )
	eventcatcher->nothandled.trigger( eventinfo, eventcatcher );
}

}; // namespace visBase
