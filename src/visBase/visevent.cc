/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "visevent.h"
#include "visdetail.h"
#include "visdataman.h"
#include "vistransform.h"
#include "iopar.h"
#include "mouseevent.h"

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



const char* EventCatcher::eventtypestr()  { return "EventType"; }
//const char EventInfo::leftMouseButton() { return 0; }
//const char EventInfo::middleMouseButton() { return 1; }
//const char EventInfo::rightMouseButton() { return 2; }


EventInfo::EventInfo()
    : detail( 0 )
    , worldpickedpos( Coord3::udf() )
    , localpickedpos( Coord3::udf() )
    , displaypickedpos( Coord3::udf() )
    , buttonstate_( OD::NoButton )
    , tabletinfo( 0 )
{}


EventInfo::EventInfo(const EventInfo& eventinfo )
    : detail( 0 )
    , tabletinfo( 0 )
{
    *this = eventinfo;
}


EventInfo::~EventInfo()
{
    setTabletInfo( 0 );
    setDetail( 0 );
}


EventInfo& EventInfo::operator=( const EventInfo& eventinfo )
{
    if ( &eventinfo == this )
	return *this;

    type = eventinfo.type;
    buttonstate_ = eventinfo.buttonstate_;
    mouseline = eventinfo.mouseline;
    pressed = eventinfo.pressed;
    pickedobjids = eventinfo.pickedobjids;
    displaypickedpos = eventinfo.displaypickedpos;
    localpickedpos = eventinfo.localpickedpos;
    worldpickedpos = eventinfo.worldpickedpos;
    key = eventinfo.key;
    mousepos = eventinfo.mousepos;

    setTabletInfo( eventinfo.tabletinfo );
    setDetail( eventinfo.detail );

    return *this;
}


void EventInfo::setTabletInfo( const TabletInfo* newtabinf )
{
    if ( newtabinf )
    {
	if ( !tabletinfo )
	    tabletinfo = new TabletInfo();

	*tabletinfo = *newtabinf;
    }
    else if ( tabletinfo )
    {
	delete tabletinfo;
	tabletinfo = 0;
    }
}


void EventInfo::setDetail( const Detail* det )
{
    if ( detail )
	delete detail;

    detail = 0;

    mDynamicCastGet( const FaceDetail*, facedetail, det );
    if ( facedetail )
    {
	detail = new FaceDetail( 0 );
	*detail = *facedetail;
    }
    else if ( det )
    {
	detail = new Detail( (DetailType) 0 );
	*detail = *det;
    }
}


EventCatcher::EventCatcher()
    : node_( new SoEventCallback )
    , eventhappened( this )
    , nothandled( this )
    , type_( Any )
    , rehandling_( false )
    , rehandled_( true )
{
    node_->ref();
    setCBs();
}


bool EventCatcher::_init()
{
    if ( !DataObject::_init() )
	return false;

    SO_ENABLE( SoHandleEventAction, SoModelMatrixElement );
    SO_ENABLE( SoHandleEventAction, SoViewVolumeElement );

    return true;
}


void EventCatcher::setEventType( int type )
{
    removeCBs();
    type_ = type;
    setCBs();
}


void EventCatcher::setUtm2Display( ObjectSet<Transformation>& nt )
{
    deepUnRef( utm2display_ );
    utm2display_ = nt;
    deepRef( utm2display_ );
}


void EventCatcher::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( eventtypestr(), (int) type_ );
}


int EventCatcher::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res!= 1 ) return res;

    int inttype;
    if ( !par.get( eventtypestr(), inttype ))
	return -1;

    setEventType( (EventType) inttype );

    return 1;
}
	
    
void EventCatcher::setCBs()
{
    node_->addEventCallback( SoMouseButtonEvent::getClassTypeId(),
			     internalCB, this );
    node_->addEventCallback( SoKeyboardEvent::getClassTypeId(),
			     internalCB, this );
    node_->addEventCallback( SoLocation2Event::getClassTypeId(),
			     internalCB, this );
}


void EventCatcher::removeCBs()
{
    node_->removeEventCallback( SoMouseButtonEvent::getClassTypeId(),
				internalCB, this );
    node_->removeEventCallback( SoKeyboardEvent::getClassTypeId(),
				internalCB, this );
    node_->removeEventCallback( SoLocation2Event::getClassTypeId(),
				internalCB, this );
}


EventCatcher::~EventCatcher()
{
    removeCBs();
    node_->unref();
    deepUnRef( utm2display_ );
}


bool EventCatcher::isHandled() const
{
    if ( rehandling_ ) return rehandled_;
/*
    For some reason, the action associated with some events
    is NULL on Mac, which causes an undesired effect...
*/
    if ( !node_ || !node_->getAction() ) return true;

    return node_->isHandled();
}


