/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ui3dviewer.h"

#include "uicursor.h"
#include "ui3dviewerbody.h"
#include "ui3dindirectviewer.h"
#include "uirgbarray.h"
#include "uimain.h"
#include "uimouseeventblockerbygesture.h"
#include "swapbuffercallback.h"
#include "odgraphicswindow.h"

#include <osgViewer/View>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGeo/TrackballManipulator>
#include <osg/MatrixTransform>
#include <osgGeo/ThumbWheel>
#include <osg/Version>

#include "envvars.h"
#include "filepath.h"
#include "iopar.h"
#include "keybindings.h"
#include "keyboardevent.h"
#include "keystrs.h"
#include "math2.h"
#include "oddirs.h"
#include "ptrman.h"
#include "settingsaccess.h"
#include "survinfo.h"
#include "visaxes.h"
#include "visscenecoltab.h"

#include "uiobjbody.h"
#include "viscamera.h"
#include "visdatagroup.h"
#include "visdataman.h"
#include "vispolygonselection.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistext.h"
#include "visthumbwheel.h"

#include <iostream>
#include <math.h>

#include <QTabletEvent>
#include <QGestureEvent>
#include <QGesture>
#include <QPainter>

#include "uiobjbody.h"
#include "keystrs.h"

#include "survinfo.h"
#include "viscamera.h"
#include "vissurvscene.h"
#include "visdatagroup.h"

static const char* sKeydTectScene()	{ return "dTect.Scene."; }
static const char* sKeyManipCenter()	{ return "Manipulator Center"; }
static const char* sKeyManipDistance()	{ return "Manipulator Distance"; }
static const char* preOdHomePosition()	{return "Home position.Aspect ratio"; }
static const char* sKeyHomePos()	{ return "Home position"; }
static const char* sKeyCameraRotation() { return "Camera Rotation"; }
static const char* sKeyWheelDisplayMode() { return "Wheel Display Mode"; }

StringView ui3DViewer::sKeyBindingSettingsKey()
{
    return KeyBindings::sSettingsKey();
}

mDefineEnumUtils(ui3DViewer,StereoType,"StereoType")
{ sKey::None().str(), "RedCyan", "QuadBuffer", 0 };

mDefineEnumUtils(ui3DViewer,WheelMode,"WheelMode")
{ "Never", "Always", "On Hover", 0 };


class TrackBallManipulatorMessenger : public osg::NodeCallback
{
public:
    TrackBallManipulatorMessenger( ui3DViewerBody* t )
        : viewerbody_( t )
    {}

    void	operator()(osg::Node*,osg::NodeVisitor*) override;

    void	detach() { viewerbody_ = nullptr; }

protected:
    TrackBallManipulatorMessenger()
    {

    }
    ui3DViewerBody*	viewerbody_;
};


