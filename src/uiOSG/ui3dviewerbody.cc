/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ui3dviewerbody.h"
#include "ui3dviewer.h"

#include "filepath.h"
#include "iopar.h"
#include "keybindings.h"
#include "keyboardevent.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odgraphicswindow.h"
#include "ptrman.h"
#include "settingsaccess.h"
#include "survinfo.h"
#include "swapbuffercallback.h"
#include "timer.h"
#include "visosg.h"

#include "uicursor.h"
#include "uimain.h"
#include "uimouseeventblockerbygesture.h"
#include "uiobjbody.h"
#include "visaxes.h"
#include "viscamera.h"
#include "visdatagroup.h"
#include "visdataman.h"
#include "vispolygonselection.h"
#include "visscenecoltab.h"
#include "visthumbwheel.h"

#include <QGesture>
#include <QGestureEvent>
#include <QPainter>
#include <QTabletEvent>

#include <osg/MatrixTransform>
#include <osg/Version>
#include <osgGeo/ThumbWheel>
#include <osgGeo/TrackballManipulator>
#include <osgViewer/CompositeViewer>
#include <osgViewer/View>
#include <osgViewer/ViewerEventHandlers>

#include <math.h>


static const char* sKeydTectScene()	{ return "dTect.Scene."; }
static const char* sKeyManipCenter()	{ return "Manipulator Center"; }
static const char* sKeyManipDistance()	{ return "Manipulator Distance"; }
static const char* preOdHomePosition()	{return "Home position.Aspect ratio"; }
static const char* sKeyHomePos()	{ return "Home position"; }
static const char* sKeyCameraRotation() { return "Camera Rotation"; }

class TrackBallManipulatorMessenger : public osg::NodeCallback
{
public:
    TrackBallManipulatorMessenger( ui3DViewerBody* t )
        : viewerbody_( t )
    {}

    void	operator()(osg::Node*,osg::NodeVisitor*) override;

    void	detach() { viewerbody_ = nullptr; }

protected:
			TrackBallManipulatorMessenger()    {}

    ui3DViewerBody*	viewerbody_;
};


void TrackBallManipulatorMessenger::operator()( osg::Node* node,
						osg::NodeVisitor* nv )
{
    if ( nv && viewerbody_ )
    {
	auto* tnv = (osgGeo::TrackballEventNodeVisitor*) nv;
	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::RotateStart )
	    viewerbody_->setViewModeCursor( ui3DViewerBody::RotateCursor );
	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::PanStart )
	    viewerbody_->setViewModeCursor( ui3DViewerBody::PanCursor );
	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::ZoomStart )
	    viewerbody_->setViewModeCursor( ui3DViewerBody::ZoomCursor );
	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::MoveStart )
	    viewerbody_->setViewModeCursor( ui3DViewerBody::HoverCursor );
	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::MoveStop )
	    viewerbody_->setViewModeCursor( ui3DViewerBody::HoverCursor );

	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::Moving )
	{
	    viewerbody_->notifyManipulatorMovement( tnv->_deltahorangle,
						    tnv->_deltavertangle,
						    tnv->_distfactor );
	}
    }
}


//--------------------------------------------------------------------------


ui3DViewerBody::ui3DViewerBody( ui3DViewer& h, uiParent* parnt )
    : uiObjectBody(parnt,0)
    , viewalltimer_(new Timer)
    , handle_(h)
    , printpar_(*new IOPar)
    , offscreenrenderswitch_(new osg::Switch)
    , viewport_(new osg::Viewport)
    , offscreenrenderhudswitch_(new osg::Switch)
    , mouseeventblocker_(*new uiMouseEventBlockerByGestures(500))
    , manipmessenger_(new TrackBallManipulatorMessenger(this))
    , keybindman_(*new KeyBindMan)
{
    visBase::refOsgPtr( manipmessenger_ );
    visBase::refOsgPtr( offscreenrenderswitch_ );
    visBase::refOsgPtr( offscreenrenderhudswitch_ );
    visBase::refOsgPtr( viewport_ );
    eventfilter_.addEventType( uiEventFilter::KeyPress );
    eventfilter_.addEventType( uiEventFilter::Resize );
    eventfilter_.addEventType( uiEventFilter::Show );
    eventfilter_.addEventType( uiEventFilter::Gesture );

    auto* glw = new ODGLWidget( parnt->pbody()->managewidg() );

    mouseeventblocker_.attachToQObj( glw );
    eventfilter_.attachToQObj( glw );

    graphicswin_ = new ODGraphicsWindow( glw );

    swapcallback_ = new SwapCallback( this );
    visBase::refOsgPtr( swapcallback_ );
    graphicswin_->setSwapCallback( swapcallback_ );

    setStretch(2,2);

    setupHUD();
    setupView();
    setupTouch();

    mAttachCB( eventfilter_.eventhappened, ui3DViewerBody::qtEventCB );
    mAttachCB( uiMain::keyboardEventHandler().keyPressed,
	       ui3DViewerBody::setFocusCB );
    mAttachCB( viewalltimer_->tick, ui3DViewerBody::viewAllCB );
}


ui3DViewerBody::~ui3DViewerBody()
{
    detachAllNotifiers();
    delete &keybindman_;
    delete viewalltimer_;

    manipmessenger_->detach();
    visBase::unRefOsgPtr( manipmessenger_ );

    handle_.destroyed.trigger( handle_ );
    delete &printpar_;
    if ( compositeviewer_ )
    {
	compositeviewer_->removeView( view_ );
	compositeviewer_->removeView( hudview_ );
    }

    visBase::unRefOsgPtr( compositeviewer_ );
    visBase::unRefOsgPtr( viewport_ );
    visBase::unRefOsgPtr( offscreenrenderswitch_ );
    visBase::unRefOsgPtr( offscreenrenderhudswitch_ );
    visBase::unRefOsgPtr( swapcallback_ );
}


