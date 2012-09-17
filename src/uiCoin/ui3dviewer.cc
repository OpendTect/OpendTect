/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          07/02/2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: ui3dviewer.cc,v 1.14 2012/07/10 13:06:05 cvskris Exp $";

#include "ui3dviewer.h"

#include "ui3dviewerbody.h"
#include "ui3dindirectviewer.h"

#include "envvars.h"
#include "iopar.h"
#include "keybindings.h"
#include "keystrs.h"
#include "math2.h"	
#include "uicursor.h"
#include "ptrman.h"
#include "settings.h"
#include "survinfo.h"
#include "SoAxes.h"
#include "ViewportRegion.h"

#include "uiobjbody.h"
#include "viscamera.h"
#include "viscamerainfo.h"
#include "visdatagroup.h"
#include "visdataman.h"
#include "visscene.h"
#include "vissurvscene.h"

#include <iostream>
#include <math.h>

#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>

#include <Inventor/SbColor.h>
#include <Inventor/SoSceneManager.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/system/gl.h>

#include "i_layout.h"
#include "i_layoutitem.h"

#include <QTabletEvent>
#include <QPainter>
#include "SoPolygonSelect.h"

 
#define col2f(rgb) float(col.rgb())/255

#include "uiobjbody.h"
#include "keystrs.h"

#include "survinfo.h"
#include "viscamera.h"
#include "vissurvscene.h"
#include "visdatagroup.h"

DefineEnumNames(ui3DViewer,StereoType,0,"StereoType")
{ sKey::None.str(), "RedCyan", "QuadBuffer", 0 };


// Start of SO stuff

class i_uiSoviewLayoutItem : public i_uiLayoutItem
{
public:
			i_uiSoviewLayoutItem(i_LayoutMngr&,uiSoViewerBody&);

protected:

    virtual void 	commitGeometrySet(bool isprefsz);
    uiSoViewerBody&	vwr;
};


class SoTabletEventFilter : public QObject
{
public:
    			SoTabletEventFilter()				{};
protected:
    bool		eventFilter(QObject*,QEvent*);
};


class uiSoViewerBody : public uiObjectBody, public SoQtExaminerViewer
{
public:
				uiSoViewerBody(ui3DViewer&,uiParent*,
					       const char*);
				~uiSoViewerBody();

    virtual const QWidget* 	qwidget_() const	{ return qwidg_; }

    void			setSceneID(int);
    visBase::Scene*		getScene()		{ return scene_; }
    const visBase::Scene*	getScene() const 	{ return scene_; }

    visBase::Camera*		getVisCamera() { return camera_; }

    void			anyWheelStart();
    void			anyWheelStop();

    void			align();
    void			viewPlaneX();
    void			viewPlaneY();
    void			viewPlaneZ();
    void			viewPlaneInl();
    void			viewPlaneCrl();
    void			viewPlaneN();

    void			uiRotateHor(float angle);
    void			uiRotateVer(float angle);
    void			uiZoom(float rel,const SbVec3f*);
    void			setCameraZoom(float);
    float			getCameraZoom();

    bool			isCameraPerspective() const;
    bool			isCameraOrthographic() const;

    void			showAxis( bool yn )
    				{ showaxes_ = yn; scheduleRedraw(); }
    bool			axisShown() const	{ return showaxes_; }
    void			setAxisAnnotColor(const Color&);
    void			mkAxes();

    void			setHomePos();
    void			saveHomePos();
    virtual void		setViewing(SbBool);
    void			uisetViewing(bool);
    virtual SbBool		processSoEvent(const SoEvent* const event);

    KeyBindMan&			keyBindMan()		{ return keybindman_; }

    virtual uiSize		minimumSize() const 
    				{ return uiSize(200,200); }

    SoNode*			getTotalSceneGraph();

protected:
    void			updateActModeCursor();

    virtual i_LayoutItem* 	mkLayoutItem_(i_LayoutMngr&);
    ui3DViewer&     		handle()		{ return handle_; }
    virtual uiObject&		uiObjHandle()		{ return handle_; }
    void			actualRedraw();

    void			setCameraPos(const SbVec3f& rotdir,
	    				     const SbVec3f&, bool usetruedir);
    SbBool			processMouseEvent(const SoMouseButtonEvent*);

