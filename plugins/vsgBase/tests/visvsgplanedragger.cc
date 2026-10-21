/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 Plane dragger size, normal and in-plane drags, and the edge
 limit. Compiles the dragger when a Vulkan device is present.
-*/

#include "testprog.h"

#include "visvsgdataman.h"
#include "visvsgevent.h"
#include "visvsgplanedragger.h"
#include "thread.h"
#include "vsggeoplanedragger.h"
#include "vsgtesthelper.h"

namespace
{

class Order : public CallBacker
{
public:
    void started( CallBacker* ) { order_.add( "started" ); }
    void motion( CallBacker* )	{ order_.add( "motion" ); }
    void finished( CallBacker* ){ order_.add( "finished" ); }

    BufferString	order_;
};

} // namespace


static bool closeTo( const Coord3& pos, double x, double y, double z )
{
    return pos.distTo( Coord3(x,y,z) ) < 1e-6;
}


static bool testCenterSizeDim()
{
    RefMan<dGBVis::DepthTabPlaneDragger> dragger =
				dGBVis::DepthTabPlaneDragger::create();
    dragger->setDim( 2 );
    dragger->setCenter( Coord3(10,20,30), false );
    dragger->setSize( Coord3(4,5,6), false );
    dragger->setDim( 0 );
    dragger->setCenter( Coord3(1,2,3), false );
    dragger->setSize( Coord3(7,8,9), false );
    dragger->setDim( 2 );
    mRunStandardTest( dragger->getDim()==2 &&
		      closeTo(dragger->center(),10,20,30) &&
		      closeTo(dragger->size(),4,5,6),
		      "Each dimension keeps its center and size" );
    return true;
}


static bool testNormalMove()
{
    RefMan<dGBVis::DepthTabPlaneDragger> dragger =
				dGBVis::DepthTabPlaneDragger::create();
    dragger->setDim( 2 );
    dragger->setCenter( Coord3(1,2,3) );
    dragger->beginDrag( Coord3(1,2,3), true );
    dragger->dragTo( Coord3(9,8,7) );
    dragger->endDrag();
    mRunStandardTest( closeTo(dragger->center(),1,2,7),
		      "Normal drag changes only that axis" );

    RefMan<dGBVis::EventCatcher> catcher = dGBVis::EventCatcher::create();
    dragger->setEventCatcher( catcher.ptr() );
    dragger->setCenter( Coord3(0,0,0) );
    dragger->setSize( Coord3(4,4,0) );
    visBase::EventInfo press;
    press.type = dGBVis::MouseClick;
    press.pressed = true;
    press.buttonstate_ = OD::LeftButton;
    press.pickedobjids += dragger->id();
    press.mouseline = Line3( 0., -10., 5., 0., 10., -5. );
    catcher->reHandle( press );

    visBase::EventInfo move;
    move.type = dGBVis::MouseMovement;
    move.buttonstate_ = press.buttonstate_;
    move.mouseline = Line3( 0., -10., 5., 0., 10., -2. );
    catcher->reHandle( move );
    mRunStandardTest( closeTo(dragger->center(),0,0,3),
		      "Plain drag moves the plane along its normal" );

    visBase::EventInfo release;
    release.type = dGBVis::MouseClick;
    release.pressed = false;
    release.buttonstate_ = press.buttonstate_;
    release.mouseline = move.mouseline;
    catcher->reHandle( release );
    return true;
}


static bool testClamp()
{
    RefMan<dGBVis::DepthTabPlaneDragger> dragger =
				dGBVis::DepthTabPlaneDragger::create();
    dragger->setDim( 2 );
    dragger->setSize( Coord3(1,1,0) );
    dragger->setSpaceLimits( Interval<float>(-1.f,1.f),
			     Interval<float>(-1.f,1.f),
			     Interval<float>(0.f,10.f) );
    dragger->setCenter( Coord3(0,0,1) );
    dragger->beginDrag( Coord3(0,0,1), true );
    dragger->dragTo( Coord3(0,0,50) );
    dragger->endDrag();
    mRunStandardTest( closeTo(dragger->center(),0,0,10),
		      "Normal drag stops at the space limit" );

    dragger->beginDrag( Coord3(0,0,10), false );
    dragger->dragTo( Coord3(40,-40,10) );
    dragger->endDrag();
    mRunStandardTest( closeTo(dragger->center(),0.5,-0.5,10),
		      "In-plane drag stops when an edge hits the limit" );

    RefMan<dGBVis::DepthTabPlaneDragger> edged =
				dGBVis::DepthTabPlaneDragger::create();
    edged->setDim( 2 );
    edged->setSize( Coord3(1,1,0) );
    edged->setSpaceLimits( Interval<float>(-2.f,2.f),
			   Interval<float>(-2.f,2.f),
			   Interval<float>(0.f,10.f) );
    edged->setCenter( Coord3(0,0,5) );
    edged->beginDrag( Coord3(0,0,5), false );
    edged->dragTo( Coord3(40,-40,5) );
    edged->endDrag();
    mRunStandardTest( closeTo(edged->center(),1.5,-1.5,5),
		      "Edges stay inside the space limit" );
    return true;
}


