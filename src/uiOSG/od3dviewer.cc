/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/


#include "od3dviewer.h"
#include "ui3dviewer.h"

#include "odopenglwidget.h"
#include "odosgviewer.h"

#include "keybindings.h"
#include "keyboardevent.h"
#include "keystrs.h"
#include "settingsaccess.h"
#include "survinfo.h"
#include "swapbuffercallback.h"
#include "timer.h"

#include "visaxes.h"
#include "viscamera.h"
#include "vispolygonselection.h"
#include "visscenecoltab.h"
#include "vissurvscene.h"
#include "visthumbwheel.h"

#include "uicursor.h"
#include "uimain.h"
#include "uimouseeventblockerbygesture.h"

#include <QKeyEvent>
#include <osg/Camera>
#include <osg/NodeCallback>
#include <osg/Switch>
#include <osgGeo/ThumbWheel>
#include <osgGeo/TrackballManipulator>
#include <osgViewer/ViewerEventHandlers>


// TODO: rework
#include "filepath.h"
#include "oddirs.h"

static const char* sKeydTectScene()	{ return "dTect.Scene."; }
static const char* sKeyHomePos()	{ return "Home position"; }
static const char* sKeyManipCenter()	{ return "Manipulator Center"; }
static const char* sKeyManipDistance()	{ return "Manipulator Distance"; }
static const char* preOdHomePosition()	{return "Home position.Aspect ratio"; }
static const char* sKeyCameraRotation() { return "Camera Rotation"; }


class TrackBallManipulatorMessenger : public osg::NodeCallback
{
public:
    TrackBallManipulatorMessenger( OD3DViewer* t )
        : viewerbody_( t )
    {}

    void	operator()(osg::Node*,osg::NodeVisitor*) override;

    void	detach() { viewerbody_ = nullptr; }

protected:
    TrackBallManipulatorMessenger()
    {}

    OD3DViewer*	viewerbody_;
};


void TrackBallManipulatorMessenger::operator()( osg::Node* node,
						osg::NodeVisitor* nv )
{
    if ( nv && viewerbody_ )
    {
	auto* tnv = sCast(osgGeo::TrackballEventNodeVisitor*,nv);
	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::RotateStart )
	    viewerbody_->setViewModeCursor( OD3DViewer::RotateCursor );
	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::PanStart )
	    viewerbody_->setViewModeCursor( OD3DViewer::PanCursor );
	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::ZoomStart )
	    viewerbody_->setViewModeCursor( OD3DViewer::ZoomCursor );
	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::MoveStart )
	    viewerbody_->setViewModeCursor( OD3DViewer::HoverCursor );
	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::MoveStop )
	    viewerbody_->setViewModeCursor( OD3DViewer::HoverCursor );

	if ( tnv->_eventType==osgGeo::TrackballEventNodeVisitor::Moving )
	{
	    viewerbody_->notifyManipulatorMovement( tnv->_deltahorangle,
						    tnv->_deltavertangle,
						    tnv->_distfactor );
	}
    }
}


// OD3DViewer

OD3DViewer::OD3DViewer( ui3DViewer& h, uiParent* parnt )
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

    glwidget_ = new ODOpenGLWidget( parnt->pbody()->managewidg() );
    mouseeventblocker_.attachToQObj( glwidget_ );
    eventfilter_.attachToQObj( glwidget_ );

    swapcallback_ = new SwapCallback( this );
    swapcallback_->ref();
    glwidget_->getGraphicsWindow()->setSwapCallback( swapcallback_ );

    setupHUD();
    setupView();
    setupTouch();

    PtrMan<IOPar> homepospar = SI().pars().subselect( sKeyHomePos() );
    if ( homepospar )
	setHomePos( *homepospar) ;

    setStretch( 2, 2 );

    mAttachCB( eventfilter_.eventhappened, OD3DViewer::qtEventCB );
    mAttachCB( uiMain::keyboardEventHandler().keyPressed,
	       OD3DViewer::setFocusCB );
   mAttachCB( viewalltimer_->tick, OD3DViewer::viewAllCB );
}