    SoTabletEventFilter		tableteventfilter_;

    ui3DViewer&			handle_;
    QWidget*			qwidg_;

    IOPar&			printpar_;
    KeyBindMan&			keybindman_;

    SoOrthographicCamera*	axescamera_;
    SoAxes*			axes_;
    SoDirectionalLight*		axeslight_;
    SoSeparator*		owngraph_;
    bool			showaxes_;
    
    bool			mousebutdown_;
    float			zoomfactor_;

    visBase::CameraInfo*	camerainfo_;
    visBase::Scene*		scene_;
    visBase::Camera*		camera_;
    visBase::DataObjectGroup*	displayroot_;
    MouseCursor			actmodecursor_;

    static void			errorCB(const SoError*,void*);
};



uiSoViewerBody::uiSoViewerBody( ui3DViewer& hndl, uiParent* parnt, 
				const char* nm)
    : uiObjectBody(parnt,nm)
    , SoQtExaminerViewer(parnt && parnt->pbody() ?
			 parnt->pbody()->managewidg() : 0, nm, true)
    , handle_(hndl)
    , scene_(0)
    , camera_(0)
    , camerainfo_(0)
    , displayroot_(0)
    , axes_(0)
    , showaxes_(false)
    , printpar_(*new IOPar)
    , keybindman_(*new KeyBindMan)
    , mousebutdown_(false)
    , zoomfactor_( 1 )
    , axescamera_( 0 )
    , axeslight_( 0 )
    , owngraph_( 0 )
{
    setClearBeforeRender(FALSE, TRUE);
    qwidg_ = getWidget();
    setStretch(2,2);
    //Dont use use any sorted triangle stuff, as they don't work
    //well with remote display (bug 293).
    int trnsptyp = GetEnvVarIVal( "DTECT_COIN_TRANSPARENCY_TYPE",
	    	(int)SoGLRenderAction::SORTED_OBJECT_BLEND );
    setTransparencyType( (SoGLRenderAction::TransparencyType)trnsptyp );

    float hli = GetEnvVarDVal( "DTECT_COIN_HEADLIGHT_INTENSITY", 1 );
    setHeadlight( !mIsZero(hli,mDefEps) );
    getHeadlight()->intensity.setValue( hli );

    setPopupMenuEnabled( GetEnvVarYN("DTECT_COIN_ENABLE_POPUPMENU") );

    SoDebugError::setHandlerCallback( uiSoViewerBody::errorCB, this );
     
    mkAxes();

#ifndef use_SoQtRenderArea
    setDecoration(FALSE);
#endif

    qwidg_->installEventFilter( &tableteventfilter_ );
}


uiSoViewerBody::~uiSoViewerBody() 
{ 
    qwidg_->removeEventFilter( &tableteventfilter_ );
    handle_.destroyed.trigger(handle_);
    if ( displayroot_ ) displayroot_->unRef();
    delete &keybindman_;
    delete &printpar_;

    owngraph_->unref();
}


SbBool uiSoViewerBody::processMouseEvent( const SoMouseButtonEvent* event )
{
    if ( !event || event->getState()!=SoButtonEvent::DOWN )
	return false;

    if ( event->getButton()!=SoMouseButtonEvent::BUTTON4 &&
	 event->getButton()!=SoMouseButtonEvent::BUTTON5 )
	return false;

    mSettUse( get, "dTect", "Zoom factor", zoomfactor_ );
    if ( zoomfactor_<=0 )
	return false;

    SbVec2s mousepos = event->getPosition();
    SoCamera* cam = getCamera();
    const SbViewVolume vv = cam->getViewVolume();

    const SbVec2f normmousepos =
	    event->getNormalizedPosition(getViewportRegion());
    SbVec3f startpos, stoppos;
    vv.projectPointToLine( normmousepos, startpos, stoppos );
    SbVec3f direction = stoppos-startpos;
    direction.normalize();

    if ( event->getButton() == SoMouseButtonEvent::BUTTON4 )
	uiZoom( 0.1*zoomfactor_, &direction );
    else if ( event->getButton() == SoMouseButtonEvent::BUTTON5 )
	uiZoom( -0.1*zoomfactor_, &direction );

    return true;
}