void TrackBallManipulatorMessenger::operator()( osg::Node* node,
						osg::NodeVisitor* nv )
{
    if ( nv && viewerbody_ )
    {
	osgGeo::TrackballEventNodeVisitor* tnv =
				    (osgGeo::TrackballEventNodeVisitor*) nv;

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


class uiDirectViewBody : public ui3DViewerBody
{
public:
				uiDirectViewBody(ui3DViewer&,uiParent*);

    const mQtclass(QWidget)*	qwidget_() const override;


    virtual uiSize		minimumSize() const override
				{ return uiSize(200,200); }

protected:

    osgViewer::GraphicsWindow&	getGraphicsWindow() override
				{ return *graphicswin_.get(); }
    osg::GraphicsContext*	getGraphicsContext() override
				{ return graphicswin_.get(); }

    osg::ref_ptr<ODGraphicsWindow>	graphicswin_;
};


uiDirectViewBody::uiDirectViewBody( ui3DViewer& hndl, uiParent* parnt )
    : ui3DViewerBody( hndl, parnt )
{
    ODGLWidget* glw = new ODGLWidget( parnt->pbody()->managewidg() );

    mouseeventblocker_.attachToQObj( glw );
    eventfilter_.attachToQObj( glw );

    graphicswin_ = new ODGraphicsWindow( glw );

    swapcallback_ = new SwapCallback( this );
    swapcallback_->ref();
    graphicswin_->setSwapCallback( swapcallback_ );

    setStretch(2,2);

    setupHUD();
    setupView();
    setupTouch();
}


const mQtclass(QWidget)* uiDirectViewBody::qwidget_() const
{ return graphicswin_->getGLWidget(); }


//--------------------------------------------------------------------------


ui3DViewerBody::ui3DViewerBody( ui3DViewer& h, uiParent* parnt )
    : uiObjectBody(parnt,0)
    , handle_(h)
    , printpar_(*new IOPar)
    , wheeldisplaymode_((int)ui3DViewer::OnHover)
    , offscreenrenderswitch_(new osg::Switch)
    , compositeviewer_(nullptr)
    , view_(nullptr)
    , viewport_(new osg::Viewport)
    , stereotype_(None)
    , stereooffset_(0)
    , hudview_(0)
    , offscreenrenderhudswitch_(new osg::Switch)
    , hudscene_(0)
    , mouseeventblocker_(*new uiMouseEventBlockerByGestures(500))
    , axes_(0)
    , polygonselection_(0)
    , manipmessenger_(new TrackBallManipulatorMessenger(this))
    , swapcallback_( 0 )
    , visscenecoltab_(0)
    , keybindman_(*new KeyBindMan)
    , mapview_(false)
{
    manipmessenger_->ref();
    offscreenrenderswitch_->ref();
    offscreenrenderhudswitch_->ref();
    viewport_->ref();
    eventfilter_.addEventType( uiEventFilter::KeyPress );
    eventfilter_.addEventType( uiEventFilter::Resize );
    eventfilter_.addEventType( uiEventFilter::Show );
    eventfilter_.addEventType( uiEventFilter::Gesture );

    mAttachCB( eventfilter_.eventhappened, ui3DViewerBody::qtEventCB );

    mAttachCB( uiMain::keyboardEventHandler().keyPressed,
	       ui3DViewerBody::setFocusCB );
}


ui3DViewerBody::~ui3DViewerBody()
{
    detachAllNotifiers();
    delete &keybindman_;

    manipmessenger_->detach();
    manipmessenger_->unref();

    handle_.destroyed.trigger(handle_);
    delete &printpar_;
    if ( compositeviewer_ )
    {
	compositeviewer_->removeView( view_ );
	compositeviewer_->removeView( hudview_ );
	compositeviewer_->unref();
    }
    viewport_->unref();
    offscreenrenderswitch_->unref();
    offscreenrenderhudswitch_->unref();
    if ( swapcallback_ ) swapcallback_->unref();
}


void ui3DViewerBody::removeSwapCallback( CallBacker* )
{
    if ( swapcallback_ )
    {
	getGraphicsContext()->setSwapCallback( 0 );
	swapcallback_->unref();
	swapcallback_ = 0;
    }
}

#define mMainCameraOrder    0
#define mHudCameraOrder	    (mMainCameraOrder+1)

void ui3DViewerBody::setupHUD()
{
    if ( hudview_ )
	return;

    visBase::Camera* vishudcam = visBase::Camera::create();
    osg::ref_ptr<osg::Camera> hudcamera = vishudcam->osgCamera();
    hudcamera->setGraphicsContext( getGraphicsContext() );
    hudcamera->setName("HUD Camera");
    hudcamera->setProjectionMatrix( osg::Matrix::ortho2D(0,1024,0,768) );
    hudcamera->setViewport( viewport_ );
    hudcamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    hudcamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    hudcamera->setViewMatrix(osg::Matrix::identity());
    hudcamera->setProjectionResizePolicy( osg::Camera::FIXED );

    //draw subgraph after main camera view.
    hudcamera->setRenderOrder(osg::Camera::POST_RENDER, mHudCameraOrder );

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
	compositeviewer_->ref();
    }

    compositeviewer_->addView( hudview_ );

    horthumbwheel_ = visBase::ThumbWheel::create();
    hudscene_->addObject( horthumbwheel_ );
    mAttachCB( horthumbwheel_->rotation, ui3DViewerBody::thumbWheelRotationCB);

    verthumbwheel_ = visBase::ThumbWheel::create();
    hudscene_->addObject( verthumbwheel_ );
    mAttachCB( verthumbwheel_->rotation, ui3DViewerBody::thumbWheelRotationCB);

    distancethumbwheel_ = visBase::ThumbWheel::create();
    hudscene_->addObject( distancethumbwheel_ );
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
	hudscene_->addObject( axes_ );
	if ( camera_ )
	    axes_->setPrimaryCamera( camera_ );
    }

    if ( !polygonselection_ )
    {
	polygonselection_ = visBase::PolygonSelection::create();
	hudscene_->addObject( polygonselection_ );
	polygonselection_->setHUDCamera( vishudcam );
    }

    if ( !visscenecoltab_ )
    {
	visscenecoltab_ = visBase::SceneColTab::create();
	hudscene_->addObject( visscenecoltab_ );
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
	axes_->setPrimaryCamera( camera_ );

    if ( scene_ )
        scene_->setCamera( camera_ );

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
	compositeviewer_->ref();
    }

    compositeviewer_->addView( view_ );

    // To put exaggerated bounding sphere radius offside
    manip->setMinimumDistance( 0 );

    if ( polygonselection_ )
	polygonselection_->setPrimaryCamera( camera_ );

    view_->getSceneData()->addEventCallback(new osgGeo::ThumbWheelEventHandler);
    enableThumbWheelHandling( true );

    bool reversezoom = false;
    Settings::common().getYN(SettingsAccess::sKeyMouseWheelReversal(),
			     reversezoom);

    setReversedMouseWheelDirection( reversezoom );

    // Camera projection must be initialized before computing home position
    reSizeEvent( 0 );
}


