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
#include <osg/ComputeBoundsVisitor>
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

#include "uiobjbody.h"
#include "viscamera.h"
//#include "viscamerainfo.h"
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
#include <QPainter>

 
#define col2f(rgb) float(col.rgb())/255

#include "uiobjbody.h"
#include "keystrs.h"

#include "survinfo.h"
#include "viscamera.h"
#include "vissurvscene.h"
#include "visdatagroup.h"

static const char* sKeydTectScene()	{ return "dTect.Scene."; }

DefineEnumNames(ui3DViewer,StereoType,0,"StereoType")
{ sKey::None().str(), "RedCyan", "QuadBuffer", 0 };



class uiDirectViewBody : public ui3DViewerBody
{
public:
				uiDirectViewBody(ui3DViewer&,uiParent*);

    const mQtclass(QWidget)* 	qwidget_() const;
    

    virtual uiSize		minimumSize() const 
    				{ return uiSize(200,200); }

protected:
    void			updateActModeCursor();
    osgGA::GUIActionAdapter&	getActionAdapter() {return *graphicswin_.get();}
    osg::GraphicsContext*	getGraphicsContext(){return graphicswin_.get();}

    bool			mousebutdown_;
    float			zoomfactor_;

    //visBase::CameraInfo*	camerainfo_;
    MouseCursor			actmodecursor_;

    osg::ref_ptr<osgQt::GraphicsWindowQt>	graphicswin_;
};


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
{
    sceneroot_->ref();
    viewport_->ref();
    eventfilter_.addEventType( uiEventFilter::KeyPress );
    eventfilter_.addEventType( uiEventFilter::Resize );
    eventfilter_.addEventType( uiEventFilter::Show );

    mAttachCB( eventfilter_.eventhappened, ui3DViewerBody::qtEventCB );
}


ui3DViewerBody::~ui3DViewerBody()
{			
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
}

#define mMainCameraOrder    0
#define mHudCameraOrder	    (mMainCameraOrder+1)

void ui3DViewerBody::setupHUD()
{
    if ( hudview_ )
	return;

    osg::ref_ptr<osg::Camera> hudcamera = new osg::Camera;
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
    hudcamera->addChild( hudscene_->osgNode() );
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
    }
}


void ui3DViewerBody::setupView()
{
    camera_ = visBase::Camera::create();
    if ( axes_ )
	axes_->setMasterCamera( camera_ );
    if ( polygonselection_ )
	polygonselection_->setMasterCamera( camera_ );
    mDynamicCastGet(osg::Camera*, osgcamera, camera_->osgNode() );
    osgcamera->setGraphicsContext( getGraphicsContext() );
    osgcamera->setClearColor( osg::Vec4(0.0f, 0.0f, 0.5f, 1.0f) );
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
	    osgGA::StandardManipulator::DEFAULT_SETTINGS |
	    osgGA::StandardManipulator::SET_CENTER_ON_WHEEL_FORWARD_MOVEMENT );
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
    handler->addThumbWheel( (osgGeo::ThumbWheel*) horthumbwheel_->osgNode() );
    handler->addThumbWheel( (osgGeo::ThumbWheel*) verthumbwheel_->osgNode() );
    osgcamera->addEventCallback( handler );

    // Camera projection must be initialized before computing home position
    reSizeEvent( 0 );
}


uiObject& ui3DViewerBody::uiObjHandle()
{ return handle_; }


osgViewer::CompositeViewer* ui3DViewerBody::getCompositeViewer()
{
    static osg::ref_ptr<osgViewer::CompositeViewer> viewer = 0;
    if ( !viewer )
    {
	viewer = new osgViewer::CompositeViewer;
	viewer->setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
	viewer->getEventVisitor()->setTraversalMask(
					visBase::cEventTraversalMask() );
	viewer->setKeyEventSetsDone( 0 );
	osgQt::setViewer( viewer.get() );
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

    const double aspectratio = static_cast<double>(widget->width())/
	static_cast<double>(widget->height());

    osgcamera->setProjectionMatrixAsPerspective( 45.0f, aspectratio,
						  1.0f, 10000.0f );
    hudview_->getCamera()->setProjectionMatrix(
	osg::Matrix::ortho2D(0,widget->width(),0,widget->height() ));

    horthumbwheel_->setPosition( true, 20, 10, 100, 15, -1 );
    verthumbwheel_->setPosition( false, 10, 20, 100, 15, -1 );
    const float offset = axes_->getLength() + 10;
    axes_->setPosition( widget->width()-offset, offset );
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
}


void ui3DViewerBody::toggleViewMode(CallBacker* cb )
{
    setViewMode( !isViewMode(), true );
}


void ui3DViewerBody::showRotAxis( bool yn )
{
    axes_->turnOn( yn );
}


visBase::PolygonSelection* ui3DViewerBody::getPolygonSelector() const
{
    return polygonselection_;
}


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
}