OD3DViewer::~OD3DViewer()
{
    detachAllNotifiers();
    delete &keybindman_;
    delete viewalltimer_;

    manipmessenger_->detach();
    visBase::unRefOsgPtr( manipmessenger_ );

    handle_.destroyed.trigger( handle_ );
    delete &printpar_;

    visBase::unRefOsgPtr( viewport_ );
    visBase::unRefOsgPtr( offscreenrenderswitch_ );
    visBase::unRefOsgPtr( offscreenrenderhudswitch_ );
    visBase::unRefOsgPtr( swapcallback_ );
}


void OD3DViewer::removeSwapCallback( CallBacker* )
{
    if ( swapcallback_ )
	getGraphicsContext()->setSwapCallback( nullptr );

    visBase::unRefAndNullOsgPtr( swapcallback_ );
}


void OD3DViewer::viewAllCB( CallBacker* )
{
    viewAll( true );
}


static int sMainCameraOrder()	{ return 0; }
static int sHUDCameraOrder()	{ return 1; }

void OD3DViewer::setupHUD()
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
    hudcamera->setRenderOrder( osg::Camera::POST_RENDER, sHUDCameraOrder() );

    //we don't want the camera to grab event focus from the viewers main cam(s).
    hudcamera->setAllowEventFocus(false);

    hudscene_ = visBase::DataObjectGroup::create();

    hudview_ = new ODOSGViewer( glwidget_ );
    hudview_->setCamera( hudcamera );
    hudview_->doInit();
    offscreenrenderhudswitch_->removeChild(
	0, offscreenrenderhudswitch_->getNumChildren() );
    offscreenrenderhudswitch_->addChild( hudscene_->osgNode() );
    hudview_->setSceneData( offscreenrenderhudswitch_ );

    horthumbwheel_ = visBase::ThumbWheel::create();
    hudscene_->addObject( horthumbwheel_.ptr() );
    mAttachCB( horthumbwheel_->rotation, OD3DViewer::thumbWheelRotationCB);

    verthumbwheel_ = visBase::ThumbWheel::create();
    hudscene_->addObject( verthumbwheel_.ptr() );
    mAttachCB( verthumbwheel_->rotation, OD3DViewer::thumbWheelRotationCB);

    distancethumbwheel_ = visBase::ThumbWheel::create();
    hudscene_->addObject( distancethumbwheel_.ptr() );
    mAttachCB( distancethumbwheel_->rotation,
	       OD3DViewer::thumbWheelRotationCB);

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
	hudscene_->addObject( polygonselection_ );
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


void OD3DViewer::setupTouch()
{
    if ( getGraphicsWindow().getEventQueue() )
	getGraphicsWindow().getEventQueue()->setFirstTouchEmulatesMouse(false);

    qwidget()->grabGesture( Qt::PinchGesture );

}


osgGeo::TrackballManipulator* OD3DViewer::getCameraManipulator() const
{
    if ( !view_ )
	return nullptr;

    return sCast(osgGeo::TrackballManipulator*,view_->getCameraManipulator());
}


void OD3DViewer::setMouseWheelZoomFactor( float factor )
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( !manip )
	return;

    if ( getReversedMouseWheelDirection() )
	manip->setWheelZoomFactor( factor );
    else
	manip->setWheelZoomFactor( -factor );
}


float OD3DViewer::getMouseWheelZoomFactor() const
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( !manip )
	return MouseEvent::getDefaultMouseWheelZoomFactor();

    return fabs( manip->getWheelZoomFactor() );
}


void OD3DViewer::setReversedMouseWheelDirection( bool reversed )
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


bool OD3DViewer::getReversedMouseWheelDirection() const
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    return manip ? manip->getWheelZoomFactor()>0 : false;
}


