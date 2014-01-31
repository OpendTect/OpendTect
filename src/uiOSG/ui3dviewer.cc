/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "ui3dviewer.h"

#include "uicursor.h"
#include "ui3dviewerbody.h"
#include "ui3dindirectviewer.h"
#include "uirgbarray.h"

#include <osgQt/GraphicsWindowQt>
#include <osgViewer/View>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGeo/TrackballManipulator>
#include <osg/MatrixTransform>
#include <osgGeo/ThumbWheel>


#include "envvars.h"
#include "iopar.h"
#include "keybindings.h"
#include "keystrs.h"
#include "math2.h"
#include "uicursor.h"
#include "ptrman.h"
#include "settings.h"
#include "survinfo.h"
#include "visaxes.h"
#include "visscenecoltab.h"

#include "uiobjbody.h"
#include "viscamera.h"
#include "visdatagroup.h"
#include "visdataman.h"
#include "vispolygonselection.h"
#include "visscene.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistext.h"
#include "visthumbwheel.h"

#include <iostream>
#include <math.h>

#include "i_layout.h"
#include "i_layoutitem.h"

#include <QTabletEvent>
#include <QGestureEvent>
#include <QGesture>
#include <QPainter>


#define col2f(rgb) float(col.rgb())/255

#include "uiobjbody.h"
#include "keystrs.h"

#include "survinfo.h"
#include "viscamera.h"
#include "vissurvscene.h"
#include "visdatagroup.h"

static const char* sKeydTectScene()	{ return "dTect.Scene."; }
static const char* sKeyManipCenter()	{ return "Manipulator Center"; }
static const char* sKeyManipDistance()	{ return "Manipulator Distance"; }
static const char* preOdHomePosition()	{return "Home position.Aspect ratio";}
static const char* sKeyHomePos()	{ return "Home position"; }
static const char* sKeyCameraRotation() { return "Camera Rotation"; }


DefineEnumNames(ui3DViewer,StereoType,0,"StereoType")
{ sKey::None().str(), "RedCyan", "QuadBuffer", 0 };


class TrackBallManipulatorMessenger : public osg::NodeCallback
{
public:
    TrackBallManipulatorMessenger( ui3DViewerBody* t )
        : viewerbody_( t )
    {}
    void	operator() (osg::Node* node, osg::NodeVisitor* nv )
    {
        if ( viewerbody_ && nv )
        {
            osgGeo::TrackballEventNodeVisitor* tnv =
                (osgGeo::TrackballEventNodeVisitor*) nv;

            viewerbody_->notifyManipulatorMovement(
               tnv->_deltahorangle, tnv->_deltavertangle, tnv->_distfactor );
        }
    }
    void	detach() { viewerbody_ = 0; }

protected:
    TrackBallManipulatorMessenger()
    {
        
    }
    ui3DViewerBody*	viewerbody_;
};



class uiDirectViewBody : public ui3DViewerBody
{
public:
				uiDirectViewBody(ui3DViewer&,uiParent*);

    const mQtclass(QWidget)*	qwidget_() const;


    virtual uiSize		minimumSize() const
				{ return uiSize(200,200); }

protected:
    void			updateActModeCursor();
    osgViewer::GraphicsWindow&	getGraphicsWindow(){return *graphicswin_.get();}
    osg::GraphicsContext*	getGraphicsContext(){return graphicswin_.get();}

    bool			mousebutdown_;
    float			zoomfactor_;

    //visBase::CameraInfo*	camerainfo_;
    MouseCursor			actmodecursor_;

    osg::ref_ptr<osgQt::GraphicsWindowQt>	graphicswin_;
};


uiDirectViewBody::uiDirectViewBody( ui3DViewer& hndl, uiParent* parnt )
    : ui3DViewerBody( hndl, parnt )
    , mousebutdown_(false)
    , zoomfactor_( 1 )
{
    osg::ref_ptr<osg::DisplaySettings> ds = osg::DisplaySettings::instance();
    osgQt::GLWidget* glw = new osgQt::GLWidget( parnt->pbody()->managewidg() );

    eventfilter_.attachToQObj( glw );

    graphicswin_ = new osgQt::GraphicsWindowQt( glw );
    setStretch(2,2);

    setupHUD();
    setupView();
    setupTouch();
}