const mQtclass(QWidget)* ui3DViewerBody::qwidget_() const
{
    return graphicswin_->getGLWidget();
}


osgViewer::GraphicsWindow& ui3DViewerBody::getGraphicsWindow()
{
    return *graphicswin_;
}


osg::GraphicsContext* ui3DViewerBody::getGraphicsContext()
{
    return graphicswin_;
}


void ui3DViewerBody::removeSwapCallback( CallBacker* )
{
    if ( swapcallback_ )
	getGraphicsContext()->setSwapCallback( nullptr );

    visBase::unRefAndNullOsgPtr( swapcallback_ );
}


void ui3DViewerBody::viewAllCB( CallBacker* )
{
    viewAll( true );
}


#define mMainCameraOrder    0
#define mHudCameraOrder	    (mMainCameraOrder+1)

void ui3DViewerBody::setupHUD()
{
    if ( hudview_ )
	return;

    vishudcamera_ = visBase::Camera::create();
    osg::ref_ptr<osg::Camera> hudcamera = vishudcamera_->osgCamera();
    hudcamera->setGraphicsContext( getGraphicsContext() );
    hudcamera->setName( "HUD Camera" );
    hudcamera->setProjectionMatrix( osg::Matrix::ortho2D(0,1024,0,768) );
    hudcamera->setViewport( viewport_ );
    hudcamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    hudcamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    hudcamera->setViewMatrix(osg::Matrix::identity());
    hudcamera->setProjectionResizePolicy( osg::Camera::FIXED );

    //draw subgraph after main camera view.
    hudcamera->setRenderOrder( osg::Camera::POST_RENDER, mHudCameraOrder );

    //we don't want the camera to grab event focus from the viewers main cam(s).
    hudcamera->setAllowEventFocus(false);

    hudscene_ = visBase::DataObjectGroup::create();

    hudview_ = new osgViewer::View;
    hudview_->setCamera( hudcamera );
    offscreenrenderhudswitch_->removeChild(
			0, offscreenrenderhudswitch_->getNumChildren() );
    offscreenrenderhudswitch_->addChild( hudscene_->osgNode() );
    hudview_->setSceneData( offscreenrenderhudswitch_ );

    if ( !compositeviewer_ )
    {
	compositeviewer_ = getCompositeViewer();
	visBase::refOsgPtr( compositeviewer_ );
    }

    compositeviewer_->addView( hudview_ );

    horthumbwheel_ = visBase::ThumbWheel::create();
    hudscene_->addObject( horthumbwheel_.ptr() );
    mAttachCB( horthumbwheel_->rotation, ui3DViewerBody::thumbWheelRotationCB);

    verthumbwheel_ = visBase::ThumbWheel::create();
    hudscene_->addObject( verthumbwheel_.ptr() );
    mAttachCB( verthumbwheel_->rotation, ui3DViewerBody::thumbWheelRotationCB);

    distancethumbwheel_ = visBase::ThumbWheel::create();
    hudscene_->addObject( distancethumbwheel_.ptr() );
    mAttachCB( distancethumbwheel_->rotation,
               ui3DViewerBody::thumbWheelRotationCB);

    FontData annotfontdata;
    annotfontdata.setPointSize( 18 );

    if ( !axes_ )
    {
	axes_ = visBase::Axes::create();
	axes_->setSize( 5.0f, 55.0f );
	axes_->setAnnotationTextSize( FontData::defaultPointSize() + 6 );
	axes_->setAnnotationFont( annotfontdata );
	hudscene_->addObject( axes_.ptr() );
	if ( camera_ )
	    axes_->setPrimaryCamera( camera_.ptr() );
    }

    if ( !polygonselection_ )
    {
	polygonselection_ = visBase::PolygonSelection::create();
	hudscene_->addObject( polygonselection_.ptr() );
	polygonselection_->setHUDCamera( vishudcamera_.ptr() );
    }

    if ( !visscenecoltab_ )
    {
	visscenecoltab_ = visBase::SceneColTab::create();
	hudscene_->addObject( visscenecoltab_.ptr() );
	visscenecoltab_->setAnnotFont( annotfontdata );
	visscenecoltab_->turnOn( false );
	visscenecoltab_->setPos( visBase::SceneColTab::Bottom );
    }
}


void ui3DViewerBody::setupTouch()
{
    if ( getGraphicsWindow().getEventQueue() )
	getGraphicsWindow().getEventQueue()->setFirstTouchEmulatesMouse(false);

    qwidget()->grabGesture(Qt::PinchGesture);

}


osgGeo::TrackballManipulator* ui3DViewerBody::getCameraManipulator() const
{
    if ( !view_ )
	return nullptr;

    return sCast(osgGeo::TrackballManipulator*,view_->getCameraManipulator());
}


void ui3DViewerBody::setMouseWheelZoomFactor( float factor )
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( !manip )
	return;

    if ( getReversedMouseWheelDirection() )
	manip->setWheelZoomFactor( factor );
    else
	manip->setWheelZoomFactor( -factor );
}


float ui3DViewerBody::getMouseWheelZoomFactor() const
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( !manip )
	return false;

    return fabs( manip->getWheelZoomFactor() );
}


void ui3DViewerBody::setReversedMouseWheelDirection( bool reversed )
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( !manip )
	return;

    const float zoomfactor = fabs( manip->getWheelZoomFactor() );

    if ( reversed )
	manip->setWheelZoomFactor( zoomfactor );
    else
	manip->setWheelZoomFactor( -zoomfactor );
}


bool ui3DViewerBody::getReversedMouseWheelDirection() const
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    return manip ? manip->getWheelZoomFactor()>0 : false;
}