SbBool uiSoViewerBody::processSoEvent( const SoEvent* const event )
{
    SoPolygonSelect::setActiveSceneManager( getSceneManager() );

    QWidget* qglwidget = getGLWidget();
    if ( qglwidget && !qglwidget->hasFocus() )
	qglwidget->setFocus();

    const SoType type( event->getTypeId() );
    const SoKeyboardEvent* keyevent = 0;
    if ( type.isDerivedFrom( SoKeyboardEvent::getClassTypeId() ) )
	keyevent = (SoKeyboardEvent*)event;

    SoMouseButtonEvent* mousebuttonevent = 0;
    if ( type.isDerivedFrom( SoMouseButtonEvent::getClassTypeId() ) )
	mousebuttonevent = (SoMouseButtonEvent*)event;

    if ( mousebuttonevent )
    {
	mousebutdown_ = mousebuttonevent->getState() == SoButtonEvent::DOWN;
	if ( processMouseEvent(mousebuttonevent) )
	    return true;
    }

    const bool stillviewing = isViewing() && !isAnimating();
    const bool hasbutevent = keyevent || mousebuttonevent || mousebutdown_;
    if ( stillviewing && !hasbutevent )
	SbBool res = SoQtRenderArea::processSoEvent( event );

    if ( keyevent && keyevent->getKey() == SoKeyboardEvent::Q )
	return true;

    if ( keyevent && ( keyevent->getKey() == SoKeyboardEvent::PAGE_UP ||
		       keyevent->getKey() == SoKeyboardEvent::PAGE_DOWN ) )
    {
	if ( keyevent->getState() == SoButtonEvent::DOWN ) return true;
	
	const bool isup = keyevent->getKey() == SoKeyboardEvent::PAGE_UP;
	handle_.pageupdown.trigger(isup);
	return true;
    }

    const SoEvent* keyev = 
	keybindman_.processSoEvent( event, isViewing(), isPopupMenuEnabled() );
    if ( !keyev )
	return SoQtExaminerViewer::processSoEvent( event );

    const SbBool res = SoQtExaminerViewer::processSoEvent( keyev );
    if ( !getInteractiveCount() )
	camerainfo_->setInteractive(false); //Will trigger redraw

    mDynamicCastGet(const visSurvey::Scene*,survscene,scene_);
    if ( survscene ) 
    {
	MouseCursor newcursor;
	const MouseCursor* mousecursor = survscene->getMouseCursor();
	if ( mousecursor && mousecursor->shape_!=MouseCursor::NotSet )
	    newcursor = *mousecursor;
	else
	    newcursor.shape_ = MouseCursor::Arrow;

	if ( newcursor!=actmodecursor_ )
	{
	    actmodecursor_ = newcursor;
	    updateActModeCursor();
	}
    }

    return res;
}


void uiSoViewerBody::updateActModeCursor()
{
    if ( isViewing() )
	return;

    if ( !isCursorEnabled() )
	return;

    QCursor qcursor;
    uiCursorManager::fillQCursor( actmodecursor_, qcursor );

    getGLWidget()->setCursor( qcursor );
}


void uiSoViewerBody::setViewing( SbBool yn )
{
    SoQtExaminerViewer::setViewing( yn );
    handle_.viewmodechanged.trigger(handle_);
    updateActModeCursor();
}


void uiSoViewerBody::uisetViewing( bool yn )
{
    SoQtExaminerViewer::setViewing( yn );
}


void uiSoViewerBody::setAxisAnnotColor( const Color& col )
{
    axes_->textcolor_.setValue( col2f(r), col2f(g), col2f(b) );
    scheduleRedraw();
}