const mQtclass(QWidget)* uiDirectViewBody::qwidget_() const
{ return graphicswin_->getGLWidget(); }


void uiDirectViewBody::updateActModeCursor()
{
    /*
    if ( isViewing() )
	return;

    if ( !isCursorEnabled() )
	return;

    QCursor qcursor;
    uiCursorManager::fillQCursor( actmodecursor_, qcursor );

    getGLWidget()->setCursor( qcursor );
    */
}


//--------------------------------------------------------------------------


ui3DViewerBody::ui3DViewerBody( ui3DViewer& h, uiParent* parnt )
    : uiObjectBody( parnt, 0 )
    , handle_( h )
    , printpar_(*new IOPar)
    , sceneroot_( new osg::Group )
    , hudview_( 0 )
    , hudscene_( 0 )
    , viewport_( new osg::Viewport )
    , compositeviewer_( 0 )
    , axes_( 0 )
    , polygonselection_( 0 )
    , visscenecoltab_( 0 )
    , manipmessenger_( new TrackBallManipulatorMessenger( this ) )
    , setinitialcamerapos_( true )
{
    manipmessenger_->ref();
    sceneroot_->ref();
    viewport_->ref();
    eventfilter_.addEventType( uiEventFilter::KeyPress );
    eventfilter_.addEventType( uiEventFilter::Resize );
    eventfilter_.addEventType( uiEventFilter::Show );
    eventfilter_.addEventType( uiEventFilter::Gesture );

    mAttachCB( eventfilter_.eventhappened, ui3DViewerBody::qtEventCB );
}


ui3DViewerBody::~ui3DViewerBody()
{
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
    sceneroot_->unref();
    detachAllNotifiers();
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
    hudview_->setSceneData( hudscene_->osgNode() );
    if ( !compositeviewer_ )
    {
	compositeviewer_ = getCompositeViewer();
        compositeviewer_->setRunFrameScheme( osgViewer::ViewerBase::ON_DEMAND );
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


    if ( !axes_ )
    {
	axes_ = visBase::Axes::create();
	axes_->setSize( 5.0f, 55.0f );
	hudcamera->addChild( axes_->osgNode() );
	if ( camera_ )
	    axes_->setMasterCamera( camera_ );
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
	visscenecoltab_->setAnnotFont( FontData() );
	visscenecoltab_->turnOn( false );
	visscenecoltab_->setPos( visBase::SceneColTab::Bottom );
    }
}


void ui3DViewerBody::setupTouch()
{
    qwidget()->grabGesture(Qt::PinchGesture);
}


void ui3DViewerBody::setupView()
{
    camera_ = visBase::Camera::create();
    if ( axes_ )
	axes_->setMasterCamera( camera_ );
    if ( polygonselection_ )
	polygonselection_->setMasterCamera( camera_ );

    if ( scene_ )
        scene_->setCamera( camera_ );

    mDynamicCastGet(osg::Camera*, osgcamera, camera_->osgNode(true) );
    osgcamera->setGraphicsContext( getGraphicsContext() );
    osgcamera->setClearColor( osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f) );
    osgcamera->setViewport( viewport_ );
    osgcamera->setRenderOrder(osg::Camera::POST_RENDER, mMainCameraOrder );

    view_ = new osgViewer::View;
    view_->setCamera( osgcamera );
    view_->setSceneData( sceneroot_ );
    view_->addEventHandler( new osgViewer::StatsHandler );

    // Unlike Coin, default OSG headlight has zero ambiance
    view_->getLight()->setAmbient( osg::Vec4(0.6f,0.6f,0.6f,1.0f) );

    osg::ref_ptr<osgGeo::TrackballManipulator> manip =
	new osgGeo::TrackballManipulator(
	    osgGA::StandardManipulator::DEFAULT_SETTINGS );

    manip->addMovementCallback( manipmessenger_ );
    manip->setBoundTraversalMask( visBase::cBBoxTraversalMask() );
    manip->setIntersectTraversalMask( visBase::cIntersectionTraversalMask() );
    manip->setAnimationTime( 0.5 );

    manip->enableDragging( isViewMode() );

    manip->setAutoComputeHomePosition( false );

    view_->setCameraManipulator( manip.get() );

    if ( !compositeviewer_ )
    {
	compositeviewer_ = getCompositeViewer();
	compositeviewer_->ref();
    }

    compositeviewer_->addView( view_ );

    // To put exaggerated bounding sphere radius offside
    manip->setMinimumDistance( 0 );

    osgGeo::ThumbWheelEventHandler* handler= new osgGeo::ThumbWheelEventHandler;

    handler->addThumbWheel(
		(osgGeo::ThumbWheel*) horthumbwheel_->osgNode(true) );
    handler->addThumbWheel(
		 (osgGeo::ThumbWheel*) verthumbwheel_->osgNode(true) );
    handler->addThumbWheel(
		 (osgGeo::ThumbWheel*) distancethumbwheel_->osgNode(true) );

    view_->getSceneData()->addEventCallback( handler );

    // Camera projection must be initialized before computing home position
    reSizeEvent( 0 );
}


