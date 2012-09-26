/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "keystrs.h"
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

#include <osgGA/GUIEventHandler>
#include <osgUtil/LineSegmentIntersector>
#include <osgViewer/Viewer>
#include <osg/ValueObject>

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
    , type( Any )
    , pressed( false )
    , dragging( false )
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
    dragging = eventinfo.dragging;
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

//=============================================================================


class EventCatchHandler : public osgGA::GUIEventHandler                  
{      
public:
		EventCatchHandler( EventCatcher& eventcatcher )
		    : eventcatcher_( eventcatcher )
		    , ishandled_( true )
		{
		    initKeyMap();
		}

    using	osgGA::GUIEventHandler::handle;
    bool	handle(const osgGA::GUIEventAdapter&,osgGA::GUIActionAdapter&);

    void 	setHandled()			{ ishandled_ = true; }
    bool	isHandled() const		{ return ishandled_; }
    void	initKeyMap();

protected:
    EventCatcher&				eventcatcher_;
    bool					ishandled_;

    typedef std::map<int,OD::KeyboardKey>	KeyMap;
    KeyMap					keymap_;
};


bool EventCatchHandler::handle( const osgGA::GUIEventAdapter& ea,
				osgGA::GUIActionAdapter& aa )
{
    if ( ea.getHandled() )
	return false;

    EventInfo eventinfo;

    if ( ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN )
    {
	eventinfo.type = Keyboard;
	eventinfo.pressed = true;
    }
    else if ( ea.getEventType() == osgGA::GUIEventAdapter::KEYUP )
    {
	eventinfo.type = Keyboard;
	eventinfo.pressed = false;
    }
    else if ( ea.getEventType() == osgGA::GUIEventAdapter::DRAG )
    {
	eventinfo.type = MouseMovement;
	eventinfo.dragging = true;
    }
    else if ( ea.getEventType() == osgGA::GUIEventAdapter::MOVE )
    {
	eventinfo.type = MouseMovement;
	eventinfo.dragging = false;
    }
    else if ( ea.getEventType() == osgGA::GUIEventAdapter::PUSH )
    {
	eventinfo.type = MouseClick;
	eventinfo.pressed = true;
    }
    else if ( ea.getEventType() == osgGA::GUIEventAdapter::RELEASE )
    {
	eventinfo.type = MouseClick;
	eventinfo.pressed = false;
    }
    else
	return false;

    if ( eventinfo.type == Keyboard )
    {
	KeyMap::iterator it = keymap_.find( ea.getKey() );
	eventinfo.key = it==keymap_.end() ? ea.getKey() : it->second;
    }

    if ( eventinfo.type==MouseMovement || eventinfo.type==MouseClick )
	eventinfo.setTabletInfo( TabletInfo::currentState() );

    int buttonstate = 0;

    if ( eventinfo.type == MouseClick )
    {
	if ( ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON )
	    buttonstate += OD::LeftButton;
	if ( ea.getButton() == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON )
	    buttonstate += OD::MidButton;
	if ( ea.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON )
	    buttonstate += OD::RightButton;
    }

    if ( ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT )
	buttonstate += OD::ShiftButton;

    if ( ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL )
	buttonstate += OD::ControlButton;

    if ( ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT )
	buttonstate += OD::AltButton;

    eventinfo.buttonstate_ = (OD::ButtonState) buttonstate;

    eventinfo.mousepos.x = ea.getX();
    eventinfo.mousepos.y = ea.getY(); 

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
	    new osgUtil::LineSegmentIntersector( osgUtil::Intersector::WINDOW,
						 ea.getX(), ea.getY() );

    osgUtil::IntersectionVisitor iv( intersector.get() );
    mDynamicCastGet( osg::View*, view, &aa );

    if ( view )
    {
	iv.setTraversalMask( IntersectionTraversal );
	view->getCamera()->accept( iv );

	if ( intersector->containsIntersections() )
	{
	    const osgUtil::LineSegmentIntersector::Intersection pick =
					intersector->getFirstIntersection();

	    osg::NodePath::const_reverse_iterator it = pick.nodePath.rbegin();
	    for ( ; it!=pick.nodePath.rend(); it++ )
	    {
		int objid;
		static std::string idstr( sKey::ID() );
		if ( (*it)->getUserValue(idstr, objid) && objid>=0 )
		    eventinfo.pickedobjids += objid;
	    }

	    const osg::Vec3 lpos = pick.getLocalIntersectPoint();
	    eventinfo.localpickedpos = Coord3( lpos[0], lpos[1], lpos[2] );

	    const osg::Vec3 dpos = pick.getWorldIntersectPoint();
	    eventinfo.displaypickedpos = Coord3( dpos[0], dpos[1], dpos[2] );

	    Coord3& pos( eventinfo.worldpickedpos );
	    pos = eventinfo.displaypickedpos;
	    for ( int idx=eventcatcher_.utm2display_.size()-1; idx>=0; idx-- )
		pos = eventcatcher_.utm2display_[idx]->transformBack( pos );
	}
    }

    const osg::Camera* camera = view->getCamera();
    const osg::Matrix MVPW = camera->getViewMatrix() *
			     camera->getProjectionMatrix() *
			     camera->getViewport()->computeWindowMatrix();

    osg::Matrix invMVPW; invMVPW.invert( MVPW );

    const osg::Vec3 startpos = intersector->getStart() * invMVPW;
    const osg::Vec3 stoppos = intersector->getEnd() * invMVPW;
    const Coord3 startcoord(startpos[0], startpos[1], startpos[2] );
    const Coord3 stopcoord(stoppos[0], stoppos[1], stoppos[2] );
    eventinfo.mouseline = Line3( startcoord, stopcoord-startcoord );

    ishandled_ = false;

    if ( eventcatcher_.eventType()==Any ||
	 eventcatcher_.eventType()==eventinfo.type )
    {
	eventcatcher_.eventhappened.trigger( eventinfo, &eventcatcher_ );
    }

    if ( !ishandled_ )
	eventcatcher_.nothandled.trigger( eventinfo, &eventcatcher_ );

    const bool res = ishandled_;
    ishandled_ = true;

    return res;
}


