/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visevent.cc,v 1.1 2002-04-08 07:52:12 kristofer Exp $";

#include "visevent.h"
#include "visdataman.h"

#include "Inventor/nodes/SoEventCallback.h"
#include "Inventor/events/SoMouseButtonEvent.h"
#include "Inventor/events/SoKeyboardEvent.h"
#include "Inventor/events/SoLocation2Event.h"
#include "Inventor/SoPickedPoint.h"



visBase::EventCatcher::EventCatcher( EventType type_ )
    : node( new SoEventCallback )
    , eventhappened( this )
    , type( type_ )
{
    node->ref();

    if ( type==MouseClick )
	node->addEventCallback( SoMouseButtonEvent::getClassTypeId(),
				   internalCB, this );
    else if ( type==Keyboard )
	node->addEventCallback( SoKeyboardEvent::getClassTypeId(),
				   internalCB, this );
    else
	node->addEventCallback( SoKeyboardEvent::getClassTypeId(),
				   internalCB, this );
}


visBase::EventCatcher::~EventCatcher()
{
    if ( type==MouseClick )
	node->removeEventCallback( SoMouseButtonEvent::getClassTypeId(),
				   internalCB, this );
    else if ( type==Keyboard )
	node->removeEventCallback( SoKeyboardEvent::getClassTypeId(),
				   internalCB, this );
    else node->removeEventCallback( SoKeyboardEvent::getClassTypeId(),
				    internalCB, this );

    node->unref();
}


void visBase::EventCatcher::internalCB( void* userdata, SoEventCallback* node )
{
    if ( node->isHandled() ) return;

    visBase::EventCatcher* eventcatcher = (visBase::EventCatcher*) userdata;
    const SoEvent* event = node->getEvent();

    visBase::EventInfo eventinfo;

    eventinfo.shift = event->wasShiftDown();
    eventinfo.ctrl = event->wasCtrlDown();
    eventinfo.alt = event->wasAltDown();

    SbVec2s mousepos = event->getPosition();
    eventinfo.mousepos.x = mousepos[0];
    eventinfo.mousepos.y = mousepos[1];

    const SoPickedPoint* pickedpoint = node->getPickedPoint();
    if ( pickedpoint && pickedpoint->isOnGeometry() )
    {
	const SoPath* path = pickedpoint->getPath();
	if ( path )
	{
	    eventinfo.pickedobjid = visBase::DM().getId( path->getTail() );
	    if ( eventinfo.pickedobjid > -1 )
	    {
		SbVec3f pos3d = pickedpoint->getPoint();
		eventinfo.pickedpos.x = pos3d[0];
		eventinfo.pickedpos.y = pos3d[1];
		eventinfo.pickedpos.z = pos3d[2];
	    }
	}
    }
    else
	eventinfo.pickedobjid = -1;

    SoType eventtype =  event->getTypeId();
    if ( eventtype==SoKeyboardEvent::getClassTypeId() )
    {
	const SoKeyboardEvent* kbevent = (const SoKeyboardEvent*) event;
	eventinfo.key = (int)kbevent->getKey();
	eventinfo.type = visBase::MouseMovement;
    }
    else if ( eventtype==SoLocation2Event::getClassTypeId() )
    {
	eventinfo.type = visBase::MouseMovement;
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

	eventinfo.type = visBase::MouseClick;
    }

    eventcatcher->eventhappened.trigger( eventinfo, eventcatcher );
    node->setHandled();
}