void OD3DViewer::setupView()
{
    camera_ = visBase::Camera::create();
    if ( axes_ )
	axes_->setPrimaryCamera( camera_.ptr() );

    RefMan<visBase::Scene> scene = getScene();
    if ( scene )
	scene->setCamera( camera_.ptr() );

    viewport_->setViewport( 0, 0, glwidget_->width(), glwidget_->height() );

    mDynamicCastGet(osg::Camera*, osgcamera, camera_->osgNode(true) );
    osgcamera->setGraphicsContext( getGraphicsContext() );
    osgcamera->setClearColor( osg::Vec4(0.0f,0.0f,0.0f,1.0f) );
    osgcamera->setViewport( viewport_ );
    osgcamera->setRenderOrder( osg::Camera::POST_RENDER, sMainCameraOrder() );
    osgcamera->setNearFarRatio( 0.002 );	// default is 0.0005

    view_ = new ODOSGViewer( glwidget_ );
    view_->setCamera( osgcamera );
    view_->doInit();
    view_->setSceneData( offscreenrenderswitch_ );

    glwidget_->setViewer( view_ );

    auto* statshandler = new osgViewer::StatsHandler;
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

    manip->setAllowThrow( true );
    manip->useLeftMouseButtonForAllMovement( true, true );
    manip->enableKeyHandling( false );	// No space key to restore home view
    manip->addMovementCallback( manipmessenger_ );
    manip->setBoundTraversalMask( visBase::cBBoxTraversalMask() );
    manip->setIntersectTraversalMask( visBase::cIntersectionTraversalMask() );
    manip->setAnimationTime( 0.5 );
    manip->setAutoComputeHomePosition( false );

    view_->setCameraManipulator( manip.get() );

    enableDragging( isViewMode() );

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


visBase::Scene* OD3DViewer::getScene()
{
    return scene_.get().ptr();
}


const visBase::Scene* OD3DViewer::getScene() const
{
    return mSelf().getScene();
}


void OD3DViewer::enableThumbWheelHandling( bool yn,
					   const visBase::ThumbWheel* tw )
{
    if ( !tw )	// set all
    {
	if ( horthumbwheel_ )
	    enableThumbWheelHandling( yn, horthumbwheel_.ptr() );
	if ( verthumbwheel_ )
	    enableThumbWheelHandling( yn, verthumbwheel_.ptr() );
	if ( distancethumbwheel_ )
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
	    auto* osgtw = (osgGeo::ThumbWheel*)tw->osgNode(true);
	    handler->removeThumbWheel( osgtw );
	    if ( yn )
		handler->addThumbWheel( osgtw );
	}
    }
}


uiObject& OD3DViewer::uiObjHandle()
{
    return handle_;
}


const QWidget* OD3DViewer::qwidget_() const
{
    return glwidget_;
}


osgViewer::GraphicsWindow& OD3DViewer::getGraphicsWindow()
{
    return *glwidget_->getGraphicsWindow();
}


osg::GraphicsContext* OD3DViewer::getGraphicsContext()
{
    return glwidget_->getGraphicsWindow();
}


osg::Camera* OD3DViewer::getOsgCamera()
{
    return view_ ? view_->getCamera() : nullptr;
}


const osg::Camera* OD3DViewer::getOsgCamera() const
{
    return const_cast<OD3DViewer*>( this )->getOsgCamera();
}


#define mLongSideDistance	15
#define mShortSideDistance	40

#define mThumbWheelLen		100
#define mThumbWheelWidth	15

#define mZCoord			-1


void OD3DViewer::reSizeEvent( CallBacker* )
{
    const mQtclass(QWidget)* widget = qwidget();
    osg::ref_ptr<osg::Camera> osgcamera = getOsgCamera();
    if ( !widget || !camera_ || !osgcamera )
	return;

    const int newwidth = widget->width();
    const int newheight = widget->height();

    if ( hudview_ )
	hudview_->getCamera()->setProjectionMatrix(
		osg::Matrix::ortho2D(0,newwidth,0,newheight) );

    const float longsideoffset = mLongSideDistance+mThumbWheelWidth/2.0;
    const float shortsideoffset = mShortSideDistance+mThumbWheelLen/2.0;

    if ( horthumbwheel_ )
	horthumbwheel_->setPosition( true, shortsideoffset, longsideoffset,
				 mThumbWheelLen, mThumbWheelWidth, mZCoord );
    if ( verthumbwheel_ )
	verthumbwheel_->setPosition( false, longsideoffset, shortsideoffset,
				 mThumbWheelLen, mThumbWheelWidth, mZCoord );
    if ( distancethumbwheel_ )
	distancethumbwheel_->setPosition( false, longsideoffset,
			mMAX(newheight-shortsideoffset,shortsideoffset),
			mThumbWheelLen, mThumbWheelWidth, mZCoord );

    if ( axes_ )
    {
	const float offset = axes_->getLength() + 10;
	axes_->setPosition( newwidth-offset, offset );
    }

    if ( visscenecoltab_ )
	visscenecoltab_->setWindowSize( newwidth, newheight );
}