void uiSoViewerBody::setSceneID( int sceneid )
{
    visBase::DataObject* obj = visBase::DM().getObject( sceneid );
    mDynamicCastGet(visBase::Scene*,newscene,obj)
    if ( !newscene ) return;

    if ( !displayroot_ )
    {
	displayroot_ = visBase::DataObjectGroup::create();
	displayroot_->ref();

	camerainfo_ = visBase::CameraInfo::create();
	displayroot_->addObject( camerainfo_ );

	camera_ = visBase::Camera::create();
	displayroot_->addObject( camera_ );

	mDynamicCastGet(const visSurvey::Scene*,survscene,newscene);
	if ( survscene ) 
	{
	    Coord minpos = SI().minCoord( true );
	    float middlez = (SI().zRange(true).start + SI().zRange(true).stop)
			  * 0.5 * survscene->getZStretch();
	    Coord3 camerapos( minpos.x, minpos.y - 1000, -middlez );
	    Coord3 pointat( minpos.x, minpos.y, -middlez );
	    camera_->setPosition( camerapos );
	    camera_->pointAt( pointat, Coord3( 0, 1, 1) );
	    camera_->setFarDistance( 1e30 );
	}
    }

    int sceneidx = displayroot_->getFirstIdx( scene_ );
    if ( sceneidx >= 0 )
	displayroot_->removeObject( sceneidx );

    displayroot_->addObject( newscene );
    scene_ = newscene;

    SoNode* root = displayroot_->getInventorNode();
    if ( !root ) { pErrMsg("huh"); return; }

    setSceneGraph( root );
}


i_LayoutItem* uiSoViewerBody::mkLayoutItem_( i_LayoutMngr& mngr )
{ 
    return new i_uiSoviewLayoutItem( mngr, *this );
}


void uiSoViewerBody::align()
{
    SoCamera* cam = getCamera();
    if ( !cam ) return;
    SbVec3f dir;
    cam->orientation.getValue().multVec( SbVec3f(0,0,-1), dir );

    SbVec3f focalpoint = cam->position.getValue() +
					cam->focalDistance.getValue() * dir;
    SbVec3f upvector( dir[0], dir[1], 1 );
    cam->pointAt( focalpoint, upvector );
}


void uiSoViewerBody::setCameraPos( const SbVec3f& updir, 
				   const SbVec3f& focusdir,
				   bool usetruedir )
{
    SoCamera* cam = getCamera();
    if ( !cam ) return;

    SbVec3f dir;
    cam->orientation.getValue().multVec( SbVec3f(0,0,-1), dir );

#define mEps 1e-6
    const float angletofocus = Math::ACos( dir.dot(focusdir) );
    if ( mIsZero(angletofocus,mEps) ) return;

    const float diffangle = fabs(angletofocus) - M_PI_2;
    const int sign = usetruedir || mIsZero(diffangle,mEps) || diffangle < 0 ? 1 : -1;
    SbVec3f focalpoint = cam->position.getValue() +
			 cam->focalDistance.getValue() * sign * focusdir;
    cam->pointAt( focalpoint, updir );

    cam->position = focalpoint + cam->focalDistance.getValue() * sign *focusdir;
    viewAll();
}


void uiSoViewerBody::viewPlaneX()
{
    setCameraPos( SbVec3f(0,0,1), SbVec3f(1,0,0), false );
}


void uiSoViewerBody::viewPlaneY()
{
    setCameraPos( SbVec3f(0,0,1), SbVec3f(0,1,0), false );
}


void uiSoViewerBody::viewPlaneN()
{
    setCameraPos( SbVec3f(0,0,1), SbVec3f(0,1,0), true );
}


void uiSoViewerBody::viewPlaneZ()
{
    SoCamera* cam = getCamera();
    if ( !cam ) return;

    SbVec3f curdir;
    cam->orientation.getValue().multVec( SbVec3f(0,0,-1), curdir );
    setCameraPos( curdir, SbVec3f(0,0,-1), true );
}


static void getInlCrlVec( SbVec3f& vec, bool inl )
{
    const RCol2Coord& b2c = SI().binID2Coord();
    const RCol2Coord::RCTransform& xtr = b2c.getTransform(true);
    const RCol2Coord::RCTransform& ytr = b2c.getTransform(false);
    const float det = xtr.det( ytr );
    if ( inl )
	vec = SbVec3f( ytr.c/det, -xtr.c/det, 0 );
    else
	vec = SbVec3f( -ytr.b/det, xtr.b/det, 0 );
    vec.normalize();
}


void uiSoViewerBody::viewPlaneInl()
{
    SbVec3f inlvec;
    getInlCrlVec( inlvec, true );
    setCameraPos( SbVec3f(0,0,1), inlvec, false );
}


