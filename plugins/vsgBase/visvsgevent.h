#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "visvsgdata.h"

#include "integerid.h"
#include "keyenum.h"
#include "position.h"
#include "trigonometry.h"
#include "typeset.h"

#include <vsg/core/ref_ptr.h>

#include <functional>

namespace vsg { class Camera; class Node; class Visitor; }
namespace dGBVis { class Transformation; class EventCatchHandler; }

namespace dGBVis
{

enum EventType		{ Any=7, MouseClick=1, Keyboard=2, MouseMovement=4,
			  MouseDoubleClick=5 };

/*!\brief What a viewer event tells: the pointer, the keys and what is under it.
*/

class EventInfo
{
public:
    EventType			type		= Any;
    OD::ButtonState		buttonstate_	= OD::NoButton;
    Coord			mousepos	= Coord::udf();
    Line3			mouseline;
				/*!<From the mouse position into the scene,
				    in display coordinates. */
    double			pickdepth	= mUdf(double);
				//!<Mouseline parameter value of picked pos

    bool			pressed		= false;
				/*!<Only set if type is MouseClick or Keyboard.
				    False if the button has been released. */
    bool			dragging	= false;
				//!<Only set if type is MouseMovement

    OD::KeyboardKey		key_;
				//!<Only set if type is Keyboard

    TypeSet<VisID>		pickedobjids;

    Coord3			displaypickedpos = Coord3::udf();
    Coord3			localpickedpos = Coord3::udf();
    Coord3			worldpickedpos = Coord3::udf();
};


/*!\brief Turns viewer pointer events into EventInfo.

  Handlers run on the visualization thread, before the frame is recorded.
  There is no delayed queue: a handler may update dGBVis objects directly.
*/

mExpClass(vsgBase) EventCatcher : public DataObject
{
    friend class EventCatchHandler;

public:
    static RefMan<EventCatcher> create();
				mCreateDGBVisObj(EventCatcher);

    CNotifier<EventCatcher,const EventInfo&> eventhappened;
    CNotifier<EventCatcher,const EventInfo&> nothandled;

    bool			isHandled() const	{ return ishandled_; }
    void			setHandled()		{ ishandled_ = true; }
    void			reHandle(const EventInfo&);

    void			setCamera(vsg::Camera*);
    vsg::Camera*		camera() const		{ return camera_; }
    void			setSceneRoot(vsg::Node*);
    void			setUtm2Display(const Transformation*);

    void			submit(EventType,OD::ButtonState,
				       bool pressed,bool dragging,
				       const Coord& mousepos,
				       OD::KeyboardKey key=OD::KB_NoKey);

    static bool			forwardToCamera(const EventCatcher&);
				/*!<False once a handler has called
				    setHandled(). EventCatchHandler copies
				    this onto the VSG event. */

    using ModifierQuery = std::function<OD::ButtonState()>;
    static void			setModifierQuery(const ModifierQuery&);
				/*!<Supplies the Shift, Control and Alt state
				    from the window system. Without it, the
				    events carry no modifiers. */

    vsg::Visitor*		eventHandler()	    { return handler_.get(); }

    int				pickedMarkerIndex() const
				{ return markerindex_; }
				/*!<Index stamped on the hit marker, or -1. */
    bool			pickDisplayPos(const Coord&,Coord3&) const;
				/*!<Active-mask hit in display coordinates. */

private:
				~EventCatcher();

    void			publish(EventInfo&);
    static ModifierQuery&	modifierQuery();

    vsg::ref_ptr<vsg::Visitor>	handler_;
    ConstRefMan<Transformation> utm2display_;
    vsg::Camera*		camera_ = nullptr;
    vsg::Node*			sceneroot_ = nullptr;
    bool			ishandled_ = false;
    bool			wasdragging_ = false;
    int				markerindex_ = -1;
};

} // namespace dGBVis