void OD3DViewer::thumbWheelRotationCB( CallBacker* cb )
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


void OD3DViewer::toggleViewMode( CallBacker* )
{
    setViewMode( !isViewMode(), true );
}


void OD3DViewer::showRotAxis( bool yn )
{
    if ( axes_ )
	axes_->turnOn( yn );
}


bool OD3DViewer::isAxisShown() const
{
    return axes_ && axes_->isOn();
}


void OD3DViewer::setWheelDisplayMode( OD::WheelMode mode )
{
    const bool doshow = mode != OD::WheelMode::Never;
    if ( horthumbwheel_ )
	horthumbwheel_->turnOn( doshow );
    if ( verthumbwheel_ )
	verthumbwheel_->turnOn( doshow );
    if ( distancethumbwheel_ )
	distancethumbwheel_->turnOn( doshow );
    enableThumbWheelHandling( doshow );

    const bool enablefadeinout = mode==OD::WheelMode::OnHover;
    if ( horthumbwheel_ )
	horthumbwheel_->enableFadeInOut( enablefadeinout );
    if ( verthumbwheel_ )
	verthumbwheel_->enableFadeInOut( enablefadeinout );
    if ( distancethumbwheel_ )
	distancethumbwheel_->enableFadeInOut( enablefadeinout );
}


OD::WheelMode OD3DViewer::getWheelDisplayMode() const
{
    if ( !horthumbwheel_->isOn() )
	return OD::WheelMode::Never;

    if ( horthumbwheel_->isFadeInOutEnabled() )
	return OD::WheelMode::OnHover;

    return OD::WheelMode::Always;
}


void OD3DViewer::setAnnotColor( const OD::Color& col )
{
    if ( axes_ )
	axes_->setAnnotationColor( col );

    if ( horthumbwheel_ )
	horthumbwheel_->setAnnotationColor( col );
    if ( verthumbwheel_ )
	verthumbwheel_->setAnnotationColor( col );
    if ( distancethumbwheel_ )
	distancethumbwheel_->setAnnotationColor( col );
}


void OD3DViewer::setAnnotationFont( const FontData& fd )
{
    if ( visscenecoltab_ )
	visscenecoltab_->setAnnotFont( fd );
    if ( axes_ )
	axes_->setAnnotationFont( fd );
}


visBase::PolygonSelection* OD3DViewer::getPolygonSelector()
{
    return polygonselection_.ptr();
}


visBase::SceneColTab* OD3DViewer::getSceneColTab()
{
    return visscenecoltab_.ptr();
}


void OD3DViewer::qtEventCB( CallBacker* )
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


void OD3DViewer::setFocusCB( CallBacker* )
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


bool OD3DViewer::isViewMode() const
{
    ConstRefMan<visBase::Scene> scene = getScene();
    return scene && !scene->isPickable();
}


void OD3DViewer::setViewMode( bool yn, bool trigger )
{
    enableDragging( yn );

    RefMan<visBase::Scene> scene = getScene();
    if ( scene )
	scene->setPickable( !yn );

    setViewModeCursor( HoverCursor );

    if ( trigger )
	handle_.viewmodechanged.trigger( handle_ );
}


void OD3DViewer::setViewModeCursor( ViewModeCursor viewmodecursor )
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


Coord3 OD3DViewer::getCameraPosition() const
{
    osg::ref_ptr<const osg::Camera> cam = getOsgCamera();
    if ( !cam )
	return Coord3::udf();

    osg::Vec3 eye, center, up;
    cam->getViewMatrixAsLookAt( eye, center, up );
    return Coord3( eye.x(), eye.y(), eye.z() );
}