void EventCatcher::setHandled()
{
    if ( rehandling_ ) { rehandled_ = true; return; }
/*
    For some reason, the action associated with some events
    is NULL on Mac, which causes an undesired effect...
*/
    if ( node_ && node_->getAction() ) node_->setHandled();
}


void EventCatcher::reHandle( const EventInfo& eventinfo )
{
    rehandling_ = true;
    rehandled_ = false;
    eventhappened.trigger( eventinfo, this );
    rehandled_ = true;
    rehandling_ = false;
}


SoNode* EventCatcher::gtInvntrNode()
{ return node_; }


// Macro used by hack to repair Qt-Linux tablet bug
#define mTabletPressCheck( insync ) \
    if ( eventinfo.tabletinfo->eventtype_ == TabletInfo::Press ) \
    { \
	eventcatcher->tabletispressed_ = true; \
	eventcatcher->tabletinsyncwithmouse_ = insync; \
    }


void EventCatcher::internalCB( void* userdata, SoEventCallback* evcb )
{
    EventCatcher* eventcatcher = (EventCatcher*) userdata;
    if ( eventcatcher->isHandled() ) return;
    const SoEvent* event = evcb->getEvent();

    int buttonstate = 0;
    if ( event->wasShiftDown() )
	buttonstate += OD::ShiftButton;

    if ( event->wasCtrlDown() )
	buttonstate += OD::ControlButton;

    if ( event->wasAltDown() )
	buttonstate += OD::AltButton;

    EventInfo eventinfo;
    const SbVec2s mousepos = event->getPosition();
    eventinfo.mousepos.x = mousepos[0];
    eventinfo.mousepos.y = mousepos[1];

    const SoHandleEventAction* action = evcb->getAction();
    SoState* state = action->getState();

    const SoPickedPoint* pickedpoint = evcb->getPickedPoint();
    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    const SbViewportRegion& viewportregion = action->getViewportRegion();

    const SbVec2f normmousepos = event->getNormalizedPosition(viewportregion);
    SbVec3f startpos, stoppos;
    vv.projectPointToLine( normmousepos, startpos, stoppos );
    const Coord3 startcoord(startpos[0], startpos[1], startpos[2] );
    const Coord3 stopcoord(stoppos[0], stoppos[1], stoppos[2] );
    eventinfo.mouseline = Line3( startcoord, stopcoord-startcoord );

    if ( pickedpoint && pickedpoint->isOnGeometry() )
    {
	const SoPath* path = pickedpoint->getPath();
	if ( path )
	{
	    DM().getIds( path, eventinfo.pickedobjids );
	    if ( eventinfo.pickedobjids.size() )
	    {
		SbVec3f pos3d = pickedpoint->getPoint();
		eventinfo.displaypickedpos.x = pos3d[0];
		eventinfo.displaypickedpos.y = pos3d[1];
		eventinfo.displaypickedpos.z = pos3d[2];
		SbVec3f localpos;
		pickedpoint->getWorldToObject().multVecMatrix(pos3d, localpos );
		eventinfo.localpickedpos.x = localpos[0];
		eventinfo.localpickedpos.y = localpos[1];
		eventinfo.localpickedpos.z = localpos[2];

		eventinfo.worldpickedpos = eventinfo.displaypickedpos;
		for ( int idx=eventcatcher->utm2display_.size()-1;idx>=0; idx--)
		{
		    eventinfo.worldpickedpos =
			eventcatcher->utm2display_[idx]->transformBack(
				eventinfo.worldpickedpos);
		}

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

    SoType eventtype = event->getTypeId();
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
	eventinfo.setTabletInfo( TabletInfo::currentState() );
    }
    else if ( eventtype==SoMouseButtonEvent::getClassTypeId() )
    {
	eventinfo.type = MouseClick;
	eventinfo.setTabletInfo( TabletInfo::currentState() );

	const SoMouseButtonEvent* mbevent = (const SoMouseButtonEvent*) event;
	SoMouseButtonEvent::Button button = mbevent->getButton();
	if ( button==SoMouseButtonEvent::BUTTON1 )
	    buttonstate += OD::LeftButton;
	if ( button==SoMouseButtonEvent::BUTTON2 )
	    buttonstate += OD::RightButton;
	if ( button==SoMouseButtonEvent::BUTTON3 )
	    buttonstate += OD::MidButton;

	if ( SoMouseButtonEvent::isButtonPressEvent( mbevent, button ) )
	    eventinfo.pressed = true;
	else
	    eventinfo.pressed = false;
    }

    eventinfo.buttonstate_ = (OD::ButtonState) buttonstate;

    if ( eventcatcher->eventType()==Any ||
	 eventcatcher->eventType()==eventinfo.type )
    {
	eventcatcher->eventhappened.trigger( eventinfo, eventcatcher );
    }
    if ( !eventcatcher->isHandled() )
	eventcatcher->nothandled.trigger( eventinfo, eventcatcher );
}

}; // namespace visBase
