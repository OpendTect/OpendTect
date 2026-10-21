/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgevent.h"

#include "visvsgdataman.h"
#include "visvsgtransform.h"
#include "vsggeomarkerset.h"
#include "vsggeopick.h"

#include "keyenum.h"

#include <vsg/app/Camera.h>
#include <vsg/core/Inherit.h>
#include <vsg/nodes/Node.h>
#include <vsg/ui/PointerEvent.h>

mCreateDGBVisFactoryEntry( dGBVis::EventCatcher );

namespace dGBVis
{

class EventCatchHandler : public vsg::Inherit<vsg::Visitor,EventCatchHandler>
{
public:
    explicit		EventCatchHandler(EventCatcher&);

    void		apply(vsg::ButtonPressEvent&) override;
    void		apply(vsg::ButtonReleaseEvent&) override;
    void		apply(vsg::MoveEvent&) override;

private:

    template <class Pointer>
    void		routePointer(Pointer&,bool pressed);

    static OD::ButtonState buttonState(vsg::ButtonMask,od_uint32 button);
    void		submit(EventType,OD::ButtonState,bool pressed,
			       bool dragging,int x,int y);

    EventCatcher&	catcher_;
};


EventCatchHandler::EventCatchHandler( EventCatcher& catcher )
    : catcher_(catcher)
{}


template <class Pointer>
void EventCatchHandler::routePointer( Pointer& ev, bool pressed )
{
    submit( MouseClick, buttonState(ev.mask,ev.button), pressed, false,
	    ev.x, ev.y );
    if ( !EventCatcher::forwardToCamera(catcher_) )
	ev.handled = true;
}


void EventCatchHandler::apply( vsg::ButtonPressEvent& ev )
{
    if ( ev.handled )
	return;

    routePointer( ev, true );
}


void EventCatchHandler::apply( vsg::ButtonReleaseEvent& ev )
{
    if ( ev.handled )
	return;

    routePointer( ev, false );
}


void EventCatchHandler::apply( vsg::MoveEvent& ev )
{
    if ( ev.handled )
	return;

    const bool dragging = (ev.mask & (vsg::BUTTON_MASK_1 |
				      vsg::BUTTON_MASK_2 |
				      vsg::BUTTON_MASK_3)) != 0;
    submit( MouseMovement, buttonState(ev.mask,0), false, dragging,
	    ev.x, ev.y );
    if ( !EventCatcher::forwardToCamera(catcher_) )
	ev.handled = true;
}


OD::ButtonState EventCatchHandler::buttonState( vsg::ButtonMask mask,
						od_uint32 button )
{
    int state = 0;
    if ( (mask & vsg::BUTTON_MASK_1) || button==1 )
	state |= int(OD::LeftButton);
    if ( (mask & vsg::BUTTON_MASK_2) || button==2 )
	state |= int(OD::MidButton);
    if ( (mask & vsg::BUTTON_MASK_3) || button==3 )
	state |= int(OD::RightButton);

    const EventCatcher::ModifierQuery& query = EventCatcher::modifierQuery();
    if ( query )
	state |= int(query());

    return OD::ButtonState( state );
}


void EventCatchHandler::submit( EventType type, OD::ButtonState state,
				bool pressed, bool dragging, int x, int y )
{
    catcher_.submit( type, state, pressed, dragging, Coord(x,y) );
}


EventCatcher::ModifierQuery& EventCatcher::modifierQuery()
{
    static ModifierQuery query;
    return query;
}


void EventCatcher::setModifierQuery( const ModifierQuery& query )
{
    modifierQuery() = query;
}


EventCatcher::EventCatcher()
    : eventhappened(this)
    , nothandled(this)
{
    handler_ = EventCatchHandler::create( *this );
}


EventCatcher::~EventCatcher()
{
    detachAllNotifiers();
}


void EventCatcher::setCamera( vsg::Camera* camera )
{
    camera_ = camera;
}


void EventCatcher::setSceneRoot( vsg::Node* root )
{
    sceneroot_ = root;
}


void EventCatcher::setUtm2Display( const Transformation* trans )
{
    utm2display_ = trans;
}


void EventCatcher::reHandle( const visBase::EventInfo& eventinfo )
{
    eventhappened.trigger( eventinfo, this );
}


bool EventCatcher::forwardToCamera( const EventCatcher& catcher )
{
    return !catcher.isHandled();
}


void EventCatcher::publish( visBase::EventInfo& eventinfo )
{
    ishandled_ = false;
    eventhappened.trigger( eventinfo, this );
    if ( !ishandled_ )
	nothandled.trigger( eventinfo, this );
}


bool EventCatcher::pickDisplayPos( const Coord& mousepos,
				    Coord3& displaypos ) const
{
    if ( !camera_ || !sceneroot_ || !mousepos.isDefined() )
	return false;

    const vsgGeo::Pick hit = vsgGeo::intersect(
		*camera_, *sceneroot_, int(mousepos.x_), int(mousepos.y_),
		cActivePickMask, 1. );
    if ( !hit.valid )
	return false;

    displaypos = Coord3( hit.worldPoint.x, hit.worldPoint.y, hit.worldPoint.z );
    return displaypos.isDefined();
}


void EventCatcher::submit( EventType type, OD::ButtonState buttonstate,
			   bool pressed, bool dragging, const Coord& mousepos,
			   OD::KeyboardKey key )
{
    markerindex_ = -1;
    if ( type==MouseClick && pressed )
	wasdragging_ = false;
    else if ( type==MouseMovement && dragging )
	wasdragging_ = true;

    visBase::EventInfo eventinfo;
    eventinfo.type = type;
    eventinfo.buttonstate_ = buttonstate;
    eventinfo.pressed = pressed;
    eventinfo.dragging = type==MouseClick && !pressed ? wasdragging_
						      : dragging;
    eventinfo.mousepos = mousepos;
    eventinfo.key_ = key;

    if ( (type==MouseClick || type==MouseMovement ||
	  type==MouseDoubleClick) &&
	 camera_ && sceneroot_ && mousepos.isDefined() )
    {
	vsg::dvec3 start, end;
	vsgGeo::windowRay( *camera_, int(mousepos.x_), int(mousepos.y_),
			   start, end );
	const Coord3 startc( start.x, start.y, start.z );
	const Coord3 endc( end.x, end.y, end.z );
	eventinfo.mouseline = Line3( startc, Vector3(endc-startc) );

	const vsg::Mask mask = type==MouseMovement && !dragging
		? cPassivePickMask
		: cActivePickMask;
	const vsgGeo::Pick hit = vsgGeo::intersect(
		*camera_, *sceneroot_, int(mousepos.x_), int(mousepos.y_),
		mask, 1. );

	if ( hit.valid )
	{
	    for ( auto it=hit.nodePath.rbegin(); it!=hit.nodePath.rend();
		  ++it )
	    {
		int markeridx = -1;
		if ( markerindex_<0 &&
		     (*it)->getValue(vsgGeo::markerIndexKey(),markeridx) )
		    markerindex_ = markeridx;

		const VisID id = DataObject::getID( *it );
		if ( id.isValid() && !eventinfo.pickedobjids.isPresent(id) )
		    eventinfo.pickedobjids += id;
	    }

	    eventinfo.displaypickedpos = Coord3( hit.worldPoint.x,
						 hit.worldPoint.y,
						 hit.worldPoint.z );

	    Coord3 world = eventinfo.displaypickedpos;
	    if ( utm2display_ )
		utm2display_->transformBack( world );
	    eventinfo.worldpickedpos = world;
	}
    }

    if ( type==MouseClick && !pressed )
	wasdragging_ = false;

    publish( eventinfo );
}

} // namespace dGBVis