void ui3DViewerBody::setupView()
{
    camera_ = visBase::Camera::create();
    if ( axes_ )
	axes_->setPrimaryCamera( camera_.ptr() );

    RefMan<visBase::Scene> scene = getScene();
    if ( scene )
	scene->setCamera( camera_.ptr() );

    mDynamicCastGet(osg::Camera*, osgcamera, camera_->osgNode(true) );
    osgcamera->setGraphicsContext( getGraphicsContext() );
    osgcamera->setClearColor( osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f) );
    osgcamera->setViewport( viewport_ );
    osgcamera->setRenderOrder(osg::Camera::POST_RENDER, mMainCameraOrder );
    osgcamera->setNearFarRatio( 0.002 );	// default is 0.0005

    view_ = new osgViewer::View;
    view_->setCamera( osgcamera );
    view_->setSceneData( offscreenrenderswitch_ );
    osgViewer::StatsHandler* statshandler = new osgViewer::StatsHandler;
    statshandler->setKeyEventTogglesOnScreenStats( 'g' );
    statshandler->setKeyEventPrintsOutStats( 'G' );
    statshandler->getCamera()->setAllowEventFocus( false );
    view_->addEventHandler( statshandler );

    // Unlike Coin, default OSG headlight has zero ambiance
    view_->getLight()->setAmbient( osg::Vec4(0.6f,0.6f,0.6f,1.0f) );

    osg::ref_ptr<osgGeo::TrackballManipulator> manip =
	new osgGeo::TrackballManipulator(
	    osgGA::StandardManipulator::DEFAULT_SETTINGS );

    keybindman_.setTrackballManipulator( manip );

    manip->useLeftMouseButtonForAllMovement( true, true );
    manip->enableKeyHandling( false );	// No space key to restore home view
    manip->addMovementCallback( manipmessenger_ );
    manip->setBoundTraversalMask( visBase::cBBoxTraversalMask() );
    manip->setIntersectTraversalMask( visBase::cIntersectionTraversalMask() );
    manip->setAnimationTime( 0.5 );

    manip->setAutoComputeHomePosition( false );

    view_->setCameraManipulator( manip.get() );

    enableDragging( isViewMode() );

    if ( !compositeviewer_ )
    {
	compositeviewer_ = getCompositeViewer();
	visBase::refOsgPtr( compositeviewer_ );
    }

    compositeviewer_->addView( view_ );

    // To put exaggerated bounding sphere radius offside
    manip->setMinimumDistance( 0 );

    if ( polygonselection_ )
	polygonselection_->setPrimaryCamera( camera_.ptr() );

    view_->getSceneData()->addEventCallback(new osgGeo::ThumbWheelEventHandler);
    enableThumbWheelHandling( true );

    bool reversezoom = false;
    Settings::common().getYN(SettingsAccess::sKeyMouseWheelReversal(),
			     reversezoom);

    setReversedMouseWheelDirection( reversezoom );

    // Camera projection must be initialized before computing home position
    reSizeEvent( nullptr );
}


visBase::Scene* ui3DViewerBody::getScene()
{
    return scene_.get().ptr();
}


const visBase::Scene* ui3DViewerBody::getScene() const
{
    return mSelf().getScene();
}


void ui3DViewerBody::enableThumbWheelHandling( bool yn,
					       const visBase::ThumbWheel* tw )
{
    if ( !tw )	// set all
    {
	enableThumbWheelHandling( yn, horthumbwheel_.ptr() );
	enableThumbWheelHandling( yn, verthumbwheel_.ptr() );
	enableThumbWheelHandling( yn, distancethumbwheel_.ptr() );
    }
    else if ( view_ && view_->getSceneData() )
    {
	osgGeo::ThumbWheelEventHandler* handler = 0;
#if OSG_MIN_VERSION_REQUIRED(3,3,2)
	osg::Callback* cb = view_->getSceneData()->getEventCallback();
#else
	osg::NodeCallback* cb = view_->getSceneData()->getEventCallback();
#endif
	while ( cb && !handler )
	{
	    handler = dCast(osgGeo::ThumbWheelEventHandler*,cb);
	    cb = cb->getNestedCallback();
	}

	if ( handler )
	{
	    osgGeo::ThumbWheel* osgtw = (osgGeo::ThumbWheel*) tw->osgNode(true);
	    handler->removeThumbWheel( osgtw );
	    if ( yn )
		handler->addThumbWheel( osgtw );
	}
    }
}


uiObject& ui3DViewerBody::uiObjHandle()
{ return handle_; }


osgViewer::CompositeViewer* ui3DViewerBody::getCompositeViewer()
{
    mDefineStaticLocalObject( Threads::Lock, lock, (true) );
    Threads::Locker locker ( lock );
    mDefineStaticLocalObject( osg::ref_ptr<osgViewer::CompositeViewer>,
	viewer, = nullptr );

    if ( !viewer || viewer->done() )
    {
	osg::ref_ptr<osgViewer::CompositeViewer> updatedviewer = viewer;
	if ( !updatedviewer )
	    updatedviewer = new osgViewer::CompositeViewer;
	else
	    updatedviewer->setDone( false );

	updatedviewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
	updatedviewer->getEventVisitor()->setTraversalMask(
	    visBase::cEventTraversalMask() );
	updatedviewer->setRunFrameScheme( osgViewer::ViewerBase::ON_DEMAND );
	updatedviewer->setKeyEventSetsDone( 0 );

	if ( !viewer )
	{
	    viewer = updatedviewer;
	    setViewer( viewer.get() );
	    visBase::DataObject::setCommonViewer( viewer );
	}
    }

    return viewer.get();
}


osg::Camera* ui3DViewerBody::getOsgCamera()
{
    return view_ ? view_->getCamera() : nullptr;
}


const osg::Camera* ui3DViewerBody::getOsgCamera() const
{
    return mSelf().getOsgCamera();
}

