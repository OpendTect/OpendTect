/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visevent.cc,v 1.9 2003-07-08 09:49:28 jeroen Exp $";

#include "visevent.h"
#include "visdetail.h"
#include "visdataman.h"
#include "iopar.h"

#include "Inventor/nodes/SoEventCallback.h"
#include "Inventor/events/SoMouseButtonEvent.h"
#include "Inventor/events/SoKeyboardEvent.h"
#include "Inventor/events/SoLocation2Event.h"
#include "Inventor/details/SoFaceDetail.h"
#include "Inventor/SoPickedPoint.h"
#include "Inventor/SbLinear.h"

const char* visBase::EventCatcher::eventtypestr = "EventType";

mCreateFactoryEntry( visBase::EventCatcher );

visBase::EventCatcher::EventCatcher()
    : node( new SoEventCallback )
    , eventhappened( this )
    , type( Any )
{
    node->ref();

    setCBs();
}


void visBase::EventCatcher::setEventType( EventType type_ )
{
    removeCBs();
    type = type_;
    setCBs();
}


void visBase::EventCatcher::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    SceneObject::fillPar( par, saveids );
    par.set( eventtypestr, (int) type );
}


int visBase::EventCatcher::usePar( const IOPar& par )
{
    int res = SceneObject::usePar( par );
    if ( res!= 1 ) return res;

    int type;

    if ( !par.get( eventtypestr, type ))
	return -1;

    setEventType( (EventType) type );

    return 1;
}
	
    
void visBase::EventCatcher::setCBs()
{
    if ( type==MouseClick || type==Any )
	node->addEventCallback( SoMouseButtonEvent::getClassTypeId(),
				   internalCB, this );
    else if ( type==Keyboard || type==Any )
	node->addEventCallback( SoKeyboardEvent::getClassTypeId(),
				   internalCB, this );
    else if ( type==MouseMovement || type==Any )
	node->addEventCallback( SoLocation2Event::getClassTypeId(),
				    internalCB, this );
}


void visBase::EventCatcher::removeCBs()
{
    if ( type==MouseClick || type==Any )
	node->removeEventCallback( SoMouseButtonEvent::getClassTypeId(),
				   internalCB, this );
    else if ( type==Keyboard || type==Any )
	node->removeEventCallback( SoKeyboardEvent::getClassTypeId(),
				   internalCB, this );
    else if ( type==MouseMovement || type==Any )
	node->removeEventCallback( SoLocation2Event::getClassTypeId(),
				    internalCB, this );
}


visBase::EventCatcher::~EventCatcher()
{
    removeCBs();
    node->unref();
}


bool visBase::EventCatcher::isEventHandled() const
{
    return node->isHandled();
}


void visBase::EventCatcher::eventIsHandled()
{
    node->setHandled();
}


SoNode* visBase::EventCatcher::getData()
{ return node; }


void visBase::EventCatcher::internalCB( void* userdata, SoEventCallback* evcb )
{
    visBase::EventCatcher* eventcatcher = (visBase::EventCatcher*) userdata;
    if ( eventcatcher->isEventHandled() ) return;

    const SoEvent* event = evcb->getEvent();

    visBase::EventInfo eventinfo;

    eventinfo.shift = event->wasShiftDown();
    eventinfo.ctrl = event->wasCtrlDown();
    eventinfo.alt = event->wasAltDown();

    SbVec2s mousepos = event->getPosition();
    eventinfo.mousepos.x = mousepos[0];
    eventinfo.mousepos.y = mousepos[1];

    const SoPickedPoint* pickedpoint = evcb->getPickedPoint();
    if ( pickedpoint && pickedpoint->isOnGeometry() )
    {
	const SoPath* path = pickedpoint->getPath();
	if ( path )
	{
	    visBase::DM().getIds( path, eventinfo.pickedobjids );
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
		    eventinfo.detail = new visBase::FaceDetail( fd );
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
}
