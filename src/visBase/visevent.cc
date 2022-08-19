/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visevent.h"
//#include "visdetail.h"
#include "visdataman.h"
#include "vistransform.h"
#include "iopar.h"
#include "mouseevent.h"
#include "timer.h"

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
    : worldpickedpos( Coord3::udf() )
    , localpickedpos( Coord3::udf() )
    , displaypickedpos( Coord3::udf() )
    , pickdepth( mUdf(double) )
    , mousepos( Coord::udf() )
    , buttonstate_( OD::NoButton )
    , tabletinfo( 0 )
    , type( Any )
    , pressed( false )
    , dragging( false )
{}


EventInfo::EventInfo(const EventInfo& eventinfo )
    : tabletinfo( 0 )
{
    *this = eventinfo;
}


EventInfo::~EventInfo()
{
    setTabletInfo( 0 );
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
    pickdepth = eventinfo.pickdepth;
    key_ = eventinfo.key_;
    mousepos = eventinfo.mousepos;

    setTabletInfo( eventinfo.tabletinfo );
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


//=============================================================================


class EventCatchHandler : public osgGA::GUIEventHandler
{
public:
		EventCatchHandler( EventCatcher& eventcatcher )
		    : eventcatcher_( eventcatcher )
		    , wasdragging_( false )
		{
		    initKeyMap();
		}

    using	osgGA::GUIEventHandler::handle;
    bool	handle(const osgGA::GUIEventAdapter&,
		       osgGA::GUIActionAdapter&) override;

    void	initKeyMap();

protected:
    void	traverse(EventInfo&,unsigned int mask,osgViewer::View*) const;

    EventCatcher&				eventcatcher_;
    bool					wasdragging_;

    typedef std::map<int,OD::KeyboardKey>	KeyMap;
    KeyMap					keymap_;
};


void EventCatchHandler::traverse( EventInfo& eventinfo, unsigned int mask,
				  osgViewer::View* view ) const
{
    if ( !view || !eventinfo.mousepos.isDefined() )
	return;

    osg::ref_ptr<osgUtil::LineSegmentIntersector> lineintersector =
	new osgUtil::LineSegmentIntersector( osgUtil::Intersector::WINDOW,
				eventinfo.mousepos.x, eventinfo.mousepos.y );

    const float frustrumPixelRadius = 1.0f;
    osg::ref_ptr<osgUtil::PolytopeIntersector> polyintersector =
	new osgUtil::PolytopeIntersector( osgUtil::Intersector::WINDOW,
				    eventinfo.mousepos.x-frustrumPixelRadius,
				    eventinfo.mousepos.y-frustrumPixelRadius,
				    eventinfo.mousepos.x+frustrumPixelRadius,
				    eventinfo.mousepos.y+frustrumPixelRadius );

    polyintersector->setDimensionMask( osgUtil::PolytopeIntersector::DimZero |
				       osgUtil::PolytopeIntersector::DimOne );

    const osg::Camera* camera = view->getCamera();
    const osg::Matrix MVPW = camera->getViewMatrix() *
			     camera->getProjectionMatrix() *
			     camera->getViewport()->computeWindowMatrix();

    osg::Matrix invMVPW; invMVPW.invert( MVPW );

    const osg::Vec3 startpos = lineintersector->getStart() * invMVPW;
    const osg::Vec3 stoppos = lineintersector->getEnd() * invMVPW;
    if ( startpos.isNaN() || stoppos.isNaN() )
	return;
    const Coord3 startcoord(startpos[0], startpos[1], startpos[2] );
    const Coord3 stopcoord(stoppos[0], stoppos[1], stoppos[2] );
    eventinfo.mouseline = Line3( startcoord, stopcoord-startcoord );

    osgUtil::IntersectionVisitor iv( lineintersector.get() );
    iv.setTraversalMask( mask );
    view->getCamera()->accept( iv );
    bool linehit = lineintersector->containsIntersections();
    const osgUtil::LineSegmentIntersector::Intersection linepick =
				    lineintersector->getFirstIntersection();

    iv.setIntersector( polyintersector.get() );
    view->getCamera()->accept( iv );
    bool polyhit = polyintersector->containsIntersections();
    const osgUtil::PolytopeIntersector::Intersection polypick =
				    polyintersector->getFirstIntersection();

    if ( linehit && polyhit )
    {
	const osg::Plane triangleplane( linepick.getWorldIntersectNormal(),
					linepick.getWorldIntersectPoint() );

	const int sense = triangleplane.distance(startpos)<0.0 ? -1 : 1;

	const double epscoincide = 1e-6 * (stoppos-startpos).length();
	bool partlybehindplane = false;

	// Prefer lines/points over triangles if they fully coincide
	for ( int idx=polypick.numIntersectionPoints-1; idx>=0; idx-- )
	{
	    osg::Vec3 polypos = polypick.intersectionPoints[idx];
	    if ( polypick.matrix.valid() )
		polypos = polypos * (*polypick.matrix);

	    const double dist = sense * triangleplane.distance(polypos);

	    if ( dist >= epscoincide )	// partly in front of plane
	    {
		linehit = false;
		break;
	    }

	    if ( dist <= -epscoincide )
		partlybehindplane = true;

	    if ( !idx && partlybehindplane )
		polyhit = false;
	}
    }

    if ( linehit || polyhit )
    {
	const osg::NodePath& nodepath = polyhit ? polypick.nodePath
						: linepick.nodePath;

	osg::NodePath::const_reverse_iterator it = nodepath.rbegin();
	for ( ; it!=nodepath.rend(); it++ )
	{
	    const VisID objid = DataObject::getID( *it );
	    if ( objid.isValid() )
		eventinfo.pickedobjids += objid;
	}

	osg::Vec3 pickpos = linehit ? linepick.localIntersectionPoint
				    : polypick.localIntersectionPoint;

	eventinfo.localpickedpos = Conv::to<Coord3>( pickpos );

	const osg::ref_ptr<osg::RefMatrix> mat = linehit ? linepick.matrix
							 : polypick.matrix;
	if ( mat.valid() )
	    pickpos = pickpos * (*mat);

	eventinfo.displaypickedpos = Conv::to<Coord3>( pickpos );
	eventinfo.pickdepth =
		eventinfo.mouseline.closestPoint( eventinfo.displaypickedpos );

	Coord3& pos( eventinfo.worldpickedpos );
	pos = eventinfo.displaypickedpos;
	for ( int idx=eventcatcher_.utm2display_.size()-1; idx>=0; idx-- )
	    eventcatcher_.utm2display_[idx]->transformBack( pos );
    }
}


// solution for osg: Ctrl + a-z
#define mOsgCombinedCtrlKey( key )\
{\
    if ( key>=1 && key<=26 )\
	key+=96;\
}\


bool EventCatchHandler::handle( const osgGA::GUIEventAdapter& ea,
				osgGA::GUIActionAdapter& aa )
{
    if ( ea.getHandled() )
	return false;

    if ( ea.getXnormalized()<=-1.0 || ea.getXnormalized()>=1.0 ||
	 ea.getYnormalized()<=-1.0 || ea.getYnormalized()>=1.0 )
    {
	if ( !wasdragging_ )
	    return false;
    }

    EventInfo eventinfo;
    bool isactivepickevent = true;

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
	wasdragging_ = true;
    }
    else if ( ea.getEventType() == osgGA::GUIEventAdapter::MOVE )
    {
	eventinfo.type = MouseMovement;
	eventinfo.dragging = false;
	isactivepickevent = false;
    }
    else if ( ea.getEventType() == osgGA::GUIEventAdapter::PUSH )
    {
	eventinfo.type = MouseClick;
	eventinfo.pressed = true;
	wasdragging_ = false;
    }
    else if ( ea.getEventType() == osgGA::GUIEventAdapter::RELEASE )
    {
	eventinfo.type = MouseClick;
	eventinfo.pressed = false;
    }
    else if ( ea.getEventType() == osgGA::GUIEventAdapter::DOUBLECLICK )
    {
	eventinfo.type = MouseDoubleClick;
	eventinfo.pressed = false;
	wasdragging_ = false;
    }
    else
	return false;

    if ( eventinfo.type == Keyboard )
    {
	int key = ea.getKey();
	mOsgCombinedCtrlKey( key );
	KeyMap::iterator it = keymap_.find( key );
	eventinfo.key_ = it==keymap_.end() ? (OD::KeyboardKey)key : it->second;
    }

    if ( eventinfo.type==MouseMovement || eventinfo.type==MouseClick )
	eventinfo.setTabletInfo( TabletInfo::currentState() );

    int buttonstate = 0;

    if ( eventinfo.type == MouseClick || eventinfo.type == MouseDoubleClick )
    {
	if ( ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON )
	    buttonstate += OD::LeftButton;
	if ( ea.getButton() == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON )
	    buttonstate += OD::MidButton;
	if ( ea.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON )
	{
	    buttonstate += OD::RightButton;
	    if ( !eventinfo.pressed && !wasdragging_ )
		isactivepickevent = false;
	}

	//We don't accept press or release events if no buttons are active.
	//Probably we have touch-event.
	if ( !buttonstate )
	    return false;
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

    mDynamicCastGet( osgViewer::View*, view, &aa );

    EventInfo passiveinfo( eventinfo );
    EventInfo activeinfo( eventinfo );

    traverse( passiveinfo, cPassiveIntersecTraversalMask(), view );
    traverse( activeinfo, cActiveIntersecTraversalMask(), view );

    EventInfo* foremostinfo = new EventInfo( eventinfo );
    foremostinfo->mouseline = passiveinfo.mouseline; // = activeinfo.mouseline

    const double eps = 1e-6;
    if ( !isactivepickevent && !mIsUdf(passiveinfo.pickdepth) )
    {
	if ( mIsUdf(activeinfo.pickdepth) ||
	     passiveinfo.pickdepth <= activeinfo.pickdepth*(1.0+eps) )
	{
	    *foremostinfo = passiveinfo;
	}
    }

    if ( isactivepickevent && !mIsUdf(activeinfo.pickdepth) )
    {
	if ( mIsUdf(passiveinfo.pickdepth) ||
	     activeinfo.pickdepth <= passiveinfo.pickdepth*(1.0+eps) )
	{
	    *foremostinfo = activeinfo;
	}
    }

    eventcatcher_.releaseEventsCB( 0 );

    Threads::Locker locker( eventcatcher_.eventqueuelock_ );
    eventcatcher_.eventqueue_ += foremostinfo;
    locker.unlockNow();

    if ( !eventcatcher_.eventreleasepostosg_ )
    {
	eventcatcher_.releaseEventsCB( 0 );
	return eventcatcher_.ishandled_;
    }

    // 0 times out if all events in window system's queue have been processed
    eventcatcher_.eventreleasetimer_->start( 0, true );
    return false;
}


void EventCatchHandler::initKeyMap()
{
    keymap_[osgGA::GUIEventAdapter::KEY_0] = OD::KB_Zero;
    keymap_[osgGA::GUIEventAdapter::KEY_1] = OD::KB_One;
    keymap_[osgGA::GUIEventAdapter::KEY_2] = OD::KB_Two;
    keymap_[osgGA::GUIEventAdapter::KEY_3] = OD::KB_Three;
    keymap_[osgGA::GUIEventAdapter::KEY_4] = OD::KB_Four;
    keymap_[osgGA::GUIEventAdapter::KEY_5] = OD::KB_Five;
    keymap_[osgGA::GUIEventAdapter::KEY_6] = OD::KB_Six;
    keymap_[osgGA::GUIEventAdapter::KEY_7] = OD::KB_Seven;
    keymap_[osgGA::GUIEventAdapter::KEY_8] = OD::KB_Eight;
    keymap_[osgGA::GUIEventAdapter::KEY_9] = OD::KB_Nine;
    keymap_[osgGA::GUIEventAdapter::KEY_A] = OD::KB_A;
    keymap_[osgGA::GUIEventAdapter::KEY_B] = OD::KB_B;
    keymap_[osgGA::GUIEventAdapter::KEY_C] = OD::KB_C;
    keymap_[osgGA::GUIEventAdapter::KEY_D] = OD::KB_D;
    keymap_[osgGA::GUIEventAdapter::KEY_E] = OD::KB_E;
    keymap_[osgGA::GUIEventAdapter::KEY_F] = OD::KB_F;
    keymap_[osgGA::GUIEventAdapter::KEY_G] = OD::KB_G;
    keymap_[osgGA::GUIEventAdapter::KEY_H] = OD::KB_H;
    keymap_[osgGA::GUIEventAdapter::KEY_I] = OD::KB_I;
    keymap_[osgGA::GUIEventAdapter::KEY_J] = OD::KB_J;
    keymap_[osgGA::GUIEventAdapter::KEY_K] = OD::KB_K;
    keymap_[osgGA::GUIEventAdapter::KEY_L] = OD::KB_L;
    keymap_[osgGA::GUIEventAdapter::KEY_M] = OD::KB_M;
    keymap_[osgGA::GUIEventAdapter::KEY_N] = OD::KB_N;
    keymap_[osgGA::GUIEventAdapter::KEY_O] = OD::KB_O;
    keymap_[osgGA::GUIEventAdapter::KEY_P] = OD::KB_P;
    keymap_[osgGA::GUIEventAdapter::KEY_Q] = OD::KB_Q;
    keymap_[osgGA::GUIEventAdapter::KEY_R] = OD::KB_R;
    keymap_[osgGA::GUIEventAdapter::KEY_S] = OD::KB_S;
    keymap_[osgGA::GUIEventAdapter::KEY_T] = OD::KB_T;
    keymap_[osgGA::GUIEventAdapter::KEY_U] = OD::KB_U;
    keymap_[osgGA::GUIEventAdapter::KEY_V] = OD::KB_V;
    keymap_[osgGA::GUIEventAdapter::KEY_W] = OD::KB_W;
    keymap_[osgGA::GUIEventAdapter::KEY_X] = OD::KB_X;
    keymap_[osgGA::GUIEventAdapter::KEY_Y] = OD::KB_Y;
    keymap_[osgGA::GUIEventAdapter::KEY_Z] = OD::KB_Z;

    // Grabbed from function setUpKeyMap() in osgQt/QGraphicsViewAdapter.cpp

    keymap_[osgGA::GUIEventAdapter::KEY_BackSpace] = OD::KB_Backspace;
    keymap_[osgGA::GUIEventAdapter::KEY_Tab] = OD::KB_Tab;
    keymap_[osgGA::GUIEventAdapter::KEY_Linefeed] = OD::KB_Return;// No LineFeed
    keymap_[osgGA::GUIEventAdapter::KEY_Clear] = OD::KB_Clear;
    keymap_[osgGA::GUIEventAdapter::KEY_Return] = OD::KB_Return;
    keymap_[osgGA::GUIEventAdapter::KEY_Pause] = OD::KB_Pause;
    keymap_[osgGA::GUIEventAdapter::KEY_Scroll_Lock] = OD::KB_ScrollLock;
    keymap_[osgGA::GUIEventAdapter::KEY_Sys_Req] = OD::KB_SysReq;
    keymap_[osgGA::GUIEventAdapter::KEY_Escape] = OD::KB_Escape;
    keymap_[osgGA::GUIEventAdapter::KEY_Delete] = OD::KB_Delete;

    keymap_[osgGA::GUIEventAdapter::KEY_Home] = OD::KB_Home;
    keymap_[osgGA::GUIEventAdapter::KEY_Left] = OD::KB_Left;
    keymap_[osgGA::GUIEventAdapter::KEY_Up] = OD::KB_Up;
    keymap_[osgGA::GUIEventAdapter::KEY_Right] = OD::KB_Right;
    keymap_[osgGA::GUIEventAdapter::KEY_Down] = OD::KB_Down;
    keymap_[osgGA::GUIEventAdapter::KEY_Prior] = OD::KB_Left;	// No Prior
    keymap_[osgGA::GUIEventAdapter::KEY_Page_Up] = OD::KB_PageUp;
    keymap_[osgGA::GUIEventAdapter::KEY_Next] = OD::KB_Right;	// No Next
    keymap_[osgGA::GUIEventAdapter::KEY_Page_Down] = OD::KB_PageDown;
    keymap_[osgGA::GUIEventAdapter::KEY_End] = OD::KB_End;
    keymap_[osgGA::GUIEventAdapter::KEY_Begin] = OD::KB_Home;	// No Begin

    keymap_[osgGA::GUIEventAdapter::KEY_Select] = OD::KB_Select;
    keymap_[osgGA::GUIEventAdapter::KEY_Print] = OD::KB_Print;
    keymap_[osgGA::GUIEventAdapter::KEY_Execute] = OD::KB_Execute;
    keymap_[osgGA::GUIEventAdapter::KEY_Insert] = OD::KB_Insert;
    //keymap_[osgGA::GUIEventAdapter::KEY_Undo]		// No Undo
    //keymap_[osgGA::GUIEventAdapter::KEY_Redo]		// No Redo
    keymap_[osgGA::GUIEventAdapter::KEY_Menu] = OD::KB_Menu;
    keymap_[osgGA::GUIEventAdapter::KEY_Find] = OD::KB_Search;	// No Find
    keymap_[osgGA::GUIEventAdapter::KEY_Cancel] = OD::KB_Cancel;
    keymap_[osgGA::GUIEventAdapter::KEY_Help] = OD::KB_Help;
    keymap_[osgGA::GUIEventAdapter::KEY_Break] = OD::KB_Escape; // No Break
    keymap_[osgGA::GUIEventAdapter::KEY_Mode_switch] = OD::KB_Mode_switch;
    keymap_[osgGA::GUIEventAdapter::KEY_Script_switch] = OD::KB_Mode_switch;
							// No Script switch
    keymap_[osgGA::GUIEventAdapter::KEY_Num_Lock] = OD::KB_NumLock;

    keymap_[osgGA::GUIEventAdapter::KEY_Shift_L] = OD::KB_Shift;
    keymap_[osgGA::GUIEventAdapter::KEY_Shift_R] = OD::KB_Shift;
    keymap_[osgGA::GUIEventAdapter::KEY_Control_L] = OD::KB_Control;
    keymap_[osgGA::GUIEventAdapter::KEY_Control_R] = OD::KB_Control;
    keymap_[osgGA::GUIEventAdapter::KEY_Caps_Lock] = OD::KB_CapsLock;
    keymap_[osgGA::GUIEventAdapter::KEY_Shift_Lock] = OD::KB_CapsLock;

    keymap_[osgGA::GUIEventAdapter::KEY_Meta_L] = OD::KB_Meta;	// No Meta L
    keymap_[osgGA::GUIEventAdapter::KEY_Meta_R] = OD::KB_Meta;	// No Meta R
    keymap_[osgGA::GUIEventAdapter::KEY_Alt_L] = OD::KB_Alt;	// No Alt L
    keymap_[osgGA::GUIEventAdapter::KEY_Alt_R] = OD::KB_Alt;	// No Alt R
    keymap_[osgGA::GUIEventAdapter::KEY_Super_L] = OD::KB_Super_L;
    keymap_[osgGA::GUIEventAdapter::KEY_Super_R] = OD::KB_Super_R;
    keymap_[osgGA::GUIEventAdapter::KEY_Hyper_L] = OD::KB_Hyper_L;
    keymap_[osgGA::GUIEventAdapter::KEY_Hyper_R] = OD::KB_Hyper_R;

    keymap_[osgGA::GUIEventAdapter::KEY_KP_Space] = OD::KB_Space;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Tab] = OD::KB_Tab;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Enter] = OD::KB_Enter;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_F1] = OD::KB_F1;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_F2] = OD::KB_F2;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_F3] = OD::KB_F3;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_F4] = OD::KB_F4;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Home] = OD::KB_Home;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Left] = OD::KB_Left;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Up] = OD::KB_Up;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Right] = OD::KB_Right;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Down] = OD::KB_Down;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Prior] = OD::KB_Left;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Page_Up] = OD::KB_PageUp;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Next] = OD::KB_Right;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Page_Down] = OD::KB_PageDown;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_End] = OD::KB_End;

    // keymap_[osgGA::GUIEventAdapter::KEY_KP_Begin] = OD::KB_; // No Begin
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Insert] = OD::KB_Insert;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Delete] = OD::KB_Delete;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Equal] = OD::KB_Equal;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Multiply] = OD::KB_Asterisk;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Add] = OD::KB_Plus;
    //keymap_[osgGA::GUIEventAdapter::KEY_KP_Separator] // No Separator
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Subtract] = OD::KB_Minus;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Decimal] = OD::KB_Period;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_Divide] = OD::KB_Division;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_0] = OD::KB_Zero;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_1] = OD::KB_One;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_2] = OD::KB_Two;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_3] = OD::KB_Three;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_4] = OD::KB_Four;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_5] = OD::KB_Five;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_6] = OD::KB_Six;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_7] = OD::KB_Seven;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_8] = OD::KB_Eight;
    keymap_[osgGA::GUIEventAdapter::KEY_KP_9] = OD::KB_Nine;

    keymap_[osgGA::GUIEventAdapter::KEY_F1] = OD::KB_F1;
    keymap_[osgGA::GUIEventAdapter::KEY_F2] = OD::KB_F2;
    keymap_[osgGA::GUIEventAdapter::KEY_F3] = OD::KB_F3;
    keymap_[osgGA::GUIEventAdapter::KEY_F4] = OD::KB_F4;
    keymap_[osgGA::GUIEventAdapter::KEY_F5] = OD::KB_F5;
    keymap_[osgGA::GUIEventAdapter::KEY_F6] = OD::KB_F6;
    keymap_[osgGA::GUIEventAdapter::KEY_F7] = OD::KB_F7;
    keymap_[osgGA::GUIEventAdapter::KEY_F8] = OD::KB_F8;
    keymap_[osgGA::GUIEventAdapter::KEY_F9] = OD::KB_F9;
    keymap_[osgGA::GUIEventAdapter::KEY_F10] = OD::KB_F10;
    keymap_[osgGA::GUIEventAdapter::KEY_F11] = OD::KB_F11;
    keymap_[osgGA::GUIEventAdapter::KEY_F12] = OD::KB_F12;
    keymap_[osgGA::GUIEventAdapter::KEY_F13] = OD::KB_F13;
    keymap_[osgGA::GUIEventAdapter::KEY_F14] = OD::KB_F14;
    keymap_[osgGA::GUIEventAdapter::KEY_F15] = OD::KB_F15;
    keymap_[osgGA::GUIEventAdapter::KEY_F16] = OD::KB_F16;
    keymap_[osgGA::GUIEventAdapter::KEY_F17] = OD::KB_F17;
    keymap_[osgGA::GUIEventAdapter::KEY_F18] = OD::KB_F18;
    keymap_[osgGA::GUIEventAdapter::KEY_F19] = OD::KB_F19;
    keymap_[osgGA::GUIEventAdapter::KEY_F20] = OD::KB_F20;
    keymap_[osgGA::GUIEventAdapter::KEY_F21] = OD::KB_F21;
    keymap_[osgGA::GUIEventAdapter::KEY_F22] = OD::KB_F22;
    keymap_[osgGA::GUIEventAdapter::KEY_F23] = OD::KB_F23;
    keymap_[osgGA::GUIEventAdapter::KEY_F24] = OD::KB_F24;
    keymap_[osgGA::GUIEventAdapter::KEY_F25] = OD::KB_F25;
    keymap_[osgGA::GUIEventAdapter::KEY_F26] = OD::KB_F26;
    keymap_[osgGA::GUIEventAdapter::KEY_F27] = OD::KB_F27;
    keymap_[osgGA::GUIEventAdapter::KEY_F28] = OD::KB_F28;
    keymap_[osgGA::GUIEventAdapter::KEY_F29] = OD::KB_F29;
    keymap_[osgGA::GUIEventAdapter::KEY_F30] = OD::KB_F30;
    keymap_[osgGA::GUIEventAdapter::KEY_F31] = OD::KB_F31;
    keymap_[osgGA::GUIEventAdapter::KEY_F32] = OD::KB_F32;
    keymap_[osgGA::GUIEventAdapter::KEY_F33] = OD::KB_F33;
    keymap_[osgGA::GUIEventAdapter::KEY_F34] = OD::KB_F34;
    keymap_[osgGA::GUIEventAdapter::KEY_F35] = OD::KB_F35;
}