#if OSG_VERSION_LESS_THAN(3,3,0)

osgGA::GUIEventAdapter::TouchPhase
    translateQtGestureState( Qt::GestureState state )
{
    osgGA::GUIEventAdapter::TouchPhase touchPhase;
    switch ( state )
    {
	case Qt::GestureStarted:
	    touchPhase = osgGA::GUIEventAdapter::TOUCH_BEGAN;
	    break;
	case Qt::GestureUpdated:
	    touchPhase = osgGA::GUIEventAdapter::TOUCH_MOVED;
	    break;
	case Qt::GestureFinished:
	case Qt::GestureCanceled:
	    touchPhase = osgGA::GUIEventAdapter::TOUCH_ENDED;
	    break;
	default:
	    touchPhase = osgGA::GUIEventAdapter::TOUCH_UNKNOWN;
    };

    return touchPhase;
}



void ui3DViewerBody::handleGestureEvent( QGestureEvent* qevent )
{
    bool accept = false;

    if ( QPinchGesture* pinch =
	    static_cast<QPinchGesture *>(qevent->gesture(Qt::PinchGesture) ) )
    {
	const QPointF qcenterf = pinch->centerPoint();
	const QPoint pinchcenter = qwidget()->mapFromGlobal(qcenterf.toPoint());
	const osg::Vec2 osgcenter( pinchcenter.x(),
	    qwidget()->height() - pinchcenter.y() );
	const float angle = pinch->totalRotationAngle();
	const float scale = pinch->totalScaleFactor();

	//We don't have absolute positions of the two touches, only a scale and
	//rotation. Hence we create pseudo-coordinates which are reasonable, and
	//centered around the real position
	const float radius = (qwidget()->width()+qwidget()->height())/4;
	const osg::Vec2 vector(scale*cos(angle)*radius,scale*sin(angle)*radius);
	const osg::Vec2 p0 = osgcenter+vector;
	const osg::Vec2 p1 = osgcenter-vector;

	osg::ref_ptr<osgGA::GUIEventAdapter> event = 0;
	const osgGA::GUIEventAdapter::TouchPhase touchPhase =
		translateQtGestureState( pinch->state() );
	if ( touchPhase==osgGA::GUIEventAdapter::TOUCH_BEGAN )
	{
	    event = getGraphicsWindow().getEventQueue()->touchBegan(0 ,
					    touchPhase, p0[0], p0[1] );
	}
	else if ( touchPhase==osgGA::GUIEventAdapter::TOUCH_MOVED )
	{
	    event = getGraphicsWindow().getEventQueue()->touchMoved( 0,
					    touchPhase, p0[0], p0[1] );
	}
	else
	{
	    event = getGraphicsWindow().getEventQueue()->touchEnded( 0,
					    touchPhase, p0[0], p0[1], 1 );
	}

	if ( event )
	{
	    event->addTouchPoint( 1, touchPhase, p1[0], p1[1] );
	    accept = true;
	}
    }

    if ( accept )
	qevent->accept();
}

#endif // Qtgesture

#define mLongSideDistance	15
#define mShortSideDistance	40

#define mThumbWheelLen		100
#define mThumbWheelWidth	15

#define mZCoord			-1


void ui3DViewerBody::reSizeEvent(CallBacker*)
{
    const mQtclass(QWidget)* widget = qwidget();
    if ( !widget )
	return;

    if ( !camera_ )
	return;

    osg::ref_ptr<osg::Camera> osgcamera = getOsgCamera();

    if ( !osgcamera )
	return;

    hudview_->getCamera()->setProjectionMatrix(
	osg::Matrix::ortho2D(0,widget->width(),0,widget->height() ));

    const float longsideoffset = mLongSideDistance+mThumbWheelWidth/2.0;
    const float shortsideoffset = mShortSideDistance+mThumbWheelLen/2.0;

    horthumbwheel_->setPosition( true, shortsideoffset, longsideoffset,
                                 mThumbWheelLen, mThumbWheelWidth, mZCoord );
     verthumbwheel_->setPosition( false, longsideoffset, shortsideoffset,
                                 mThumbWheelLen, mThumbWheelWidth, mZCoord );
     distancethumbwheel_->setPosition( false, longsideoffset,
			mMAX(widget->height()-shortsideoffset,shortsideoffset),
			mThumbWheelLen, mThumbWheelWidth, mZCoord );

    const float offset = axes_->getLength() + 10;
    axes_->setPosition( widget->width()-offset, offset );

    if ( visscenecoltab_ )
	visscenecoltab_->setWindowSize( widget->width(), widget->height() );
}


void ui3DViewerBody::thumbWheelRotationCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( float, deltaangle, caller, cb );
    if ( caller==horthumbwheel_.ptr() )
    {
	uiRotate( deltaangle, true );
    }
    else if ( caller==verthumbwheel_.ptr() )
    {
	uiRotate( deltaangle, false );
    }
    else if ( caller==distancethumbwheel_.ptr() )
    {
	osg::ref_ptr<osgGeo::TrackballManipulator> manip =
							getCameraManipulator();
	float change = -deltaangle/M_PI;
	if ( manip && manip->getWheelZoomFactor()<0 )
	    change *= -1;

	manip->changeDistance( change );
    }
}


void ui3DViewerBody::toggleViewMode(CallBacker* cb )
{
    setViewMode( !isViewMode(), true );
}


void ui3DViewerBody::showRotAxis( bool yn )
{
    axes_->turnOn( yn );
}


bool ui3DViewerBody::isAxisShown() const
{
    return axes_->isOn();
}