void uiSoViewerBody::viewPlaneCrl()
{
    SbVec3f crlvec;
    getInlCrlVec( crlvec, false );
    setCameraPos( SbVec3f(0,0,1), crlvec, false );
}


void uiSoViewerBody::anyWheelStart()
{
    uisetFocus();
    interactiveCountInc();
}


void uiSoViewerBody::anyWheelStop()
{
    interactiveCountDec();
    if ( !getInteractiveCount() )
	camerainfo_->setInteractive(false); //Will trigger redraw
}


void uiSoViewerBody::uiRotateHor( float angle )
{ 
    // SoQtExaminerViewer::bottomWheelMotion
    if ( isAnimating() ) stopAnimating();

    // compute new camera position. 
    // See SoQtExaminerViewer::rotYWheelMotion
    SoCamera* cam = getCamera();
    if ( !cam ) return; // can happen for empty scenegraph

    SbVec3f dir;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), dir);

    SbVec3f focalpoint = cam->position.getValue() +
	cam->focalDistance.getValue() * dir;

    cam->orientation = SbRotation(SbVec3f(0, 1, 0), angle ) *
	cam->orientation.getValue();

    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), dir);
    cam->position = focalpoint - cam->focalDistance.getValue() * dir;
}


void uiSoViewerBody::uiRotateVer( float angle )
{ 
    // SoQtExaminerViewer::bottomWheelMotion
    if (  isAnimating() )	stopAnimating();

    // compute new camera position. 
    // See SoQtExaminerViewer::rotXWheelMotion
    SoCamera* cam = getCamera();
    if ( !cam ) return; // can happen for empty scenegraph

    SbVec3f dir;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), dir);

    SbVec3f focalpoint = cam->position.getValue() +
	cam->focalDistance.getValue() * dir;

    cam->orientation = SbRotation(SbVec3f(-1, 0, 0), angle) *
	cam->orientation.getValue();

    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), dir);
    cam->position = focalpoint - cam->focalDistance.getValue() * dir;
}


void uiSoViewerBody::uiZoom( float rel, const SbVec3f* extdir )
{ 

    SoCamera* cam = getCamera();
    if ( !cam) return; // can happen for empty scenegraph

    SoType t = cam->getTypeId();

    // This will be in the range of <0, ->>.
    float multiplicator = exp(rel);

    if (t.isDerivedFrom(SoOrthographicCamera::getClassTypeId())) 
    {
	SoOrthographicCamera * oc = (SoOrthographicCamera *)cam;
	oc->height = oc->height.getValue() * multiplicator;
    } 
    else if (t.isDerivedFrom(SoPerspectiveCamera::getClassTypeId())) 

//NOTE: visBase::Camera has a SoPerspectiveCamera* 
    {
	const float oldfocaldist = cam->focalDistance.getValue();
	const float newfocaldist = oldfocaldist * multiplicator;

	float movement = newfocaldist - oldfocaldist;
	const float minmovement = cam->getViewVolume().getDepth()*0.01;

	if ( fabs(movement)<minmovement )
	{
	    if ( movement<0 ) //zoom in
		return;

	    movement = minmovement;
	}

	SbVec3f olddir;
	cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), olddir);
	const SbVec3f& newdir = extdir ? *extdir : olddir;

	cam->position = cam->position.getValue() - movement * newdir;
	cam->focalDistance = oldfocaldist + movement * olddir.dot( newdir );
    } 
    else 
    {
	pErrMsg("huh!"); //assert(0 && "impossible");
    }
}


float uiSoViewerBody::getCameraZoom()
{
//  Only implemented for SoPerspectiveCamera
    SoCamera* cam = getCamera();
    if ( !cam ) return 0;
    return ((SoPerspectiveCamera *)cam)->
	heightAngle.getValue() / 2 * 360 / M_PI;
}


bool uiSoViewerBody::isCameraPerspective() const
{
    const SoType t = getCameraType();
    return t.isDerivedFrom( SoPerspectiveCamera::getClassTypeId() );
}


bool uiSoViewerBody::isCameraOrthographic() const
{
    const SoType t = getCameraType();
    return t.isDerivedFrom( SoOrthographicCamera::getClassTypeId() ); 
}


