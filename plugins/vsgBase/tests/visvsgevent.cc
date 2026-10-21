/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 The event catcher publishes the event it is given when no camera
 is attached. No GPU.
-*/

#include "testprog.h"

#include "visvsgdataman.h"
#include "visvsgevent.h"
#include "thread.h"

#include <vsg/ui/PointerEvent.h>

namespace
{

class Recorder : public CallBacker
{
public:
    void onEvent( CallBacker* cb )
    {
	mCBCapsuleUnpack(const visBase::EventInfo&,info,cb);
	last_ = info;
	count_++;
	if ( handle_ && catcher_ )
	    catcher_->setHandled();
    }

    dGBVis::EventCatcher*	catcher_ = nullptr;
    visBase::EventInfo		last_;
    int				count_ = 0;
    bool			handle_ = false;
};

} // namespace


static bool testEventFields()
{
    RefMan<dGBVis::EventCatcher> catcher = dGBVis::EventCatcher::create();
    Recorder recorder;
    catcher->eventhappened.notify( mCB(&recorder,Recorder,onEvent) );

    const OD::ButtonState shiftleft = OD::ButtonState(
		int(OD::LeftButton) | int(OD::ShiftButton) );
    catcher->submit( dGBVis::MouseClick, shiftleft, true, false,
		     Coord(4,5) );
    mRunStandardTest( recorder.count_==1 && recorder.last_.pressed &&
		      !recorder.last_.dragging &&
		      (int(recorder.last_.buttonstate_) & int(OD::ShiftButton)),
		      "Press keeps Shift" );

    catcher->submit( dGBVis::MouseMovement,
		 OD::ButtonState(int(OD::LeftButton)|int(OD::ControlButton)),
		 false, true, Coord(6,7) );
    mRunStandardTest( recorder.last_.dragging &&
		      (int(recorder.last_.buttonstate_) &
		       int(OD::ControlButton)),
		      "Drag keeps Ctrl" );

    catcher->submit( dGBVis::MouseClick,
		     OD::ButtonState(int(OD::LeftButton)|int(OD::AltButton)),
		     false, false, Coord(6,7) );
    mRunStandardTest( recorder.last_.dragging &&
		      !recorder.last_.pressed &&
		      (int(recorder.last_.buttonstate_) & int(OD::AltButton)),
		      "Release reports the drag and keeps Alt" );

    catcher->submit( dGBVis::MouseDoubleClick, OD::LeftButton, true, false,
		     Coord(1,1) );
    mRunStandardTest( recorder.last_.type==dGBVis::MouseDoubleClick,
		      "Double click is published" );

    catcher->submit( dGBVis::Keyboard, OD::NoButton, true, false, Coord::udf(),
		     OD::KB_Space );
    mRunStandardTest( recorder.last_.type==dGBVis::Keyboard &&
		      recorder.last_.key_==OD::KB_Space,
		      "Key is published" );

    catcher->eventhappened.remove( mCB(&recorder,Recorder,onEvent) );
    return true;
}


static bool testHandled()
{
    RefMan<dGBVis::EventCatcher> catcher = dGBVis::EventCatcher::create();
    Recorder recorder;
    recorder.catcher_ = catcher.ptr();
    recorder.handle_ = true;
    catcher->eventhappened.notify( mCB(&recorder,Recorder,onEvent) );
    catcher->submit( dGBVis::MouseClick, OD::LeftButton, true, false,
		     Coord(0,0) );
    mRunStandardTest( !dGBVis::EventCatcher::forwardToCamera(*catcher),
		      "setHandled stops the trackball" );

    recorder.handle_ = false;
    catcher->submit( dGBVis::MouseClick, OD::LeftButton, false, false,
		     Coord(0,0) );
    mRunStandardTest( dGBVis::EventCatcher::forwardToCamera(*catcher),
		      "An unhandled event reaches the trackball" );

    catcher->eventhappened.remove( mCB(&recorder,Recorder,onEvent) );
    return true;
}


static bool testTakenEventsAreNotPublished()
{
    RefMan<dGBVis::EventCatcher> catcher = dGBVis::EventCatcher::create();
    Recorder recorder;
    catcher->eventhappened.notify( mCB(&recorder,Recorder,onEvent) );

    const vsg::clock::time_point now = vsg::clock::now();
    vsg::ref_ptr<vsg::ButtonPressEvent> press = vsg::ButtonPressEvent::create(
		nullptr, now, 4, 5, vsg::BUTTON_MASK_1, 1 );
    press->handled = true;
    press->accept( *catcher->eventHandler() );
    vsg::ref_ptr<vsg::MoveEvent> move = vsg::MoveEvent::create(
		nullptr, now, 6, 7, vsg::BUTTON_MASK_1 );
    move->handled = true;
    move->accept( *catcher->eventHandler() );
    vsg::ref_ptr<vsg::ButtonReleaseEvent> release =
		vsg::ButtonReleaseEvent::create( nullptr, now, 6, 7,
						 vsg::BUTTON_MASK_OFF, 1 );
    release->handled = true;
    release->accept( *catcher->eventHandler() );
    mRunStandardTest( recorder.count_==0,
		      "A press, move and release that another handler took "
		      "are not published" );

    vsg::ref_ptr<vsg::ButtonPressEvent> open = vsg::ButtonPressEvent::create(
		nullptr, now, 4, 5, vsg::BUTTON_MASK_1, 1 );
    open->accept( *catcher->eventHandler() );
    mRunStandardTest( recorder.count_==1,
		      "A press that no other handler took is published" );

    catcher->eventhappened.remove( mCB(&recorder,Recorder,onEvent) );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );
    dGBVis::initStdClasses();
    if ( !testEventFields() || !testHandled() ||
	 !testTakenEventsAreNotPublished() )
	return 1;

    return 0;
}