void ui3DViewerBody::setWheelDisplayMode( OD::WheelMode mode )
{
    const bool doshow = mode != OD::WheelMode::Never;
    horthumbwheel_->turnOn( doshow );
    verthumbwheel_->turnOn( doshow );
    distancethumbwheel_->turnOn( doshow );
    enableThumbWheelHandling( doshow );

    const bool enablefadeinout = mode==OD::WheelMode::OnHover;
    horthumbwheel_->enableFadeInOut( enablefadeinout );
    verthumbwheel_->enableFadeInOut( enablefadeinout );
    distancethumbwheel_->enableFadeInOut( enablefadeinout );
}


OD::WheelMode ui3DViewerBody::getWheelDisplayMode() const
{
    if ( !horthumbwheel_->isOn() )
	return OD::WheelMode::Never;

    if ( horthumbwheel_->isFadeInOutEnabled() )
	return OD::WheelMode::OnHover;

    return OD::WheelMode::Always;
}


void ui3DViewerBody::setAnnotColor( const OD::Color& col )
{
    axes_->setAnnotationColor( col );

    horthumbwheel_->setAnnotationColor( col );
    verthumbwheel_->setAnnotationColor( col );
    distancethumbwheel_->setAnnotationColor( col );
}


void ui3DViewerBody::setAnnotationFont( const FontData& fd )
{
    visscenecoltab_->setAnnotFont( fd );
    axes_->setAnnotationFont( fd );
}


visBase::PolygonSelection* ui3DViewerBody::getPolygonSelector()
{
    return polygonselection_.ptr();
}


visBase::SceneColTab* ui3DViewerBody::getSceneColTab()
{
    return visscenecoltab_.ptr();
}


void ui3DViewerBody::qtEventCB( CallBacker* )
{
#if OSG_VERSION_LESS_THAN(3,3,0)
    if ( eventfilter_.getCurrentEventType()== uiEventFilter::Gesture )
    {
	QGestureEvent* gestureevent =
	    static_cast<QGestureEvent*> ( eventfilter_.getCurrentEvent() );
	handleGestureEvent( gestureevent );
    }
#endif

    if ( eventfilter_.getCurrentEventType()==uiEventFilter::Resize ||
	 eventfilter_.getCurrentEventType()==uiEventFilter::Show )
    {
	reSizeEvent( 0 );
    }

    if ( eventfilter_.getCurrentEventType() == uiEventFilter::KeyPress )
    {
	const QKeyEvent* keyevent =
	    (const QKeyEvent*) eventfilter_.getCurrentEvent();

	if ( keyevent->key()==Qt::Key_Escape )
	    toggleViewMode( 0 );
	if ( keyevent->key()==Qt::Key_PageUp )
	    handle_.pageupdown.trigger( true );
	if ( keyevent->key()==Qt::Key_PageDown )
	    handle_.pageupdown.trigger( false );
    }
}


void ui3DViewerBody::setFocusCB( CallBacker* )
{
    if ( !uiMain::keyboardEventHandler().hasEvent() )
	return;

    // Need focus to show mod key dependent act-mode cursors
    // or toggle between picking and positioning in act mode
    const KeyboardEvent& kbe = uiMain::keyboardEventHandler().event();
    if ( kbe.key_==OD::KB_Shift || kbe.key_==OD::KB_Control
	|| kbe.key_==OD::KB_Alt || kbe.key_==OD::KB_Space )
    {
	if ( !qwidget()->hasFocus() && qwidget()->underMouse() )
	    qwidget()->setFocus();
    }
}


bool ui3DViewerBody::isViewMode() const
{
    ConstRefMan<visBase::Scene> scene = getScene();
    return scene && !scene->isPickable();
}


void ui3DViewerBody::setViewMode( bool yn, bool trigger )
{
    enableDragging( yn );

    RefMan<visBase::Scene> scene = getScene();
    if ( scene )
	scene->setPickable( !yn );

    setViewModeCursor( HoverCursor );

    if ( trigger )
	handle_.viewmodechanged.trigger( handle_ );
}


void ui3DViewerBody::setViewModeCursor( ViewModeCursor viewmodecursor )
{
    MouseCursor cursor;

    if ( viewmodecursor == RotateCursor )
	cursor.shape_ = MouseCursor::Rotator;
    else if ( viewmodecursor == PanCursor )
	cursor.shape_ = MouseCursor::SizeAll;
    else if ( viewmodecursor == ZoomCursor )
	cursor.shape_ = MouseCursor::SizeVer;
    else if ( viewmodecursor==HoverCursor && isViewMode() )
	cursor.shape_ = MouseCursor::PointingHand;
    else
    {
	cursor.shape_ = MouseCursor::Arrow;
	actmodecursor_.shape_ = MouseCursor::NotSet;
    }

    mQtclass(QCursor) qcursor;
    uiCursorManager::fillQCursor( cursor, qcursor );
    qwidget()->setCursor( qcursor );
}


Coord3 ui3DViewerBody::getCameraPosition() const
{
    osg::ref_ptr<const osg::Camera> cam = getOsgCamera();
    if ( !cam )
	return Coord3::udf();

    osg::Vec3 eye, center, up;
    cam->getViewMatrixAsLookAt( eye, center, up );
    return Coord3( eye.x(), eye.y(), eye.z() );
}


void ui3DViewerBody::setScene( visBase::Scene* newscene )
{
    RefMan<visSurvey::Scene> survscene = scene_.get();
    if ( survscene )
    {
	mDetachCB( survscene->mousecursorchange,
		   ui3DViewerBody::mouseCursorChg );
    }

    offscreenrenderswitch_->removeChildren(0,
			    offscreenrenderswitch_->getNumChildren());

    survscene = dCast(visSurvey::Scene*,newscene);
    scene_ = survscene;
    if ( !survscene )
	return;

    offscreenrenderswitch_->addChild( survscene->osgNode() );
    mAttachCB( survscene->mousecursorchange, ui3DViewerBody::mouseCursorChg );
    setAnnotColor( survscene->getAnnotColor() );
    if ( camera_ )
	survscene->setCamera( camera_.ptr() );

    OD::Color bgcol = OD::Color::Anthracite();
    Settings::common().get(
	BufferString(sKeydTectScene(),ui3DViewer::sKeyBGColor()), bgcol );
    setBackgroundColor( bgcol );
    if ( swapcallback_ )
	swapcallback_->scene_ = survscene.ptr();
}