static bool testScaleTabs()
{
    vsg::ref_ptr<vsgGeo::TabPlaneDragger> plane =
					vsgGeo::createTabPlaneDragger();
    plane->setSize( vsg::dvec3(2.,2.,0.) );
    plane->setNormalAxis( 2 );
    const vsg::dvec3 origin( 1., 1., 5. );
    const vsg::dvec3 direction( 0., 0., -1. );
    mRunStandardTest( plane->hit(origin,direction).part==
		      vsgGeo::TabPlaneDragger::Handle,
		      "A corner is a handle" );

    plane->removeScaleTabs();
    mRunStandardTest( plane->hit(origin,direction).part==
		      vsgGeo::TabPlaneDragger::Plane,
		      "removeScaleTabs leaves the corner as plane" );
    return true;
}


static bool testGpuDrag()
{
    vsgTest::OffscreenViewer viewer( 64, 64, true );
    if ( !viewer.isOK() )
    {
	logStream() << "Dragger compile skipped, no Vulkan device: "
		    << viewer.errMsg().c_str() << od_endl;
	return true;
    }

    dGBVis::DataObject::setCommonViewer( &viewer.viewer() );
    RefMan<dGBVis::DepthTabPlaneDragger> dragger =
				dGBVis::DepthTabPlaneDragger::create();
    dragger->setDim( 1 );
    dragger->setSize( Coord3(1.6,0,1.6) );
    dragger->setCenter( Coord3(0,0,0) );
    viewer.scene().addChild( vsg::ref_ptr<vsg::Node>(dragger->vsgNode()) );

    RefMan<dGBVis::EventCatcher> catcher = dGBVis::EventCatcher::create();
    catcher->setCamera( &viewer.camera() );
    catcher->setSceneRoot( &viewer.scene() );
    dragger->setEventCatcher( catcher.ptr() );

    Order order;
    dragger->started.notify( mCB(&order,Order,started) );
    dragger->motion.notify( mCB(&order,Order,motion) );
    dragger->finished.notify( mCB(&order,Order,finished) );

    if ( !viewer.compile() || !viewer.frame() )
    {
	mRunStandardTest( false, "Dragger compiles" );
	return false;
    }

    const int x = int(viewer.width()/2);
    const int y = int(viewer.height()/2);
    catcher->submit( dGBVis::MouseClick, OD::LeftButton, true, false,
		     Coord(x,y) );
    catcher->submit( dGBVis::MouseMovement, OD::LeftButton, false, true,
		     Coord(x+4,y) );
    catcher->submit( dGBVis::MouseClick, OD::LeftButton, false, false,
		     Coord(x+4,y) );

    BufferString msg( "Order: ", order.order_.buf(), " messages: " );
    msg.add( viewer.nrValidationMessages() );
    mRunStandardTestWithError( order.order_=="startedmotionfinished" &&
			       viewer.nrValidationMessages()==0,
			       "Drag notifies started, motion, finished",
			       msg );
    dragger->started.remove( mCB(&order,Order,started) );
    dragger->motion.remove( mCB(&order,Order,motion) );
    dragger->finished.remove( mCB(&order,Order,finished) );
    dGBVis::DataObject::setCommonViewer( nullptr );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );
    dGBVis::initStdClasses();
    if ( !testCenterSizeDim() || !testNormalMove() || !testClamp() ||
	 !testScaleTabs() || !testGpuDrag() )
	return 1;

    return 0;
}