void OD3DViewer::setScene( visBase::Scene* newscene )
{
    RefMan<visSurvey::Scene> survscene = scene_.get();
    if ( survscene )
    {
	mDetachCB( survscene->mousecursorchange,
		   OD3DViewer::mouseCursorChg );
    }

    offscreenrenderswitch_->removeChildren(0,
			    offscreenrenderswitch_->getNumChildren());

    survscene = dCast(visSurvey::Scene*,newscene);
    scene_ = survscene;
    if ( !survscene )
	return;

    offscreenrenderswitch_->addChild( survscene->osgNode() );
    mAttachCB( survscene->mousecursorchange, OD3DViewer::mouseCursorChg );
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


SceneID OD3DViewer::getSceneID() const
{
    ConstRefMan<visBase::Scene> scene = getScene();
    if ( !scene || scene->id().isUdf() )
	return SceneID::udf();

    return SceneID( scene->id().asInt() );
}


void OD3DViewer::mouseCursorChg( CallBacker* )
{
    updateActModeCursor();
}


void OD3DViewer::updateActModeCursor()
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


void OD3DViewer::align()
{
}


Geom::Size2D<int> OD3DViewer::getViewportSizePixels() const
{
    osg::ref_ptr<const osg::Camera> camera = getOsgCamera();
    osg::ref_ptr<const osg::Viewport> vp = camera->getViewport();

    return Geom::Size2D<int>( mNINT32(vp->width()), mNINT32(vp->height()) );
}


void OD3DViewer::viewAll( bool animate )
{
    if ( !view_ )
	return;

    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( manip )
	manip->viewAll( view_, animate );

    requestRedraw();
}


void OD3DViewer::setAnimationEnabled( bool yn )
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( manip )
	manip->setAllowThrow( yn );
}


bool OD3DViewer::isAnimationEnabled() const
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    return manip ? manip->getAllowThrow() : false;
}


void OD3DViewer::requestRedraw()
{
    if ( !view_ )
	return;

    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    const bool animating = manip->isThrown() || manip->isAnimating();
    if ( !animating )
	view_->requestRedraw();

    view_->requestContinuousUpdate( animating );
}


void OD3DViewer::setBackgroundColor( const OD::Color& col )
{
    RefMan<visBase::Scene> scene = getScene();
    if ( scene )
	scene->setBackgroundColor( col );
}


OD::Color OD3DViewer::getBackgroundColor() const
{
    ConstRefMan<visBase::Scene> scene = getScene();
    return scene ? scene->getBackgroundColor() : OD::Color::NoColor();
}


void OD3DViewer::setCameraPos( const osg::Vec3f& updir,
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


void OD3DViewer::viewPlaneX()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(1,0,0), false, true );
}


void OD3DViewer::viewPlaneY()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,1,0), false, true );
}


void OD3DViewer::viewPlaneZ()
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

    setCameraPos( newup, osg::Vec3d(0,0,1), true, true );
}


void OD3DViewer::viewPlaneN( bool animate )
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,-1,0), true, animate );
}


void OD3DViewer::viewPlaneYZ()
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


void OD3DViewer::viewPlaneInl( bool animate )
{
    osg::Vec3f inlvec;
    getInlCrlVec( inlvec, true );
    setCameraPos( osg::Vec3f(0,0,1), inlvec, false, animate );
}


void OD3DViewer::viewPlaneCrl()
{
    osg::Vec3f crlvec;
    getInlCrlVec( crlvec, false );
    setCameraPos( osg::Vec3f(0,0,1), crlvec, false, true );
}


bool OD3DViewer::isCameraPerspective() const
{
    osgGeo::TrackballManipulator* manip = getCameraManipulator();
    return !manip ? true : manip->isCameraPerspective();
}


bool OD3DViewer::isCameraOrthographic() const
{
    return !isCameraPerspective();
}


void OD3DViewer::setCameraPerspective( bool yn )
{
    osgGeo::TrackballManipulator* manip = getCameraManipulator();
    if ( manip )
	manip->setProjectionAsPerspective( yn );
}


void OD3DViewer::toggleCameraType()
{
    if ( mapview_ ) return;

    osgGeo::TrackballManipulator* manip = getCameraManipulator();
    if ( !manip ) return;

    manip->setProjectionAsPerspective( !manip->isCameraPerspective() );

    requestRedraw();
}