void EventCatchHandler::initKeyMap()
{
    // Cribbed from function setUpKeyMap() in osgQt/QGraphicsViewAdapter.cpp

    keymap_[osgGA::GUIEventAdapter::KEY_BackSpace] = OD::Backspace;
    keymap_[osgGA::GUIEventAdapter::KEY_Tab] = OD::Tab;
    keymap_[osgGA::GUIEventAdapter::KEY_Linefeed] = OD::Return;	// No LineFeed
    keymap_[osgGA::GUIEventAdapter::KEY_Clear] = OD::Clear;
    keymap_[osgGA::GUIEventAdapter::KEY_Return] = OD::Return;
    keymap_[osgGA::GUIEventAdapter::KEY_Pause] = OD::Pause;
    keymap_[osgGA::GUIEventAdapter::KEY_Scroll_Lock] = OD::ScrollLock;
    keymap_[osgGA::GUIEventAdapter::KEY_Sys_Req] = OD::SysReq;
    keymap_[osgGA::GUIEventAdapter::KEY_Escape] = OD::Escape;
    keymap_[osgGA::GUIEventAdapter::KEY_Delete] = OD::Delete;

    keymap_[osgGA::GUIEventAdapter::KEY_Home] = OD::Home;
    keymap_[osgGA::GUIEventAdapter::KEY_Left] = OD::Left;
    keymap_[osgGA::GUIEventAdapter::KEY_Up] = OD::Up;
    keymap_[osgGA::GUIEventAdapter::KEY_Right] = OD::Right;
    keymap_[osgGA::GUIEventAdapter::KEY_Down] = OD::Down;
    keymap_[osgGA::GUIEventAdapter::KEY_Prior] = OD::Left;	// No Prior
    keymap_[osgGA::GUIEventAdapter::KEY_Page_Up] = OD::PageUp;
    keymap_[osgGA::GUIEventAdapter::KEY_Next] = OD::Right;	// No Next
    keymap_[osgGA::GUIEventAdapter::KEY_Page_Down] = OD::PageDown;
    keymap_[osgGA::GUIEventAdapter::KEY_End] = OD::End;
    keymap_[osgGA::GUIEventAdapter::KEY_Begin] = OD::Home;	// No Begin

    keymap_[osgGA::GUIEventAdapter::KEY_Select] = OD::Select;
    keymap_[osgGA::GUIEventAdapter::KEY_Print] = OD::Print;
    keymap_[osgGA::GUIEventAdapter::KEY_Execute] = OD::Execute;
    keymap_[osgGA::GUIEventAdapter::KEY_Insert] = OD::Insert;
    //keymap_[osgGA::GUIEventAdapter::KEY_Undo] = OD::;		// No Undo
    //keymap_[osgGA::GUIEventAdapter::KEY_Redo] = OD::;		// No Redo
    keymap_[osgGA::GUIEventAdapter::KEY_Menu] = OD::Menu;
    keymap_[osgGA::GUIEventAdapter::KEY_Find] = OD::Search;	// No Find
    keymap_[osgGA::GUIEventAdapter::KEY_Cancel] = OD::Cancel;
    keymap_[osgGA::GUIEventAdapter::KEY_Help] = OD::Help;
    keymap_[osgGA::GUIEventAdapter::KEY_Break] = OD::Escape;	// No Break
    keymap_[osgGA::GUIEventAdapter::KEY_Mode_switch] = OD::Mode_switch;
    keymap_[osgGA::GUIEventAdapter::KEY_Script_switch] = OD::Mode_switch;
							// No Script switch
    keymap_[osgGA::GUIEventAdapter::KEY_Num_Lock] = OD::NumLock;

    keymap_[osgGA::GUIEventAdapter::KEY_Shift_L] = OD::Shift;
    keymap_[osgGA::GUIEventAdapter::KEY_Shift_R] = OD::Shift;
    keymap_[osgGA::GUIEventAdapter::KEY_Control_L] = OD::Control;
    keymap_[osgGA::GUIEventAdapter::KEY_Control_R] = OD::Control;
    keymap_[osgGA::GUIEventAdapter::KEY_Caps_Lock] = OD::CapsLock;
    keymap_[osgGA::GUIEventAdapter::KEY_Shift_Lock] = OD::CapsLock;

    keymap_[osgGA::GUIEventAdapter::KEY_Meta_L] = OD::Meta;	// No Meta L
    keymap_[osgGA::GUIEventAdapter::KEY_Meta_R] = OD::Meta;	// No Meta R
    keymap_[osgGA::GUIEventAdapter::KEY_Alt_L] = OD::Alt;	// No Alt L
    keymap_[osgGA::GUIEventAdapter::KEY_Alt_R] = OD::Alt;	// No Alt R
    keymap_[osgGA::GUIEventAdapter::KEY_Super_L] = OD::Super_L;
    keymap_[osgGA::GUIEventAdapter::KEY_Super_R] = OD::Super_R;
    keymap_[osgGA::GUIEventAdapter::KEY_Hyper_L] = OD::Hyper_L;
    keymap_[osgGA::GUIEventAdapter::KEY_Hyper_R] = OD::Hyper_R;

    keymap_[osgGA::GUIEventAdapter::KEY_KP_Space] = OD::Space;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Tab] = OD::Tab;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Enter] = OD::Enter;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_F1] = OD::F1;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_F2] = OD::F2;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_F3] = OD::F3;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_F4] = OD::F4;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Home] = OD::Home;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Left] = OD::Left;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Up] = OD::Up;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Right] = OD::Right;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Down] = OD::Down;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Prior] = OD::Left;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Page_Up] = OD::PageUp;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Next] = OD::Right;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Page_Down] = OD::PageDown;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_End] = OD::End;

    // keymap_[osgGA::GUIEventAdapter::KEY_KP_Begin] = OD::;	// No Begin
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Insert] = OD::Insert;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Delete] = OD::Delete;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Equal] = OD::Equal;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Multiply] = OD::Asterisk;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Add] = OD::Plus;
    //keymap_[osgGA::GUIEventAdapter::KEY_KP_Separator] = OD::;	// No Separator
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Subtract] = OD::Minus;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Decimal] = OD::Period;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Divide] = OD::Division;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_0] = OD::Zero;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_1] = OD::One;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_2] = OD::Two;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_3] = OD::Three;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_4] = OD::Four;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_5] = OD::Five;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_6] = OD::Six;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_7] = OD::Seven;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_8] = OD::Eight;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_9] = OD::Nine;

    keymap_[osgGA::GUIEventAdapter::KEY_F1] = OD::F1;
    keymap_[osgGA::GUIEventAdapter::KEY_F2] = OD::F2;
    keymap_[osgGA::GUIEventAdapter::KEY_F3] = OD::F3;
    keymap_[osgGA::GUIEventAdapter::KEY_F4] = OD::F4;
    keymap_[osgGA::GUIEventAdapter::KEY_F5] = OD::F5;
    keymap_[osgGA::GUIEventAdapter::KEY_F6] = OD::F6;
    keymap_[osgGA::GUIEventAdapter::KEY_F7] = OD::F7;
    keymap_[osgGA::GUIEventAdapter::KEY_F8] = OD::F8;
    keymap_[osgGA::GUIEventAdapter::KEY_F9] = OD::F9;
    keymap_[osgGA::GUIEventAdapter::KEY_F10] = OD::F10;
    keymap_[osgGA::GUIEventAdapter::KEY_F11] = OD::F11;
    keymap_[osgGA::GUIEventAdapter::KEY_F12] = OD::F12;
    keymap_[osgGA::GUIEventAdapter::KEY_F13] = OD::F13;
    keymap_[osgGA::GUIEventAdapter::KEY_F14] = OD::F14;
    keymap_[osgGA::GUIEventAdapter::KEY_F15] = OD::F15;
    keymap_[osgGA::GUIEventAdapter::KEY_F16] = OD::F16;
    keymap_[osgGA::GUIEventAdapter::KEY_F17] = OD::F17;
    keymap_[osgGA::GUIEventAdapter::KEY_F18] = OD::F18;
    keymap_[osgGA::GUIEventAdapter::KEY_F19] = OD::F19;
    keymap_[osgGA::GUIEventAdapter::KEY_F20] = OD::F20;
    keymap_[osgGA::GUIEventAdapter::KEY_F21] = OD::F21;
    keymap_[osgGA::GUIEventAdapter::KEY_F22] = OD::F22;
    keymap_[osgGA::GUIEventAdapter::KEY_F23] = OD::F23;
    keymap_[osgGA::GUIEventAdapter::KEY_F24] = OD::F24;
    keymap_[osgGA::GUIEventAdapter::KEY_F25] = OD::F25;
    keymap_[osgGA::GUIEventAdapter::KEY_F26] = OD::F26;
    keymap_[osgGA::GUIEventAdapter::KEY_F27] = OD::F27;
    keymap_[osgGA::GUIEventAdapter::KEY_F28] = OD::F28;
    keymap_[osgGA::GUIEventAdapter::KEY_F29] = OD::F29;
    keymap_[osgGA::GUIEventAdapter::KEY_F30] = OD::F30;
    keymap_[osgGA::GUIEventAdapter::KEY_F31] = OD::F31;
    keymap_[osgGA::GUIEventAdapter::KEY_F32] = OD::F32;
    keymap_[osgGA::GUIEventAdapter::KEY_F33] = OD::F33;
    keymap_[osgGA::GUIEventAdapter::KEY_F34] = OD::F34;
    keymap_[osgGA::GUIEventAdapter::KEY_F35] = OD::F35;
}


//=============================================================================


EventCatcher::EventCatcher()
    : node_( new SoEventCallback )
    , eventhappened( this )
    , nothandled( this )
    , type_( Any )
    , rehandling_( false )
    , rehandled_( true )
    , osgnode_( 0 )
    , eventcatchhandler_( 0 )
{
    node_->ref();
    setCBs();

    if ( doOsg() )
    {
	osgnode_ = new osg::Node;
	osgnode_->ref();
	eventcatchhandler_ = new EventCatchHandler( *this );
	osgnode_->setEventCallback( eventcatchhandler_ );
    }
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

    if ( doOsg() )
    {
	osgnode_->removeEventCallback( eventcatchhandler_ );
	osgnode_->unref();
    }
}


bool EventCatcher::isHandled() const
{
    if ( rehandling_ ) return rehandled_;

    if ( eventcatchhandler_ )
	return eventcatchhandler_->isHandled();

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

    if ( eventcatchhandler_ )
	eventcatchhandler_->setHandled();

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


osg::Node* EventCatcher::gtOsgNode()
{ return osgnode_; }


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