void ui3DViewerBody::enableThumbWheelHandling( bool yn,
					       const visBase::ThumbWheel* tw )
{
    if ( !tw )	// set all
    {
	enableThumbWheelHandling( yn, horthumbwheel_ );
	enableThumbWheelHandling( yn, verthumbwheel_ );
	enableThumbWheelHandling( yn, distancethumbwheel_ );
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
	viewer, = 0 );

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
    return view_ ? view_->getCamera() : 0;
}


const osg::Camera* ui3DViewerBody::getOsgCamera() const
{
    return const_cast<ui3DViewerBody*>( this )->getOsgCamera();
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
    if ( caller==horthumbwheel_ )
    {
	uiRotate( deltaangle, true );
    }
    else if ( caller==verthumbwheel_ )
    {
	uiRotate( deltaangle, false );
    }
    else if ( caller==distancethumbwheel_ )
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


void ui3DViewerBody::setWheelDisplayMode( WheelMode mode )
{
    const bool doshow = mode != Never;
    horthumbwheel_->turnOn( doshow );
    verthumbwheel_->turnOn( doshow );
    distancethumbwheel_->turnOn( doshow );
    enableThumbWheelHandling( doshow );

    const bool enablefadeinout = mode==OnHover;
    horthumbwheel_->enableFadeInOut( enablefadeinout );
    verthumbwheel_->enableFadeInOut( enablefadeinout );
    distancethumbwheel_->enableFadeInOut( enablefadeinout );
}


ui3DViewerBody::WheelMode ui3DViewerBody::getWheelDisplayMode() const
{
    if ( !horthumbwheel_->isOn() )
	return Never;

    if ( horthumbwheel_->isFadeInOutEnabled() )
	return OnHover;

    return Always;
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
    return polygonselection_;
}


visBase::SceneColTab* ui3DViewerBody::getSceneColTab()
{
    return visscenecoltab_;
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
    return scene_ && !scene_->isPickable();
}


void ui3DViewerBody::setViewMode( bool yn, bool trigger )
{
    enableDragging( yn );

    if ( scene_ )
	scene_->setPickable( !yn );

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


void ui3DViewerBody::setSceneID( SceneID sceneid )
{
    if ( scene_ )
    {
	mDynamicCastGet( visSurvey::Scene*, survscene, scene_.ptr() );
	mDetachCB(survscene->mousecursorchange, ui3DViewerBody::mouseCursorChg);
    }

    visBase::DataObject* obj = visBase::DM().getObject( sceneid );
    mDynamicCastGet(visBase::Scene*,newscene,obj)
    if ( !newscene ) return;

    offscreenrenderswitch_->removeChildren(0,
			    offscreenrenderswitch_->getNumChildren());
    offscreenrenderswitch_->addChild( newscene->osgNode() );

    scene_ = newscene;
    mDynamicCastGet( visSurvey::Scene*, survscene, scene_.ptr() );
    mAttachCB( survscene->mousecursorchange, ui3DViewerBody::mouseCursorChg );

    if ( survscene )
	setAnnotColor( survscene->getAnnotColor() );

    if ( camera_ )
	newscene->setCamera( camera_ );

    OD::Color bgcol = OD::Color::Anthracite();
    Settings::common().get(
	BufferString(sKeydTectScene(),ui3DViewer::sKeyBGColor()), bgcol );
    setBackgroundColor( bgcol );
    if ( swapcallback_ )
	swapcallback_->scene_ = scene_;
}


void ui3DViewerBody::mouseCursorChg( CallBacker* )
{
    updateActModeCursor();
}


void ui3DViewerBody::updateActModeCursor()
{
    if ( isViewMode() )
	return;

    mDynamicCastGet( const visSurvey::Scene*, survscene, scene_.ptr() );
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
    visBase::Scene* scene = getScene();
    if ( scene )
	scene->setBackgroundColor( col );
}


OD::Color ui3DViewerBody::getBackgroundColor() const
{
    const visBase::Scene* scene = getScene();
    return scene ? scene->getBackgroundColor() : OD::Color::NoColor();
}


void ui3DViewerBody::setCameraPos( const osg::Vec3f& updir,
				  const osg::Vec3f& viewdir,
				  bool usetruedir )
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
	manip->viewAll( view_, trueviewdir, updir, true );
    requestRedraw();
}


void ui3DViewerBody::viewPlaneX()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(1,0,0), false );
}