uiObject& ui3DViewerBody::uiObjHandle()
{ return handle_; }


osgViewer::CompositeViewer* ui3DViewerBody::getCompositeViewer()
{
    mDefineStaticLocalObject( Threads::Lock, lock, (true) );
    Threads::Locker locker ( lock );
    mDefineStaticLocalObject( osg::ref_ptr<osgViewer::CompositeViewer>,
			      viewer, = 0 );
    if ( !viewer )
    {
	viewer = new osgViewer::CompositeViewer;
	viewer->setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
	viewer->getEventVisitor()->setTraversalMask(
					visBase::cEventTraversalMask() );
	viewer->setKeyEventSetsDone( 0 );
	osgQt::setViewer( viewer.get() );
        visBase::DataObject::setCommonViewer( viewer );
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
	const QPointF qcenter = pinch->centerPoint();
	const osg::Vec2 center( qcenter.x(), qcenter.y() );
	const float angle = pinch->rotationAngle();
	const float scale = pinch->scaleFactor();

	//We don't have absolute positions of the two touches, only a scale and
	//rotation. Hence we create pseudo-coordinates which are reasonable, and
	//centered around the real position
	const float radius = (qwidget()->width()+qwidget()->height())/4;
	const osg::Vec2 vector(scale*cos(angle)*radius,scale*sin(angle)*radius);
	const osg::Vec2 p0 = center+vector;
	const osg::Vec2 p1 = center-vector;

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

    horthumbwheel_->setPosition( true, mShortSideDistance+mThumbWheelLen/2,
                                 mLongSideDistance+mThumbWheelWidth/2,
                                 mThumbWheelLen, mThumbWheelWidth, mZCoord );
    verthumbwheel_->setPosition( false, mLongSideDistance+mThumbWheelWidth/2,
                                 mShortSideDistance+mThumbWheelLen/2,
                                 mThumbWheelLen, mThumbWheelWidth, mZCoord );
    distancethumbwheel_->setPosition( false,
        mLongSideDistance+mThumbWheelWidth/2,
        widget->height()-mShortSideDistance-mThumbWheelLen/2,
        mThumbWheelLen, mThumbWheelWidth, mZCoord );
    const float offset = axes_->getLength() + 10;
    axes_->setPosition( widget->width()-offset, offset );

    if ( visscenecoltab_ )
	visscenecoltab_->setWindowSize( widget->width(), widget->height() );
}


void ui3DViewerBody::thumbWheelRotationCB(CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(float, deltaangle, caller, cb );
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
        	static_cast<osgGeo::TrackballManipulator*>(
                                                view_->getCameraManipulator() );
        manip->changeDistance( deltaangle/M_PI );

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


void ui3DViewerBody::setAxisAnnotColor( const Color& color )
{
    axes_->setAnnotationColor( color );
}


visBase::PolygonSelection* ui3DViewerBody::getPolygonSelector() const
{
    return polygonselection_;
}

visBase::SceneColTab* ui3DViewerBody::getSceneColTab() const
{
    return visscenecoltab_;
}

void ui3DViewerBody::qtEventCB( CallBacker* )
{
    if ( eventfilter_.getCurrentEventType()==
	uiEventFilter::Gesture )
    {
	QGestureEvent* gestureevent =
	    static_cast<QGestureEvent*> ( eventfilter_.getCurrentEvent() );

	handleGestureEvent( gestureevent );
    }

    if ( eventfilter_.getCurrentEventType()== 
	uiEventFilter::Show && setinitialcamerapos_ )
    {
	viewAll( false );
    }

    if ( eventfilter_.getCurrentEventType()==uiEventFilter::Resize ||
	 eventfilter_.getCurrentEventType()==uiEventFilter::Show )
    {
	reSizeEvent( 0 );
    }
    else
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


bool ui3DViewerBody::isViewMode() const
{
    return scene_ && !scene_->isPickable();
}


#define ROTATE_WIDTH 16
#define ROTATE_HEIGHT 16
#define ROTATE_BYTES ((ROTATE_WIDTH + 7) / 8) * ROTATE_HEIGHT
#define ROTATE_HOT_X 6
#define ROTATE_HOT_Y 8

static unsigned char rotate_bitmap[ROTATE_BYTES] = {
    0xf0, 0xef, 0x18, 0xb8, 0x0c, 0x90, 0xe4, 0x83,
    0x34, 0x86, 0x1c, 0x83, 0x00, 0x81, 0x00, 0xff,
    0xff, 0x00, 0x81, 0x00, 0xc1, 0x38, 0x61, 0x2c,
    0xc1, 0x27, 0x09, 0x30, 0x1d, 0x18, 0xf7, 0x0f
};

static unsigned char rotate_mask_bitmap[ROTATE_BYTES] = {
    0xf0,0xef,0xf8,0xff,0xfc,0xff,0xfc,0xff,0x3c,0xfe,0x1c,0xff,0x00,0xff,0x00,
    0xff,0xff,0x00,0xff,0x00,0xff,0x38,0x7f,0x3c,0xff,0x3f,0xff,0x3f,0xff,0x1f,
    0xf7,0x0f
};



void ui3DViewerBody::setViewMode( bool yn, bool trigger )
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip =
	    static_cast<osgGeo::TrackballManipulator*>(
						view_->getCameraManipulator() );
    manip->enableDragging( yn );

    if ( scene_ )
	scene_->setPickable( !yn );

    MouseCursor cursor;
    if ( yn )
    {
	cursor.shape_ = MouseCursor::Bitmap;

	uiRGBArray* cursorimage = new uiRGBArray( true );
	cursorimage->setSize( ROTATE_WIDTH, ROTATE_HEIGHT );
	cursorimage->putFromBitmap( rotate_bitmap, rotate_mask_bitmap );

	cursor.image_ = cursorimage;
	cursor.hotx_ = ROTATE_HOT_X;
	cursor.hoty_ = ROTATE_HOT_Y;

    }
    else
    {
	cursor.shape_ = MouseCursor::Arrow;
    }

    mQtclass(QCursor) qcursor;
    uiCursorManager::fillQCursor( cursor, qcursor );
    qwidget()->setCursor( qcursor );

    if ( trigger )
	handle_.viewmodechanged.trigger( handle_ );
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


void ui3DViewerBody::setSceneID( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObject( sceneid );
    mDynamicCastGet(visBase::Scene*,newscene,obj)
    if ( !newscene ) return;

    sceneroot_->addChild( newscene->osgNode() );
    scene_ = newscene;

    if ( camera_ ) newscene->setCamera( camera_ );
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


void ui3DViewerBody::setCameraPos( const osg::Vec3f& updir,
				  const osg::Vec3f& viewdir,
				  bool usetruedir )
{
    osg::ref_ptr<osgGeo::TrackballManipulator> manip =
	static_cast<osgGeo::TrackballManipulator*>(
	view_->getCameraManipulator() );

    manip->viewAll( view_, viewdir, updir,true );
    requestRedraw();
}


void ui3DViewerBody::viewPlaneX()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(1,0,0), false );
}


void ui3DViewerBody::viewAll( bool animate )
{
    if ( !view_ )
	return;

    osg::ref_ptr<osgGeo::TrackballManipulator> manip =
        static_cast<osgGeo::TrackballManipulator*>(
                                        view_->getCameraManipulator() );
    
    manip->viewAll( view_, animate );

    requestRedraw();
}


void ui3DViewerBody::requestRedraw()
{
    if ( !view_ )
	return;

    osg::ref_ptr<osgGeo::TrackballManipulator> manip =
    	static_cast<osgGeo::TrackballManipulator*>(
                                               view_->getCameraManipulator() );

    const bool animating = manip->isThrown() || manip->isAnimating();
    if ( !animating )
	view_->requestRedraw();

    view_->requestContinuousUpdate( animating );

}



void ui3DViewerBody::viewPlaneY()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,1,0), false );
}