void uiSoViewerBody::setCameraZoom( float val )
{
//  Only implemented for SoPerspectiveCamera
    SoCamera* cam = getCamera();
    if ( !cam ) return;
    ((SoPerspectiveCamera *)cam)->heightAngle = val * 2 * M_PI / 360;
}


void uiSoViewerBody::errorCB( const SoError* error, void* data )
{
     if ( GetEnvVarYN("DTECT_COIN_SHOW_ERRORS") || 
	  GetEnvVarYN("COIN_DEBUG_DL") )
     {
	 const SbString& str = error->getDebugString();
	     programmerErrMsg( str.getString(), "uiSoViewerBody::errorCB" ,__FILE__,__LINE__);
     }
}


void uiSoViewerBody::actualRedraw()
{
    const SbColor col = getBackgroundColor();
    glClearColor( col[0], col[1], col[2], 0.0f );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool ismoving = false;
    bool isinteractive = false;
    
    if ( isAnimating() )
	ismoving = true;
    else if ( getInteractiveCount()>0 )
	isinteractive = true;

    camerainfo_->setInteractive( isinteractive );
    camerainfo_->setMoving( ismoving );

    SoQtExaminerViewer::actualRedraw();

    if ( showaxes_ )
    {
	axescamera_->orientation.setValue(
		getCamera()->orientation.getValue() );

	axeslight_->direction.setValue( getHeadlight()->direction.getValue() );

	SoGLRenderAction* glra = getGLRenderAction();
	if ( glra )
	{
	    glClear(GL_DEPTH_BUFFER_BIT);
	    glra->apply( owngraph_ ); 
	}
    }
}


void uiSoViewerBody::mkAxes()
{

    owngraph_ = new SoSeparator;
    owngraph_->ref();

    axeslight_ = new SoDirectionalLight;
    owngraph_->addChild( axeslight_ );
    axeslight_->intensity.setValue( 1.0 );
	
    axescamera_ = new SoOrthographicCamera;
    owngraph_->addChild( axescamera_ );
    axescamera_->position = SbVec3f(0,0,0);
    axescamera_->height = 10;
    axescamera_->nearDistance = -10;
    axescamera_->farDistance = 10;
    
    ViewportRegion* region = new ViewportRegion;
    region->origin.setValue( SbVec2f(1,0) );
    region->size.setValue( SbVec2f(0.15,0.15) );
    owngraph_->addChild( region );

    axes_ = new SoAxes;
    owngraph_->addChild( axes_ );
}


SoNode* uiSoViewerBody::getTotalSceneGraph() 
{
    if ( showaxes_ )
    {
	SoSeparator* sep = new SoSeparator;
	sep->addChild( getSceneManager()->getSceneGraph() );
	sep->addChild( owngraph_ );
	return sep;
    }

    return getSceneManager()->getSceneGraph();
}


//------------------------------------------------------------------------------


i_uiSoviewLayoutItem::i_uiSoviewLayoutItem( i_LayoutMngr& mgr,
                                            uiSoViewerBody& obj)
    : i_uiLayoutItem(mgr,obj)
    , vwr(obj)
{
}


void i_uiSoviewLayoutItem::commitGeometrySet( bool isprefsz )
{
    const uiRect& geom = curpos( setGeom );
    vwr.setSize( SbVec2s(geom.hNrPics(),geom.vNrPics()) );
    i_uiLayoutItem::commitGeometrySet( isprefsz );
}


//------------------------------------------------------------------------------


bool SoTabletEventFilter::eventFilter( QObject* obj, QEvent* ev )
{
    // Temporarily disabled pressure sensitive polygon selection because of
    // its non-smooth behaviour (on Windows)
/*
    QTabletEvent* qte = dynamic_cast<QTabletEvent*>( ev );
    if ( qte )
	SoPolygonSelect::setTabletPressure( qte->pressure() );
*/

    return false;			// Qt will resent it as a QMouseEvent
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

    setViewing( false );  // switches between view & interact mode
}


ui3DViewer::~ui3DViewer()
{
    delete sobody_;
}


uiObjectBody& ui3DViewer::mkBody( uiParent* parnt, bool direct, const char* nm )
{
    sobody_ = new uiSoViewerBody( *this, parnt, nm );
    return *sobody_; 
}