SceneID ui3DViewerBody::getSceneID() const
{
    ConstRefMan<visBase::Scene> scene = getScene();
    if ( !scene || scene->id().isUdf() )
	return SceneID::udf();

    return SceneID( scene->id().asInt() );
}


void ui3DViewerBody::mouseCursorChg( CallBacker* )
{
    updateActModeCursor();
}


void ui3DViewerBody::updateActModeCursor()
{
    if ( isViewMode() )
	return;

    ConstRefMan<visSurvey::Scene> survscene = scene_.get();
    if ( survscene )
    {
	MouseCursor newcursor;
	const MouseCursor* mousecursor = survscene->getMouseCursor();
	if ( mousecursor && mousecursor->shape_!=MouseCursor::NotSet )
	    newcursor = *mousecursor;
	else
	    newcursor.shape_ = MouseCursor::Arrow;

	if ( newcursor != actmodecursor_ )
	{
	    actmodecursor_ = newcursor;

	    QCursor qcursor;
	    uiCursorManager::fillQCursor( actmodecursor_, qcursor );
	    qwidget()->setCursor( qcursor );
	}
    }
}


void ui3DViewerBody::align()
{
}


Geom::Size2D<int> ui3DViewerBody::getViewportSizePixels() const
{
    osg::ref_ptr<const osg::Camera> camera = getOsgCamera();
    osg::ref_ptr<const osg::Viewport> vp = camera->getViewport();

    return Geom::Size2D<int>( mNINT32(vp->width()), mNINT32(vp->height()) );
}


void ui3DViewerBody::viewAll( bool animate )
{
    if ( !view_ )
	return;

    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( manip )
	manip->viewAll( view_, animate );

    requestRedraw();
}


void ui3DViewerBody::setAnimationEnabled( bool yn )
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( manip )
	manip->setAllowThrow( yn );
}


bool ui3DViewerBody::isAnimationEnabled() const
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    return manip ? manip->getAllowThrow() : false;
}


void ui3DViewerBody::requestRedraw()
{
    if ( !view_ )
	return;

    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    const bool animating = manip->isThrown() || manip->isAnimating();
    if ( !animating )
	view_->requestRedraw();

    view_->requestContinuousUpdate( animating );
}


void ui3DViewerBody::setBackgroundColor( const OD::Color& col )
{
    RefMan<visBase::Scene> scene = getScene();
    if ( scene )
	scene->setBackgroundColor( col );
}


OD::Color ui3DViewerBody::getBackgroundColor() const
{
    ConstRefMan<visBase::Scene> scene = getScene();
    return scene ? scene->getBackgroundColor() : OD::Color::NoColor();
}


void ui3DViewerBody::setCameraPos( const osg::Vec3f& updir,
				   const osg::Vec3f& viewdir,
				   bool usetruedir, bool animate )
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();

    osg::Vec3f trueviewdir = viewdir;
    if ( !usetruedir )
    {
	osg::Vec3d eye, center, up;
	manip->getTransformation( eye, center, up );
	if ( viewdir*(eye-center) < 0.0 )
	    trueviewdir = -trueviewdir;
    }

    if ( manip )
	manip->viewAll( view_, trueviewdir, updir, animate );

    requestRedraw();
}


void ui3DViewerBody::viewPlaneX()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(1,0,0), false, true );
}


void ui3DViewerBody::viewPlaneY()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,1,0), false, true );
}


void ui3DViewerBody::viewPlaneZ()
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( !manip ) return;

    osg::Vec3d oldeye, center, oldup;
    manip->getTransformation( oldeye, center, oldup );
    const osg::Vec3f oldviewdir = center-oldeye;
    const osg::Vec3f viewplanenormal = oldviewdir^oldup;

    const osg::Vec3f newviewdir(0,0,1);
    osg::Vec3 newup = newviewdir^viewplanenormal;

    newup.normalize();

    setCameraPos( newup, osg::Vec3d(0,0,1) , true, true );
}


void ui3DViewerBody::viewPlaneN( bool animate )
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,-1,0), true, animate );
}


void ui3DViewerBody::viewPlaneYZ()
{
    setCameraPos( osg::Vec3f(0,1,1), osg::Vec3f(0,0,1), true, true );
}


static void getInlCrlVec( osg::Vec3f& vec, bool inl )
{
    const Pos::IdxPair2Coord& b2c = SI().binID2Coord();
    const Pos::IdxPair2Coord::DirTransform& xtr = b2c.getTransform(true);
    const Pos::IdxPair2Coord::DirTransform& ytr = b2c.getTransform(false);
    const float det = xtr.det( ytr );

    if ( inl )
	vec = osg::Vec3f( -ytr.c/det, xtr.c/det, 0 );
    else
	vec = osg::Vec3f( ytr.b/det, -xtr.b/det, 0 );

    vec.normalize();
}


void ui3DViewerBody::viewPlaneInl( bool animate )
{
    osg::Vec3f inlvec;
    getInlCrlVec( inlvec, true );
    setCameraPos( osg::Vec3f(0,0,1), inlvec, false, animate );
}


void ui3DViewerBody::viewPlaneCrl()
{
    osg::Vec3f crlvec;
    getInlCrlVec( crlvec, false );
    setCameraPos( osg::Vec3f(0,0,1), crlvec, false, true );
}


bool ui3DViewerBody::isCameraPerspective() const
{
    osgGeo::TrackballManipulator* manip = getCameraManipulator();
    return !manip ? true : manip->isCameraPerspective();
}