void ui3DViewerBody::setBackgroundColor( const Color& col )
{
    osg::Vec4 osgcol(col2f(r),col2f(g),col2f(b), 1.0);
    getOsgCamera()->setClearColor( osgcol );

    horthumbwheel_->setBackgroundColor( col );
    verthumbwheel_->setBackgroundColor( col );
    distancethumbwheel_->setBackgroundColor( col );
}


Color ui3DViewerBody::getBackgroundColor() const
{
    const osg::Vec4 col = getOsgCamera()->getClearColor();
    return Color( mNINT32(col.r()*255), mNINT32(col.g()*255),
		  mNINT32(col.b()*255) );
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


void ui3DViewerBody::viewPlaneZ()
{
    const osg::ref_ptr<osgGeo::TrackballManipulator> manip =
	static_cast<osgGeo::TrackballManipulator*>(
	view_->getCameraManipulator() );

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
    osgGeo::TrackballManipulator* manip =
	static_cast<osgGeo::TrackballManipulator*>(
	view_->getCameraManipulator() );

    return !manip ? true : manip->isCameraPerspective();
}


bool ui3DViewerBody::isCameraOrthographic() const
{
    return !isCameraPerspective();
}


void ui3DViewerBody::toggleCameraType()
{
    osgGeo::TrackballManipulator* manip = 
	static_cast<osgGeo::TrackballManipulator*>(
	view_->getCameraManipulator() );
    
    if ( !manip ) return;
    
    manip->setProjectionAsPerspective(  !manip->isCameraPerspective() );

    requestRedraw();
}


void ui3DViewerBody::setHomePos(const IOPar& homepos)
{
    homepos_ = homepos;  
}


void ui3DViewerBody::resetToHomePosition()
{
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
    distancethumbwheel_->setAngle( df * M_PI+distancethumbwheel_->getAngle() );
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
    const osgGeo::TrackballManipulator* manip = 
	 static_cast<osgGeo::TrackballManipulator*>(
	 view_->getCameraManipulator() );

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
    
    osgGeo::TrackballManipulator* manip = 
	static_cast<osgGeo::TrackballManipulator*>( 
	view_->getCameraManipulator() );

    if ( !manip ) return false;

    manip->setCenter( Conv::to<osg::Vec3d>( center ) );
    manip->setRotation( osg::Quat( x, y, z, w ) );
    manip->setDistance( distance );

    requestRedraw();

    setinitialcamerapos_ =  false;

    return true;

}


void ui3DViewerBody::toHomePos()
{
    useCameraPos( homepos_ );
}

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

//------------------------------------------------------------------------------


ui3DViewer::ui3DViewer( uiParent* parnt, bool direct, const char* nm )
    : uiObject(parnt,nm,mkBody(parnt, direct, nm) )
    , destroyed(this)
    , viewmodechanged(this)
    , pageupdown(this)
    , vmcb(0)
{
    PtrMan<IOPar> homepospar = SI().pars().subselect( sKeyHomePos() );
    if ( homepospar )
	osgbody_->setHomePos( *homepospar) ;

    setViewMode( false );  // switches between view & interact mode

#define mGetProp(get,str,tp,var,func) \
    tp var; \
    res = Settings::common().get(BufferString(sKeydTectScene(),str),var);\
    if ( res ) func( var );

    bool res = false;
    mGetProp( get, sKeyBGColor(), Color, col, setBackgroundColor );
    mGetProp( getYN, sKeyAnimate(), bool, yn, enableAnimation );
}


ui3DViewer::~ui3DViewer()
{
    delete osgbody_;
}


uiObjectBody& ui3DViewer::mkBody( uiParent* parnt, bool direct, const char* nm )
{
    osgQt::initQtWindowingSystem();

    osgbody_ = direct
	? (ui3DViewerBody*) new uiDirectViewBody( *this, parnt )
	: (ui3DViewerBody*) new ui3DIndirectViewBody( *this, parnt );


    return *osgbody_;
}

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


void ui3DViewer::setBackgroundColor( const Color& col )
{
    osgbody_->setBackgroundColor( col );
}


void ui3DViewer::setAxisAnnotColor( const Color& col )
{
    osgbody_->setAxisAnnotColor( col );
}


Color ui3DViewer::getBackgroundColor() const
{
    return osgbody_->getBackgroundColor();
}


void ui3DViewer::getAllKeyBindings( BufferStringSet& keys )
{
}


void ui3DViewer::setKeyBindings( const char* keybindname )
{
}


const char* ui3DViewer::getCurrentKeyBindings() const
{
    return 0;
}


void ui3DViewer::viewPlane( PlaneType type )
{
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
    pErrMsg( "Not impl yet" );
    return false;
}


ui3DViewer::StereoType ui3DViewer::getStereoType() const
{
    return None;
}


void ui3DViewer::setStereoOffset( float offset )
{
    pErrMsg( "Not impl yet" );
}


float ui3DViewer::getStereoOffset() const
{
    pErrMsg( "Not impl yet" );
    return 0;
}


void ui3DViewer::setSceneID( int sceneid )
{
    osgbody_->setSceneID( sceneid );
}


int ui3DViewer::sceneID() const
{
    return osgbody_->getScene() ? osgbody_->getScene()->id() : -1;
}


void ui3DViewer::setViewMode( bool yn )
{
    if ( yn==isViewMode() )
	return;
    osgbody_->setViewMode( yn, false );
}


bool ui3DViewer::isViewMode() const
{
    return osgbody_->isViewMode();
}


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
{
    return osgbody_->getCameraPosition();
}


void ui3DViewer::align()
{
    osgbody_->align();
}


void ui3DViewer::toHomePos()
{
    osgbody_->toHomePos();
}


void ui3DViewer::saveHomePos()
{
    osgbody_->saveHomePos();
}


void ui3DViewer::showRotAxis( bool yn )
{
    // OSG-TODO
    osgbody_->showRotAxis( yn );
}


bool ui3DViewer::rotAxisShown() const
{
    return osgbody_->isAxisShown();
}


visBase::PolygonSelection* ui3DViewer::getPolygonSelector() const
{
    return osgbody_->getPolygonSelector();
}


visBase::SceneColTab* ui3DViewer::getSceneColTab() const
{
    return osgbody_->getSceneColTab();
}


void ui3DViewer::toggleCameraType()
{
    osgbody_->toggleCameraType();
}


Geom::Size2D<int> ui3DViewer::getViewportSizePixels() const
{
    return osgbody_->getViewportSizePixels();
}


void ui3DViewer::savePropertySettings() const
{
#define mSaveProp(set,str,func) \
    Settings::common().set( BufferString(sKeydTectScene(),str), func );

    mSaveProp( set, sKeyBGColor(), getBackgroundColor() );
    mSaveProp( setYN, sKeyAnimate(), isAnimationEnabled() );
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
    int sceneid;
    if ( !par.get(sKeySceneID(),sceneid) ) return false;

    setSceneID( sceneid );
    if ( !getScene() ) return false;

    int bgcol;
    if ( par.get(sKeyBGColor(),bgcol) )
    {
	Color newcol; newcol.setRgb( bgcol );
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



float ui3DViewer::getHeadOnLightIntensity() const
{
    return -1;
}

void ui3DViewer::setHeadOnLightIntensity( float value )
{
}


visBase::Scene* ui3DViewer::getScene()
{
    return osgbody_->getScene();
}


const visBase::Scene* ui3DViewer::getScene() const
{ return const_cast<ui3DViewer*>(this)->getScene(); }