void ui3DViewer::viewAll()
{
    mDynamicCastGet(visSurvey::Scene*,survscene,getScene());
    if ( !survscene )
    {
	if ( sobody_ ) sobody_->viewAll();
    }
    else
    {
	bool showtext = survscene->isAnnotTextShown();
	bool showscale = survscene->isAnnotScaleShown();
	if ( !showtext && !showscale )
	{
	    if ( sobody_ ) sobody_->viewAll();
	}
	else
	{
	    survscene->showAnnotText( false );
	    survscene->showAnnotScale( false );
	    if ( sobody_ ) sobody_->viewAll();
	    survscene->showAnnotText( showtext );
	    survscene->showAnnotScale( showscale );
	}
    }
}


void ui3DViewer::setBackgroundColor( const Color& col )
{
    if ( sobody_ )
	sobody_->setBackgroundColor( SbColor(col2f(r),col2f(g),col2f(b)) );
}


void ui3DViewer::setAxisAnnotColor( const Color& col )
{
    if ( sobody_ ) sobody_->setAxisAnnotColor( col );
}


Color ui3DViewer::getBackgroundColor() const
{
    float r, g, b;
    sobody_->getBackgroundColor().getValue( r, g, b );
    return Color( mNINT32(r*255), mNINT32(g*255), mNINT32(b*255) );
}


void ui3DViewer::getAllKeyBindings( BufferStringSet& keys )
{
    if ( sobody_ )
	sobody_->keyBindMan().getAllKeyBindings( keys );
}


void ui3DViewer::setKeyBindings( const char* keybindname )
{
    if ( sobody_ )
	sobody_->keyBindMan().setKeyBindings( keybindname, true );
}


const char* ui3DViewer::getCurrentKeyBindings() const
{
    if ( sobody_ )
	return sobody_->keyBindMan().getCurrentKeyBindings();

    return 0;
}


void ui3DViewer::viewPlane( PlaneType type )
{
    if ( sobody_ )
    {
	switch ( type )
	{
	    case X: sobody_->viewPlaneX(); break;
	    case Y: sobody_->viewPlaneN(); break;
	    case Z: sobody_->viewPlaneZ(); break;
	    case Inl: sobody_->viewPlaneInl(); break;
	    case Crl: sobody_->viewPlaneCrl(); break;
	    case YZ: sobody_->viewPlaneN(); sobody_->viewPlaneZ(); break;
	}
    }

    viewAll();
}


bool ui3DViewer::isCameraPerspective() const
{
    return sobody_->isCameraPerspective();
}


bool ui3DViewer::setStereoType( StereoType type )
{
    if ( sobody_ )
    {
	SoQtViewer::StereoType soqttype;
	if ( type == RedCyan )
	    soqttype = SoQtViewer::STEREO_ANAGLYPH;
	else if ( type == QuadBuffer )
	    soqttype = SoQtViewer::STEREO_QUADBUFFER;
	else
	    soqttype = SoQtViewer::STEREO_NONE;
	    
	return sobody_->setStereoType( soqttype );
    }
   
    pErrMsg( "Not impl yet" ); 
    return false;
}


ui3DViewer::StereoType ui3DViewer::getStereoType() const
{
    if ( sobody_ )
    {
	SoQtViewer::StereoType soqttype = sobody_->getStereoType();
	if ( soqttype == SoQtViewer::STEREO_ANAGLYPH )
	    return RedCyan;
	else if ( soqttype == SoQtViewer::STEREO_QUADBUFFER )
	    return QuadBuffer;
	else
	    return None;
    }

    return None;
} 


void ui3DViewer::setStereoOffset( float offset )
{
    if ( sobody_ ) sobody_->setStereoOffset( offset );
    else
    {
	pErrMsg( "Not impl yet" ); 
    }
}

float ui3DViewer::getStereoOffset() const
{
    if ( sobody_ )
	return sobody_->getStereoOffset();

    pErrMsg( "Not impl yet" ); 
    return 0;
}

void ui3DViewer::setSceneID( int sceneid )
{
    sobody_->setSceneID( sceneid );
}

int ui3DViewer::sceneID() const
{
    return sobody_->getScene() ? sobody_->getScene()->id() : -1;
}

void ui3DViewer::setViewing( bool yn )
{
    sobody_->uisetViewing(yn);
}