void ui3DViewerBody::qtEventCB( CallBacker* )
{
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
	{
	    toggleViewMode( 0 );
	}
    }
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
}


void ui3DViewerBody::align()
{
    /*
    SoCamera* cam = getCamera();
    if ( !cam ) return;
    osg::Vec3f dir;
    cam->orientation.getValue().multVec( osg::Vec3f(0,0,-1), dir );

    osg::Vec3f focalpoint = cam->position.getValue() +
				cam->focalDistance.getValue() * dir;
				osg::Vec3f upvector( dir[0], dir[1], 1 );
    cam->pointAt( focalpoint, upvector );
    */
}


Geom::Size2D<int> ui3DViewerBody::getViewportSizePixels() const
{
    osg::ref_ptr<const osg::Camera> camera = getOsgCamera();
    osg::ref_ptr<const osg::Viewport> vp = camera->getViewport();

    return Geom::Size2D<int>( mNINT32(vp->width()), mNINT32(vp->height()) );
}


void ui3DViewerBody::setCameraPos( const osg::Vec3f& updir, 
				  const osg::Vec3f& focusdir,
				  bool usetruedir )
{
    /*
    osg::Camera* cam = getOsgCamera();
    if ( !cam ) return;

    osg::Vec3f dir;
    cam->orientation.getValue().multVec( osg::Vec3f(0,0,-1), dir );

#define mEps 1e-6
    const float angletofocus = Math::ACos( dir.dot(focusdir) );
    if ( mIsZero(angletofocus,mEps) ) return;

    const float diffangle = fabs(angletofocus) - M_PI_2;
    const int sign =
    	usetruedir || mIsZero(diffangle,mEps) || diffangle < 0 ? 1 : -1;
    osg::Vec3f focalpoint = cam->position.getValue() +
			 cam->focalDistance.getValue() * sign * focusdir;
    cam->pointAt( focalpoint, updir );

    cam->position = focalpoint + cam->focalDistance.getValue() * sign *focusdir;
    viewAll();
    */
}


void ui3DViewerBody::viewPlaneX()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(1,0,0), false );
}


void ui3DViewerBody::viewAll()
{
    if ( !view_ )
	return;

    osg::ref_ptr<osgGA::CameraManipulator> manip =
	static_cast<osgGA::CameraManipulator*>(
	    view_->getCameraManipulator() );

    if ( !manip )
	return;

    osg::ref_ptr<osgGA::GUIEventAdapter>& ea =
	osgGA::GUIEventAdapter::getAccumulatedEventState();
    if ( !ea )
	return;

    computeViewAllPosition();
    manip->home( *ea, getActionAdapter() );
}



void ui3DViewerBody::computeViewAllPosition()
{
    if ( !view_ )
	return;

    osg::ref_ptr<osgGA::CameraManipulator> manip =
    static_cast<osgGA::CameraManipulator*>(
				view_->getCameraManipulator() );

    osg::ref_ptr<osg::Node> node = manip ? manip->getNode() : 0;
    if ( !node )
	return;

    osg::ComputeBoundsVisitor visitor(
			    osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN);
    visitor.setNodeMaskOverride( visBase::cBBoxTraversalMask() );
    node->accept(visitor);
    osg::BoundingBox &bb = visitor.getBoundingBox();

    osg::BoundingSphere boundingsphere;


    if ( bb.valid() ) boundingsphere.expandBy(bb);
    else boundingsphere = node->getBound();

    // set dist to default
    double dist = 3.5f * boundingsphere.radius();

    // try to compute dist from frustrum
    double left,right,bottom,top,znear,zfar;
    if ( getOsgCamera()->getProjectionMatrixAsFrustum(
					    left,right,bottom,top,znear,zfar) )
    {
	double vertical2 = fabs(right - left) / znear / 2.;
	double horizontal2 = fabs(top - bottom) / znear / 2.;
	double dim = horizontal2 < vertical2 ? horizontal2 : vertical2;
	double viewangle = atan2(dim,1.);
	dist = boundingsphere.radius() / sin(viewangle);
    }
    else
    {
	// try to compute dist from ortho
	if ( getOsgCamera()->getProjectionMatrixAsOrtho(
					    left,right,bottom,top,znear,zfar) )
	{
	    dist = fabs(zfar - znear) / 2.;
	}
    }

    // set home position
    manip->setHomePosition(boundingsphere.center() + osg::Vec3d(0.0,-dist,0.0f),
		    boundingsphere.center(),
		    osg::Vec3d(0.0f,0.0f,1.0f),
		    false );
}