void OD3DViewer::uiRotate( float angle, bool horizontal )
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


void OD3DViewer::notifyManipulatorMovement( float dh, float dv, float df )
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


void OD3DViewer::uiZoom( float rel, const osg::Vec3f* dir )
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


void OD3DViewer::setCameraZoom( float val )
{
    osg::ref_ptr<osg::Camera> cam = getOsgCamera();

    double fovy, aspr, znear, zfar;
    if ( cam && cam->getProjectionMatrixAsPerspective(fovy,aspr,znear,zfar) )
	cam->setProjectionMatrixAsPerspective( val, aspr, znear, zfar );
}


float OD3DViewer::getCameraZoom() const
{
    osg::ref_ptr<const osg::Camera> cam = getOsgCamera();

    double fovy, aspr, znear, zfar;
    if ( !cam || !cam->getProjectionMatrixAsPerspective(fovy,aspr,znear,zfar) )
	return 0.0;

    return fovy;
}


void OD3DViewer::fillCameraPos( IOPar& par ) const
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


bool OD3DViewer::useCameraPos( const IOPar& par )
{
    if ( par.isEmpty() )
	return false;

    const PtrMan<IOPar> survhomepospar = SI().pars().subselect( sKeyHomePos() );
    if ( !survhomepospar )
	fillCameraPos( homepos_ );
    else if ( isHomePosEmpty() )
	setHomePos( *survhomepospar );

    double x( 0 ), y( 0 ), z( 0 ), w( 0 );
    double distance( 0 );
    Coord3 center;
    if ( !par.get( sKeyCameraRotation(), x,y,z,w ) ||
	 !par.get( sKeyManipCenter(), center ) ||
	 !par.get( sKeyManipDistance(), distance ) )
	return false;

    osgGeo::TrackballManipulator* manip = getCameraManipulator();
    if ( !manip ) return false;

    manip->setCenter( Conv::to<osg::Vec3d>(center) );
    manip->setRotation( osg::Quat(x,y,z,w) );
    manip->setDistance( distance );

    requestRedraw();
    return true;

}


const osgViewer::View* OD3DViewer::getOsgViewerMainView() const
{
    return static_cast<osgViewer::View*>(view_);
}


const osgViewer::View* OD3DViewer::getOsgViewerHudView() const
{
    return static_cast<osgViewer::View*>(hudview_);
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

    const FilePath fp( GetDataDir(), "homepos.par" );
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


void OD3DViewer::setHomePos( const IOPar& homepos )
{
    homepos_ = homepos;
}


void OD3DViewer::toHomePos()
{
    useCameraPos( homepos_ );
}


void OD3DViewer::saveHomePos()
{
    homepos_.setEmpty();
    fillCameraPos( homepos_ );

    IOPar& pars = SI().getPars();
    pars.removeSubSelection( preOdHomePosition() );
    pars.removeSubSelection( sKeyHomePos() );
    pars.mergeComp( homepos_, sKeyHomePos() );
    SI().savePars();
}


void OD3DViewer::resetHomePos()
{
    homepos_.setEmpty();
    SI().getPars().removeSubSelection( sKeyHomePos() );
    SI().savePars();
}


bool OD3DViewer::isHomePosEmpty() const
{
    return homepos_.isEmpty();
}

void OD3DViewer::setStartupView()
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


void OD3DViewer::setScenesPixelDensity( float dpi )
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


bool OD3DViewer::setStereoType( OD::StereoType st )
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


OD::StereoType OD3DViewer::getStereoType() const
{
    return stereotype_;
}


void OD3DViewer::setStereoOffset( float offset )
{
    osg::ref_ptr<osg::DisplaySettings> ds = osg::DisplaySettings::instance();
    stereooffset_ = offset;
    ds->setEyeSeparation( stereooffset_/100 );
    requestRedraw();
}


float OD3DViewer::getStereoOffset() const
{
    return stereooffset_;
}


void OD3DViewer::setMapView( bool yn )
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


void OD3DViewer::enableDragging( bool yn )
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip = getCameraManipulator();
    if ( !manip )
	return;

    if ( yn )
	manip->enableDragging( true );
    else
	manip->enableDragging((int)osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON);
}