bool ui3DViewer::isViewing() const
{
    return sobody_->isViewing();
}

void ui3DViewer::rotateH( float angle )
    { sobody_->uiRotateHor(angle); }

void ui3DViewer::rotateV( float angle )
    { sobody_->uiRotateVer(angle); }

void ui3DViewer::dolly( float rel )
    { sobody_->uiZoom(rel, 0); }

float ui3DViewer::getCameraZoom()
{
    if ( sobody_ )
	return sobody_->getCameraZoom();

    return 1;
}

const Coord3 ui3DViewer::getCameraPosition() const
{
    SoCamera* cam = sobody_->getCamera();
    if ( !cam )
	return Coord3::udf();

    float x, y, z;
    cam->position.getValue().getValue( x, y, z );
    return Coord3( x, y, z );
}


void ui3DViewer::setCameraZoom( float val )
{
    sobody_->setCameraZoom( val );
}

void ui3DViewer::anyWheelStart()
    { sobody_->anyWheelStart(); }

void ui3DViewer::anyWheelStop()
    { sobody_->anyWheelStop(); }

void ui3DViewer::align()
{
    sobody_->align();
}


void ui3DViewer::toHomePos()
{
    RefMan<visBase::Camera> camera = sobody_->getVisCamera();

    if ( !camera ) return;

    camera->usePar( homepos_ );
}


void ui3DViewer::saveHomePos()
{
    RefMan<visBase::Camera> camera = sobody_->getVisCamera();

    if ( !camera ) return;

    TypeSet<int> dummy;
    camera->fillPar( homepos_, dummy );
    homepos_.removeWithKey( sKey::Type );
    SI().getPars().mergeComp( homepos_, sKeyHomePos() );
    SI().savePars();
}


void ui3DViewer::showRotAxis( bool yn )
{
    if ( sobody_ )
	sobody_->showAxis( yn );
    else
    {
	pErrMsg("Not impl");
    }
}


bool ui3DViewer::rotAxisShown() const
{
    if ( sobody_ )
	return sobody_->axisShown();

    pErrMsg("Not impl");
    return false;
}

void ui3DViewer::toggleCameraType()
{
    sobody_->toggleCameraType();
}


Geom::Size2D<int> ui3DViewer::getViewportSizePixels() const
{
    SbVec2s size = sobody_->getViewportRegion().getViewportSizePixels();
    return Geom::Size2D<int>( size[0], size[1] );
}


SoNode* ui3DViewer::getSceneGraph() const
{
    return sobody_->getTotalSceneGraph();
}


void ui3DViewer::fillPar( IOPar& par ) const
{
    par.set( sKeySceneID(), getScene()->id() );

    par.set( sKeyBGColor(), (int)getBackgroundColor().rgb() );

    par.set( sKeyStereo(), toString( getStereoType() ) );
    float offset = getStereoOffset();
    par.set( sKeyStereoOff(), offset );

    par.setYN( sKeyPersCamera(), isCameraPerspective() );

    TypeSet<int> dummy;
    RefMan<visBase::Camera> camera = sobody_->getVisCamera();

    camera->fillPar( par, dummy );

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
    bool sethomepos = true;

    if ( homepos )
    {
	homepos_ = *homepos;
	sethomepos = false;
	saveHomePos();
    }

    RefMan<visBase::Camera> camera = sobody_->getVisCamera();

    bool res = camera->usePar( par ) == 1;
    
    PtrMan<IOPar> survhomepospar = SI().pars().subselect( sKeyHomePos() );
    if ( !survhomepospar )
	saveHomePos();

    return res;
}



float ui3DViewer::getHeadOnLightIntensity() const
{
    return ( sobody_ ) ? sobody_->getHeadlight()->intensity.getValue() : -1;
}

void ui3DViewer::setHeadOnLightIntensity( float value )
{
    if ( sobody_ )
	sobody_->getHeadlight()->intensity.setValue( value );
}


visBase::Scene* ui3DViewer::getScene()
{
    if ( sobody_ ) return sobody_->getScene();
    return 0;
}


const visBase::Scene* ui3DViewer::getScene() const
{ return const_cast<ui3DViewer*>(this)->getScene(); }