//=============================================================================


EventCatcher::EventCatcher()
    : eventhappened( this )
    , nothandled( this )
    , type_( Any )
    , ishandled_( true )
    , rehandling_( false )
    , rehandled_( false )
    , osgnode_( 0 )
    , eventcatchhandler_( 0 )
    , eventreleasepostosg_( true )
    , eventreleasetimer_( new Timer() )
{
    osgnode_ = setOsgNode( new osg::Node );
    eventcatchhandler_ = new EventCatchHandler( *this );
    eventcatchhandler_->ref();
    osgnode_->setEventCallback( eventcatchhandler_ );
    mAttachCB( eventreleasetimer_->tick, EventCatcher::releaseEventsCB );
}


void EventCatcher::setEventType( int type )
{
    type_ = type;
}

void EventCatcher::releaseEventsPostOsg( bool yn )
{ eventreleasepostosg_ = yn; }


void EventCatcher::setUtm2Display( ObjectSet<Transformation>& nt )
{
    deepUnRef( utm2display_ );
    utm2display_ = nt;
    deepRef( utm2display_ );
}


EventCatcher::~EventCatcher()
{
    deepUnRef( utm2display_ );

    osgnode_->removeEventCallback( eventcatchhandler_ );
    eventcatchhandler_->unref();
    delete eventreleasetimer_;
}


bool EventCatcher::isHandled() const
{
    if ( rehandling_ ) return rehandled_;

    return ishandled_;
}


void EventCatcher::setHandled()
{
    if ( rehandling_ ) { rehandled_ = true; return; }

    ishandled_ = true;
}


void EventCatcher::reHandle( const EventInfo& eventinfo )
{
    rehandling_ = true;
    rehandled_ = false;
    eventhappened.trigger( eventinfo, this );
    rehandled_ = true;
    rehandling_ = false;
}


void EventCatcher::releaseEventsCB( CallBacker* )
{
    while ( true )
    {
	Threads::Locker locker( eventqueuelock_ );
	if ( eventqueue_.isEmpty() )
	    return;

	const EventInfo* curevent = eventqueue_.removeSingle( 0 );
	locker.unlockNow();

	ishandled_ = false;

	if ( type_==Any || type_==curevent->type )
	    eventhappened.trigger( *curevent, this );

	if ( !ishandled_ )
	    nothandled.trigger( *curevent, this );

	delete curevent;
    }
}


} // namespace visBase