bool ui3DViewerBody::isCameraOrthographic() const
{
    return !isCameraPerspective();
}


void ui3DViewerBody::setCameraPerspective( bool yn )
{
    osgGeo::TrackballManipulator* manip = getCameraManipulator();
    if ( manip )
	manip->setProjectionAsPerspective( yn );
}


void ui3DViewerBody::toggleCameraType()
{
    if ( mapview_ ) return;

    osgGeo::TrackballManipulator* manip = getCameraManipulator();
    if ( !manip ) return;

    manip->setProjectionAsPerspective( !manip->isCameraPerspective() );

    requestRedraw();
}


void ui3DViewerBody::uiRotate( float angle, bool horizontal )
{
    mDynamicCastGet( osgGA::StandardManipulator*, manip,
		     view_->getCameraManipulator() );
    if ( !manip )
	return;

    osg::Vec3d eye, center, up;
    manip->getTransformation( eye, center, up );
    osg::Matrixd mat;
    osg::Vec3d axis = horizontal ? up : (eye-center)^up;
    mat.makeRotate( angle, axis );
    mat.preMultTranslate( -center );
    mat.postMultTranslate( center );
    manip->setTransformation( eye*mat, center, up );
}


void ui3DViewerBody::notifyManipulatorMovement( float dh, float dv, float df )
{
    osgGeo::TrackballManipulator* manip = getCameraManipulator();
    const float rotationtime = manip && manip->isDiscreteZooming() ? 0.2 : 0.0;
    float distancediff = df*M_PI;
    if ( manip && manip->getWheelZoomFactor()<0 )
	distancediff *= -1;

    distancethumbwheel_->setAngle( distancethumbwheel_->getAngle()+distancediff,
				   rotationtime );

    horthumbwheel_->setAngle( dh + horthumbwheel_->getAngle() );
    verthumbwheel_->setAngle( dv + verthumbwheel_->getAngle() );
}


void ui3DViewerBody::uiZoom( float rel, const osg::Vec3f* dir )
{
    mDynamicCastGet( osgGA::StandardManipulator*, manip,
		     view_->getCameraManipulator() );

    osg::ref_ptr<const osg::Camera> cam = getOsgCamera();

    if ( !manip || !cam )
	return;

    osg::Vec3d eye, center, up;
    manip->getTransformation( eye, center, up );

    double multiplicator = exp( rel );

    double l, r, b, t, znear, zfar;
    if ( cam->getProjectionMatrixAsFrustum(l,r,b,t,znear,zfar) )
    {
	osg::Vec3d olddir = center - eye;
	osg::Vec3d newdir = olddir;
	if ( dir )
	    newdir = *dir;

	double movement = (multiplicator-1.0) * olddir.length();
	const double minmovement = fabs(znear-zfar) * 0.01;

	if ( fabs(movement) < minmovement )
	{
	    if ( movement < 0.0 )	// zoom in
		return;

	    movement = minmovement;
	}

	olddir.normalize();
	newdir.normalize();
	if ( !newdir.valid() )
	    return;

	eye -= newdir * movement;
	center -= (olddir*(newdir*olddir) - newdir) * movement;
    }
    else if ( cam->getProjectionMatrixAsOrtho(l,r,b,t,znear,zfar) )
    {
	// TODO
    }

    manip->setTransformation( eye, center, up );
}


void ui3DViewerBody::setCameraZoom( float val )
{
    //  Only implemented for SoPerspectiveCamera
    osg::ref_ptr<osg::Camera> cam = getOsgCamera();

    double fovy, aspr, znear, zfar;
    if ( cam && cam->getProjectionMatrixAsPerspective(fovy,aspr,znear,zfar) )
	cam->setProjectionMatrixAsPerspective( val, aspr, znear, zfar );
}


float ui3DViewerBody::getCameraZoom() const
{
    //  Only implemented for SoPerspectiveCamera
    osg::ref_ptr<const osg::Camera> cam = getOsgCamera();

    double fovy, aspr, znear, zfar;
    if ( !cam || !cam->getProjectionMatrixAsPerspective(fovy,aspr,znear,zfar) )
	return 0.0;

    return fovy;
}


void ui3DViewerBody::fillCameraPos( IOPar& par ) const
{
    const osgGeo::TrackballManipulator* manip = getCameraManipulator();
    if ( !manip )
	return;

    const osg::Quat quat = manip->getRotation();
    const double distance = manip->getDistance();
    const osg::Vec3d center = manip->getCenter();

    par.set( sKeyCameraRotation(), quat.x(), quat.y(), quat.z(), quat.w() );
    par.set( sKeyManipCenter(), Conv::to<Coord3>( center ) );
    par.set( sKeyManipDistance(), distance );

}


bool ui3DViewerBody::useCameraPos( const IOPar& par )
{
    if ( par.isEmpty() )
	return false;

    const PtrMan<IOPar> survhomepospar = SI().pars().subselect( sKeyHomePos() );
    if ( !survhomepospar )
	  fillCameraPos( homepos_ );

    double x( 0 ), y( 0 ), z( 0 ), w( 0 );
    double distance( 0 );
    Coord3 center;
    if ( !par.get ( sKeyCameraRotation(), x,y,z,w ) ||
	 !par.get( sKeyManipCenter(), center ) ||
	 !par.get( sKeyManipDistance(), distance ) )
	return false;

    osgGeo::TrackballManipulator* manip = getCameraManipulator();
    if ( !manip )
	return false;

    manip->animateTo( Conv::to<osg::Vec3d>(center), osg::Quat(x,y,z,w),
		      distance );
    requestRedraw();
    return true;

}


class HomePosManager
{
public:
			~HomePosManager()	{ deepErase(homepos_); }
    bool		read();
    bool		write();