void ui3DViewerBody::viewPlaneY()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,1,0), false );
}


void ui3DViewerBody::setBackgroundColor( const Color& col )
{
    getOsgCamera()->setClearColor( osg::Vec4(col2f(r),col2f(g),col2f(b), 1.0) );
}


Color ui3DViewerBody::getBackgroundColor() const
{
    const osg::Vec4 col = getOsgCamera()->getClearColor();
    return Color( mNINT32(col.r()*255), mNINT32(col.g()*255),
		  mNINT32(col.b()*255) );
}


void ui3DViewerBody::viewPlaneN()
{
    setCameraPos( osg::Vec3f(0,0,1), osg::Vec3f(0,1,0), true );
}


void ui3DViewerBody::viewPlaneZ()
{
    osg::Camera* cam = getOsgCamera();
    if ( !cam ) return;

    osg::Vec3f curdir;
    //cam->orientation.getValue().multVec( osg::Vec3f(0,0,-1), curdir );
    //setCameraPos( curdir, osg::Vec3f(0,0,-1), true );
}


static void getInlCrlVec( osg::Vec3f& vec, bool inl )
{
    const Pos::IdxPair2Coord& b2c = SI().binID2Coord();
    const Pos::IdxPair2Coord::DirTransform& xtr = b2c.getTransform(true);
    const Pos::IdxPair2Coord::DirTransform& ytr = b2c.getTransform(false);
    const float det = xtr.det( ytr );
    if ( inl )
	vec = osg::Vec3f( ytr.c/det, -xtr.c/det, 0 );
    else
	vec = osg::Vec3f( -ytr.b/det, xtr.b/det, 0 );
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
    return !camera_->isOrthogonal();
}


bool ui3DViewerBody::isCameraOrthographic() const
{
    return camera_->isOrthogonal();
}


void ui3DViewerBody::toggleCameraType()
{
    //TODO
}


void ui3DViewerBody::setHomePos()
{
    if ( !camera_ ) return;

    PtrMan<IOPar> homepospar =
	SI().pars().subselect( ui3DViewer::sKeyHomePos() );
    if ( !homepospar )
	return;

    camera_->usePar( *homepospar );
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
	homepos_ = *homepospar;

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

void ui3DViewer::viewAll()
{
    mDynamicCastGet(visSurvey::Scene*,survscene,getScene());
    if ( !survscene )
    {
	osgbody_->viewAll();
    }
    else
    {
	bool showtext = survscene->isAnnotTextShown();
	bool showscale = survscene->isAnnotScaleShown();
	if ( !showtext && !showscale )
	{
	    osgbody_->viewAll();
	}
	else
	{
	    survscene->showAnnotText( false );
	    survscene->showAnnotScale( false );
	    osgbody_->viewAll();
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
	    case YZ: osgbody_->viewPlaneN(); osgbody_->viewPlaneZ(); break;
    }

    viewAll();
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


void ui3DViewer::switchSeekMode()
{
    pErrMsg( "Not impl yet" );
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
    RefMan<visBase::Camera> camera = osgbody_->getVisCamera();
    if ( !camera ) return;

    camera->usePar( homepos_ );
}


void ui3DViewer::saveHomePos()
{
    RefMan<visBase::Camera> camera = osgbody_->getVisCamera();

    if ( !camera ) return;

    camera->fillPar( homepos_ );
    homepos_.removeWithKey( sKey::Type() );
    SI().getPars().mergeComp( homepos_, sKeyHomePos() );
    SI().savePars();
}


void ui3DViewer::showRotAxis( bool yn )
{
    // OSG-TODO
    osgbody_->showRotAxis( yn );
}


bool ui3DViewer::rotAxisShown() const
{
    pErrMsg("Not impl");
    return false;
}


visBase::PolygonSelection* ui3DViewer::getPolygonSelector() const
{
    return osgbody_->getPolygonSelector();
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

    RefMan<visBase::Camera> camera = osgbody_->getVisCamera();

    camera->fillPar( par );

    par.mergeComp( homepos_, sKeyHomePos() );
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

    if ( homepos && homepos_.isEmpty() )
	homepos_ = *homepos;

    RefMan<visBase::Camera> camera = osgbody_->getVisCamera();

    bool res = camera->usePar( par ) == 1;
    
    PtrMan<IOPar> survhomepospar = SI().pars().subselect( sKeyHomePos() );
    if ( !survhomepospar )
	saveHomePos();

    return res;
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