void ui3DViewerBody::viewPlaneY()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,1,0), false );
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

    setCameraPos( newup, osg::Vec3d(0,0,1) , true );
}


void ui3DViewerBody::viewPlaneN()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,-1,0), true );
}


void ui3DViewerBody::viewPlaneYZ()
{
    setCameraPos( osg::Vec3f(0,1,1), osg::Vec3f(0,0,1), true );
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


void ui3DViewerBody::viewPlaneInl()
{
    osg::Vec3f inlvec;
    getInlCrlVec( inlvec, true );
    setCameraPos( osg::Vec3f(0,0,1), inlvec, false );
}


void ui3DViewerBody::viewPlaneCrl()
{
    osg::Vec3f crlvec;
    getInlCrlVec( crlvec, false );
    setCameraPos( osg::Vec3f(0,0,1), crlvec, false );
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
    if ( par.isEmpty() ) return false;

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
    if ( !manip ) return false;

    manip->setCenter( Conv::to<osg::Vec3d>( center ) );
    manip->setRotation( osg::Quat( x, y, z, w ) );
    manip->setDistance( distance );

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


void ui3DViewerBody::resetToHomePosition()
{
}


void ui3DViewerBody::toHomePos()
{ useCameraPos( homepos_ ); }


void ui3DViewerBody::saveHomePos()
{
    homepos_.setEmpty();
    fillCameraPos( homepos_ );

    if ( SI().getPars().isPresent( preOdHomePosition() ) )
    {
	SI().getPars().removeSubSelection( sKeyHomePos() );
    }

    SI().getPars().mergeComp( homepos_, sKeyHomePos() );
    SI().savePars();
}


void ui3DViewerBody::setScenesPixelDensity( float dpi )
{
    bool updated = false;
    if ( scene_ )
    {
	const float scenedpi = scene_->getPixelDensity();
	if ( !mIsEqual(scenedpi,dpi,0.1f) )
	{
	    scene_->setPixelDensity( dpi );
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


bool ui3DViewerBody::setStereoType( ui3DViewerBody::StereoType st )
{
    stereotype_ = st;
    osg::ref_ptr<osg::DisplaySettings> ds = osg::DisplaySettings::instance();
    switch( st )
    {
    case ui3DViewerBody::None:
	ds->setStereo( false );
	break;
    case ui3DViewerBody::RedCyan:
	ds->setStereo( true );
	ds->setStereoMode( osg::DisplaySettings::ANAGLYPHIC );
	break;
    case ui3DViewerBody::QuadBuffer:
	ds->setStereo( true );
	ds->setStereoMode( osg::DisplaySettings::QUAD_BUFFER );
	break;
    default:
	ds->setStereo( false );
    }

    return true;
}


ui3DViewerBody::StereoType ui3DViewerBody::getStereoType() const
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
    if ( !yn ) return;
    // TODO: For now only an enable is supported.
    // Is a disable needed?

    mapview_ = yn;
    horthumbwheel_->turnOn( false );
    verthumbwheel_->turnOn( false );
    enableThumbWheelHandling( false, horthumbwheel_ );
    enableThumbWheelHandling( false, verthumbwheel_ );

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
    mDynamicCastGet(visSurvey::Scene*,survscene,scene_.ptr())
    if ( survscene ) survscene->setAnnotColor( OD::Color::Black() );
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


//------------------------------------------------------------------------------

ui3DViewer::ui3DViewer( uiParent* parnt, bool direct, const char* nm )
    : uiObject(parnt,nm,mkBody(parnt,direct,nm))
    , destroyed(this)
    , viewmodechanged(this)
    , pageupdown(this)
    , vmcb(0)
{
    PtrMan<IOPar> homepospar = SI().pars().subselect( sKeyHomePos() );
    if ( homepospar )
	osgbody_->setHomePos( *homepospar) ;

    setViewMode( false );  // switches between view & interact mode

    bool yn = false;
    bool res = Settings::common().getYN(
	BufferString(sKeydTectScene(),sKeyAnimate()), yn );
    if ( res )
	enableAnimation( yn );

    BufferString modestr;
    res = Settings::common().get(
	BufferString(sKeydTectScene(),sKeyWheelDisplayMode()), modestr );
    if ( res )
    {
	WheelMode mode; parseEnum( modestr, mode );
	setWheelDisplayMode( mode );
    }

    float zoomfactor = MouseEvent::getDefaultMouseWheelZoomFactor();
#ifdef __mac__
    zoomfactor = MouseEvent::getDefaultTrackpadZoomFactor();
#endif

    Settings::common().get( SettingsAccess::sKeyMouseWheelZoomFactor(),
			    zoomfactor );
    setMouseWheelZoomFactor( zoomfactor );
}


ui3DViewer::~ui3DViewer()
{
    delete osgbody_;
}


uiObjectBody& ui3DViewer::mkBody( uiParent* parnt, bool direct, const char* nm )
{
#if OSG_VERSION_LESS_THAN( 3, 5, 0 )
    initQtWindowingSystem();
#endif

    osgbody_ = direct
	? (ui3DViewerBody*) new uiDirectViewBody( *this, parnt )
	: (ui3DViewerBody*) new ui3DIndirectViewBody( *this, parnt );

    return *osgbody_;
}


void ui3DViewer::setMapView( bool yn )
{ osgbody_->setMapView( yn ); }

bool ui3DViewer::isMapView() const
{ return osgbody_->isMapView(); }


void ui3DViewer::viewAll( bool animate )
{
    mDynamicCastGet(visSurvey::Scene*,survscene,getScene());
    if ( !survscene )
    {
	osgbody_->viewAll( animate );
    }
    else
    {
	bool showtext = survscene->isAnnotTextShown();
	bool showscale = survscene->isAnnotScaleShown();
	if ( !showtext && !showscale )
	{
	    osgbody_->viewAll(animate);
	}
	else
	{
	    survscene->showAnnotText( false );
	    survscene->showAnnotScale( false );
	    osgbody_->viewAll(animate);
	    survscene->showAnnotText( showtext );
	    survscene->showAnnotScale( showscale );
	}
    }
}


void ui3DViewer::enableAnimation( bool yn )
{ if ( osgbody_ ) osgbody_->setAnimationEnabled( yn ); }

bool ui3DViewer::isAnimationEnabled() const
{ return osgbody_ ? osgbody_->isAnimationEnabled() : false; }


void ui3DViewer::setBackgroundColor( const OD::Color& col )
{
    osgbody_->setBackgroundColor( col );
}

OD::Color ui3DViewer::getBackgroundColor() const
{
    return osgbody_->getBackgroundColor();
}


void ui3DViewer::setAnnotationColor( const OD::Color& col )
{
    osgbody_->setAnnotColor( col );
}


OD::Color ui3DViewer::getAnnotationColor() const
{
    mDynamicCastGet(const visSurvey::Scene*,survscene,getScene());
    return survscene ? survscene->getAnnotColor() : OD::Color::White();
}


void ui3DViewer::setAnnotationFont( const FontData& fd )
{
    osgbody_->setAnnotationFont( fd );
}


void ui3DViewer::getAllKeyBindings( BufferStringSet& keys ) const
{
    osgbody_->keyBindMan().getAllKeyBindings( keys );
}


void ui3DViewer::setKeyBindings( const char* keybindname )
{
    osgbody_->keyBindMan().setKeyBindings( keybindname );
}


const char* ui3DViewer::getCurrentKeyBindings() const
{
    return osgbody_->keyBindMan().getCurrentKeyBindings();
}


void ui3DViewer::viewPlane( PlaneType type )
{
    if ( isMapView() ) return;

    switch ( type )
    {
	case X: osgbody_->viewPlaneX(); break;
	case Y: osgbody_->viewPlaneN(); break;
	case Z: osgbody_->viewPlaneZ(); break;
	case Inl: osgbody_->viewPlaneInl(); break;
	case Crl: osgbody_->viewPlaneCrl(); break;
	case YZ:osgbody_->viewPlaneYZ(); break;
    }
}


bool ui3DViewer::isCameraPerspective() const
{
    return osgbody_->isCameraPerspective();
}


bool ui3DViewer::setStereoType( StereoType type )
{
    return osgbody_->setStereoType( (ui3DViewerBody::StereoType)type );
}


ui3DViewer::StereoType ui3DViewer::getStereoType() const
{
    return (ui3DViewer::StereoType) osgbody_->getStereoType();
}


void ui3DViewer::setStereoOffset( float offset )
{
    osgbody_->setStereoOffset( offset );
}


float ui3DViewer::getStereoOffset() const
{
    return osgbody_->getStereoOffset();
}

void ui3DViewer::setMouseWheelZoomFactor( float factor )
{
    osgbody_->setMouseWheelZoomFactor( factor );
}


float ui3DViewer::getMouseWheelZoomFactor() const
{ return osgbody_->getMouseWheelZoomFactor(); }


void ui3DViewer::setReversedMouseWheelDirection( bool reversed )
{
    osgbody_->setReversedMouseWheelDirection( reversed );
}


bool ui3DViewer::getReversedMouseWheelDirection() const
{ return osgbody_->getReversedMouseWheelDirection(); }


void ui3DViewer::setSceneID( SceneID sceneid )
{
    osgbody_->setSceneID( sceneid );
}


SceneID ui3DViewer::sceneID() const
{
    return osgbody_->getScene() ? osgbody_->getScene()->id() : SceneID::udf();
}


void ui3DViewer::setViewMode( bool yn )
{
    if ( yn==isViewMode() )
	return;

    osgbody_->setViewMode( yn, false );
}


bool ui3DViewer::isViewMode() const
{ return osgbody_->isViewMode(); }

void ui3DViewer::rotateH( float angle )
{ osgbody_->uiRotate( angle, true ); }

void ui3DViewer::rotateV( float angle )
{ osgbody_->uiRotate( angle, false ); }

void ui3DViewer::dolly( float rel )
{ osgbody_->uiZoom( rel ); }

void ui3DViewer::setCameraZoom( float val )
{ osgbody_->setCameraZoom( val ); }

float ui3DViewer::getCameraZoom()
{ return osgbody_->getCameraZoom(); }

const Coord3 ui3DViewer::getCameraPosition() const
{ return osgbody_->getCameraPosition(); }

void ui3DViewer::align()
{ osgbody_->align(); }

void ui3DViewer::toHomePos()
{ osgbody_->toHomePos(); }

void ui3DViewer::saveHomePos()
{ osgbody_->saveHomePos(); }

void ui3DViewer::showRotAxis( bool yn ) // OSG-TODO
{ osgbody_->showRotAxis( yn ); }

void ui3DViewer::setWheelDisplayMode( WheelMode mode )
{ osgbody_->setWheelDisplayMode( (ui3DViewerBody::WheelMode)mode ); }

ui3DViewer::WheelMode ui3DViewer::getWheelDisplayMode() const
{ return (ui3DViewer::WheelMode)osgbody_->getWheelDisplayMode(); }

bool ui3DViewer::rotAxisShown() const
{ return osgbody_->isAxisShown(); }

visBase::PolygonSelection* ui3DViewer::getPolygonSelector()
{ return osgbody_->getPolygonSelector(); }

visBase::SceneColTab* ui3DViewer::getSceneColTab()
{ return osgbody_->getSceneColTab(); }

void ui3DViewer::toggleCameraType()
{ osgbody_->toggleCameraType(); }

Geom::Size2D<int> ui3DViewer::getViewportSizePixels() const
{ return osgbody_->getViewportSizePixels(); }


void ui3DViewer::savePropertySettings() const
{
#define mSaveProp(set,str,func) \
    Settings::common().set( BufferString(sKeydTectScene(),str), func );

    mSaveProp( set, sKeyBGColor(), getBackgroundColor() );
    mSaveProp( setYN, sKeyAnimate(), isAnimationEnabled() );
    mSaveProp( set, sKeyWheelDisplayMode(),
	       getWheelModeString(getWheelDisplayMode()) );
    Settings::common().write();
}


void ui3DViewer::fillPar( IOPar& par ) const
{
    par.set( sKeySceneID(), getScene()->id() );
    par.set( sKeyBGColor(), (int)getBackgroundColor().rgb() );
    par.set( sKeyStereo(), toString( getStereoType() ) );
    float offset = getStereoOffset();
    par.set( sKeyStereoOff(), offset );
    par.setYN( sKeyPersCamera(), isCameraPerspective() );

    osgbody_->fillCameraPos( par );
}


bool ui3DViewer::usePar( const IOPar& par )
{
    SceneID sceneid;
    if ( !par.get(sKeySceneID(),sceneid) )
	return false;

    setSceneID( sceneid );
    if ( !getScene() ) return false;

    int col;
    if ( par.get(sKeyBGColor(),col) )
    {
	OD::Color newcol;
	newcol.setRgb( col );
	setBackgroundColor( newcol );
    }

    StereoType stereotype;
    if ( parseEnum( par, sKeyStereo(), stereotype ) )
	setStereoType( stereotype );

    float offset;
    if ( par.get( sKeyStereoOff(), offset )  )
	setStereoOffset( offset );

    PtrMan<IOPar> homepos = par.subselect( sKeyHomePos() );

    if ( homepos && osgbody_->isHomePosEmpty() )
	osgbody_->setHomePos( *homepos );

    osgbody_->useCameraPos( par );
    return true;
}


visBase::Scene* ui3DViewer::getScene()
{ return osgbody_->getScene(); }

const visBase::Scene* ui3DViewer::getScene() const
{ return const_cast<ui3DViewer*>(this)->getScene(); }

const osgViewer::View*	ui3DViewer::getOsgViewerMainView() const
{ return osgbody_->getOsgViewerMainView(); }

const osgViewer::View*	ui3DViewer::getOsgViewerHudView() const
{ return osgbody_->getOsgViewerHudView(); }

void ui3DViewer::setScenesPixelDensity(float dpi)
{ osgbody_->setScenesPixelDensity( dpi ); }

float ui3DViewer::getScenesPixelDensity() const
{ return getScene()->getPixelDensity(); }