    int			size() const		{ return homepos_.size(); }
    void		getNames(BufferStringSet&) const;
    int			indexOf(const char*) const;
    const IOPar&	get(int) const;

    void		add(const IOPar&);
    void		remove(int);

protected:
    ObjectSet<IOPar>	homepos_;
};


bool HomePosManager::read()
{
    return true;
}


bool HomePosManager::write()
{
    IOPar outpar;
    for ( int idx=0; idx<homepos_.size(); idx++ )
    {
	outpar.mergeComp( *homepos_[idx], toString(idx) );
    }

    FilePath fp( GetDataDir(), "homepos.par" );
    const BufferString fnm = fp.fullPath();
    outpar.write( fnm, "Home Positions" );
    return true;
}


void HomePosManager::getNames( BufferStringSet& nms ) const
{
    nms.erase();
    for ( int idx=0; idx<homepos_.size(); idx++ )
    {
	const BufferString parnm = homepos_[idx]->find( sKey::Name() );
	nms.add( parnm );
    }
}


int HomePosManager::indexOf( const char* nm ) const
{
    BufferStringSet nms;
    getNames( nms );
    return nms.indexOf( nm );
}


const IOPar& HomePosManager::get( int idx ) const
{
    return *homepos_[idx];
}


void HomePosManager::add( const IOPar& par )
{
    homepos_ += new IOPar( par );
}


void HomePosManager::remove( int idx )
{
    delete homepos_.removeSingle( idx );
}


void ui3DViewerBody::setHomePos( const IOPar& homepos )
{
    homepos_ = homepos;
}


void ui3DViewerBody::toHomePos()
{
    useCameraPos( homepos_ );
}


void ui3DViewerBody::saveHomePos()
{
    homepos_.setEmpty();
    fillCameraPos( homepos_ );

    IOPar& pars = SI().getPars();
    pars.removeSubSelection( preOdHomePosition() );
    pars.removeSubSelection( sKeyHomePos() );
    pars.mergeComp( homepos_, sKeyHomePos() );
    SI().savePars();
}


void ui3DViewerBody::resetHomePos()
{
    homepos_.setEmpty();
    SI().getPars().removeSubSelection( sKeyHomePos() );
    SI().savePars();
}


bool ui3DViewerBody::isHomePosEmpty() const
{
    return homepos_.isEmpty();
}

void ui3DViewerBody::setStartupView()
{
    if ( mapview_ )
    {
	viewAll( true );
	return;
    }

    const bool hashomepos = !isHomePosEmpty();
    if ( hashomepos )
	toHomePos();
    else
    {
	SI().has3D() ? viewPlaneInl( true ) : viewPlaneN( true );
	// animation should be finished before calling viewAll
	viewalltimer_->start( 1000, true );
    }
}


void ui3DViewerBody::setScenesPixelDensity( float dpi )
{
    bool updated = false;
    RefMan<visBase::Scene> scene = getScene();
    if ( scene )
    {
	const float scenedpi = scene->getPixelDensity();
	if ( !mIsEqual(scenedpi,dpi,0.1f) )
	{
	    scene->setPixelDensity( dpi );
	    updated = true;
	}
    }

    if ( hudscene_ )
    {
	const float huddpi = hudscene_->getPixelDensity();
	if ( !mIsEqual(huddpi,dpi,0.1f) )
	{
	    hudscene_->setPixelDensity( dpi );
	    updated = true;
	}
    }

    if ( updated )
	requestRedraw();
}


bool ui3DViewerBody::setStereoType( OD::StereoType st )
{
    stereotype_ = st;
    osg::ref_ptr<osg::DisplaySettings> ds = osg::DisplaySettings::instance();
    switch( st )
    {
    case OD::StereoType::None:
	ds->setStereo( false );
	break;
    case OD::StereoType::RedCyan:
	ds->setStereo( true );
	ds->setStereoMode( osg::DisplaySettings::ANAGLYPHIC );
	break;
    case OD::StereoType::QuadBuffer:
	ds->setStereo( true );
	ds->setStereoMode( osg::DisplaySettings::QUAD_BUFFER );
	break;
    default:
	ds->setStereo( false );
    }

    return true;
}


OD::StereoType ui3DViewerBody::getStereoType() const
{ return stereotype_; }


void ui3DViewerBody::setStereoOffset( float offset )
{
    osg::ref_ptr<osg::DisplaySettings> ds = osg::DisplaySettings::instance();
    stereooffset_ = offset;
    ds->setEyeSeparation( stereooffset_/100 );
    requestRedraw();
}


float ui3DViewerBody::getStereoOffset() const
{ return stereooffset_; }


void ui3DViewerBody::setMapView( bool yn )
{
    if ( !yn )
	return;
    // TODO: For now only an enable is supported.
    // Is a disable needed?

    mapview_ = yn;
    horthumbwheel_->turnOn( false );
    verthumbwheel_->turnOn( false );
    enableThumbWheelHandling( false, horthumbwheel_.ptr() );
    enableThumbWheelHandling( false, verthumbwheel_.ptr() );

    osgGeo::TrackballManipulator* manip = getCameraManipulator();
    if ( manip )
    {
	manip->setProjectionAsPerspective( false );
	manip->setRotateMouseButton( osgGeo::TrackballManipulator::NoButton );
	manip->useLeftMouseButtonForAllMovement( false );
    }

    viewPlaneYZ();
    setBackgroundColor( OD::Color::White() );
    setAnnotColor( OD::Color::Black() );
    RefMan<visSurvey::Scene> survscene = scene_.get();
    if ( survscene )
	survscene->setAnnotColor( OD::Color::Black() );

    requestRedraw();
}


void ui3DViewerBody::enableDragging( bool yn )
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( !manip )
	return;

    if ( yn )
	manip->enableDragging( true );
    else
	manip->enableDragging((int)osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON);
}

