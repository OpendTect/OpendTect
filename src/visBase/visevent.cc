/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visevent.cc,v 1.14 2004-02-02 15:26:00 kristofer Exp $";

#include "visevent.h"
#include "visdetail.h"
#include "visdataman.h"
#include "vistransform.h"
#include "iopar.h"

#include "Inventor/nodes/SoEventCallback.h"
#include "Inventor/elements/SoModelMatrixElement.h"
#include "Inventor/elements/SoViewVolumeElement.h"
#include "Inventor/events/SoMouseButtonEvent.h"
#include "Inventor/events/SoKeyboardEvent.h"
#include "Inventor/events/SoLocation2Event.h"
#include "Inventor/details/SoFaceDetail.h"
#include "Inventor/SoPickedPoint.h"
#include "Inventor/SbViewportRegion.h"
#include "Inventor/SbLinear.h"

const char* visBase::EventCatcher::eventtypestr = "EventType";

visBase::EventInfo::EventInfo()
    : objecttoworldtrans( visBase::Transformation::create() )
{ objecttoworldtrans->ref(); }


visBase::EventInfo::~EventInfo()
{ objecttoworldtrans->unRef(); }


mCreateFactoryEntry( visBase::EventCatcher );

visBase::EventCatcher::EventCatcher()
    : node( new SoEventCallback )
    , eventhappened( this )
    , type( Any )
{
    node->ref();

    setCBs();
}


void visBase::EventCatcher::_init()
{
    visBase::DataObject::_init();
    SO_ENABLE( SoHandleEventAction, SoModelMatrixElement );
    SO_ENABLE( SoHandleEventAction, SoViewVolumeElement );
}


void visBase::EventCatcher::setEventType( EventType type_ )
{
    removeCBs();
    type = type_;
    setCBs();
}


void visBase::EventCatcher::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( eventtypestr, (int) type );
}


int visBase::EventCatcher::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res!= 1 ) return res;

    int inttype;
    if ( !par.get( eventtypestr, inttype ))
	return -1;

    setEventType( (EventType) inttype );

    return 1;
}
	
    
void visBase::EventCatcher::setCBs()
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


void visBase::EventCatcher::removeCBs()
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


SoNode* visBase::EventCatcher::getInventorNode()
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
	eventinfo.type = visBase::Keyboard;
	eventinfo.pressed = kbevent->getState()==SoButtonEvent::DOWN;
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
