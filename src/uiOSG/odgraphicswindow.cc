/* -*-c++-*- OpenSceneGraph - Copyright (C) 2009 Wang Rui
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.	The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#include "odgraphicswindow.h"
#include "notify.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uiosgutil.h"


#include <osg/DeleteHandler>
#include <osgViewer/ViewerBase>
#include <QInputEvent>
#include <QPointer>
#include <QWindow>

#if (QT_VERSION>=QT_VERSION_CHECK(4, 6, 0))
# define USE_GESTURES
# include <QGestureEvent>
# include <QGesture>
#endif

class QtKeyboardMap
{

public:
    QtKeyboardMap()
    {
	mKeyMap[Qt::Key_Escape] = osgGA::GUIEventAdapter::KEY_Escape;
	mKeyMap[Qt::Key_Delete] = osgGA::GUIEventAdapter::KEY_Delete;
	mKeyMap[Qt::Key_Home] = osgGA::GUIEventAdapter::KEY_Home;
	mKeyMap[Qt::Key_Enter] = osgGA::GUIEventAdapter::KEY_KP_Enter;
	mKeyMap[Qt::Key_End] = osgGA::GUIEventAdapter::KEY_End;
	mKeyMap[Qt::Key_Return] = osgGA::GUIEventAdapter::KEY_Return;
	mKeyMap[Qt::Key_PageUp] = osgGA::GUIEventAdapter::KEY_Page_Up;
	mKeyMap[Qt::Key_PageDown] = osgGA::GUIEventAdapter::KEY_Page_Down;
	mKeyMap[Qt::Key_Left] = osgGA::GUIEventAdapter::KEY_Left;
	mKeyMap[Qt::Key_Right] = osgGA::GUIEventAdapter::KEY_Right;
	mKeyMap[Qt::Key_Up] = osgGA::GUIEventAdapter::KEY_Up;
	mKeyMap[Qt::Key_Down] = osgGA::GUIEventAdapter::KEY_Down;
	mKeyMap[Qt::Key_Backspace] = osgGA::GUIEventAdapter::KEY_BackSpace;
	mKeyMap[Qt::Key_Tab] = osgGA::GUIEventAdapter::KEY_Tab;
	mKeyMap[Qt::Key_Space] = osgGA::GUIEventAdapter::KEY_Space;
	mKeyMap[Qt::Key_Delete] = osgGA::GUIEventAdapter::KEY_Delete;
	mKeyMap[Qt::Key_Alt] = osgGA::GUIEventAdapter::KEY_Alt_L;
	mKeyMap[Qt::Key_Shift] = osgGA::GUIEventAdapter::KEY_Shift_L;
	mKeyMap[Qt::Key_Control] = osgGA::GUIEventAdapter::KEY_Control_L;
	mKeyMap[Qt::Key_Meta] = osgGA::GUIEventAdapter::KEY_Meta_L;

	mKeyMap[Qt::Key_F1] = osgGA::GUIEventAdapter::KEY_F1;
	mKeyMap[Qt::Key_F2] = osgGA::GUIEventAdapter::KEY_F2;
	mKeyMap[Qt::Key_F3] = osgGA::GUIEventAdapter::KEY_F3;
	mKeyMap[Qt::Key_F4] = osgGA::GUIEventAdapter::KEY_F4;
	mKeyMap[Qt::Key_F5] = osgGA::GUIEventAdapter::KEY_F5;
	mKeyMap[Qt::Key_F6] = osgGA::GUIEventAdapter::KEY_F6;
	mKeyMap[Qt::Key_F7] = osgGA::GUIEventAdapter::KEY_F7;
	mKeyMap[Qt::Key_F8] = osgGA::GUIEventAdapter::KEY_F8;
	mKeyMap[Qt::Key_F9] = osgGA::GUIEventAdapter::KEY_F9;
	mKeyMap[Qt::Key_F10] = osgGA::GUIEventAdapter::KEY_F10;
	mKeyMap[Qt::Key_F11] = osgGA::GUIEventAdapter::KEY_F11;
	mKeyMap[Qt::Key_F12] = osgGA::GUIEventAdapter::KEY_F12;
	mKeyMap[Qt::Key_F13] = osgGA::GUIEventAdapter::KEY_F13;
	mKeyMap[Qt::Key_F14] = osgGA::GUIEventAdapter::KEY_F14;
	mKeyMap[Qt::Key_F15] = osgGA::GUIEventAdapter::KEY_F15;
	mKeyMap[Qt::Key_F16] = osgGA::GUIEventAdapter::KEY_F16;
	mKeyMap[Qt::Key_F17] = osgGA::GUIEventAdapter::KEY_F17;
	mKeyMap[Qt::Key_F18] = osgGA::GUIEventAdapter::KEY_F18;
	mKeyMap[Qt::Key_F19] = osgGA::GUIEventAdapter::KEY_F19;
	mKeyMap[Qt::Key_F20] = osgGA::GUIEventAdapter::KEY_F20;

	mKeyMap[Qt::Key_hyphen] = '-';
	mKeyMap[Qt::Key_Equal] = '=';

	mKeyMap[Qt::Key_division] = osgGA::GUIEventAdapter::KEY_KP_Divide;
	mKeyMap[Qt::Key_multiply] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
	mKeyMap[Qt::Key_Minus] = '-';
	mKeyMap[Qt::Key_Plus] = '+';
	//mKeyMap[Qt::Key_H] = osgGA::GUIEventAdapter::KEY_KP_Home;
	//mKeyMap[Qt::Key_] = osgGA::GUIEventAdapter::KEY_KP_Up;
	//mKeyMap[92] = osgGA::GUIEventAdapter::KEY_KP_Page_Up;
	//mKeyMap[86] = osgGA::GUIEventAdapter::KEY_KP_Left;
	//mKeyMap[87] = osgGA::GUIEventAdapter::KEY_KP_Begin;
	//mKeyMap[88] = osgGA::GUIEventAdapter::KEY_KP_Right;
	//mKeyMap[83] = osgGA::GUIEventAdapter::KEY_KP_End;
	//mKeyMap[84] = osgGA::GUIEventAdapter::KEY_KP_Down;
	//mKeyMap[85] = osgGA::GUIEventAdapter::KEY_KP_Page_Down;
	mKeyMap[Qt::Key_Insert] = osgGA::GUIEventAdapter::KEY_KP_Insert;
	//mKeyMap[Qt::Key_Delete] = osgGA::GUIEventAdapter::KEY_KP_Delete;
    }

    ~QtKeyboardMap()
    {
    }

    int remapKey(QKeyEvent* ev )
    {
	KeyMap::iterator itr = mKeyMap.find(ev ->key());
	if (itr == mKeyMap.end())
	{
	    return int(*(ev ->text().toLatin1().data()));
	}
	else
	    return itr->second;
    }

    private:
    typedef std::map<unsigned int, int> KeyMap;
    KeyMap mKeyMap;
};

static QtKeyboardMap s_QtKeyboardMap;


/// The object responsible for the scene re-rendering.
class HeartBeat : public QObject, public CallBacker
{
public:
    int _timerId;
    osg::Timer _lastFrameStartTime;
    osg::observer_ptr< osgViewer::ViewerBase > _viewer;

    virtual ~HeartBeat();

    void init( osgViewer::ViewerBase *viewer );
    void initCallbacks( const NotifierAccess&, const NotifierAccess& );
    void stopTimer();
    void timerEvent( QTimerEvent *ev  );

    static HeartBeat* instance();
private:

    HeartBeat();

    void stopTimerCB(CallBacker*);
    void startTimerCB(CallBacker*);

    static QPointer<HeartBeat> heartBeat;
};

QPointer<HeartBeat> HeartBeat::heartBeat;

#if (QT_VERSION < QT_VERSION_CHECK(5, 2, 0))
    #define GETDEVICEPIXELRATIO() 1.0
#else
    #define GETDEVICEPIXELRATIO() devicePixelRatio()
#endif

ODGLWidget::ODGLWidget( QWidget* prnt, const QGLWidget* sharewidget,
			Qt::WindowFlags f, bool forwardkeyevents )
    : QGLWidget(prnt,sharewidget,f)
    , _gw(nullptr)
    , _touchEventsEnabled(false)
    , _forwardKeyEvents( forwardkeyevents )
{
    setAttribute(Qt::WA_NativeWindow);
    _devicePixelRatio = GETDEVICEPIXELRATIO();
}


ODGLWidget::ODGLWidget( QGLContext* glctxt, QWidget* prnt,
			const QGLWidget* shareWidget, Qt::WindowFlags f,
			bool forwardKeyEvents )
    : QGLWidget(glctxt,prnt,shareWidget,f)
    , _gw(nullptr)
    , _touchEventsEnabled(false)
    , _forwardKeyEvents(forwardKeyEvents)
{
    setAttribute(Qt::WA_NativeWindow);
    _devicePixelRatio = GETDEVICEPIXELRATIO();
}


ODGLWidget::ODGLWidget( const QGLFormat& glformat, QWidget* prnt,
			const QGLWidget* shareWidget, Qt::WindowFlags f,
			bool forwardKeyEvents )
    : QGLWidget(glformat,prnt,shareWidget,f)
    , _gw(nullptr)
    , _touchEventsEnabled(false)
    , _forwardKeyEvents(forwardKeyEvents)
{
    setAttribute(Qt::WA_NativeWindow);
    _devicePixelRatio = GETDEVICEPIXELRATIO();
}


ODGLWidget::~ODGLWidget()
{
    // close ODGraphicsWindow and remove the reference to us
    if( _gw )
    {
	_gw->close();
	_gw->_widget = nullptr;
	_gw = nullptr;
    }
}


void ODGLWidget::setTouchEventsEnabled( bool e )
{
#ifdef USE_GESTURES
    if (e==_touchEventsEnabled)
	return;

    _touchEventsEnabled = e;

    if (_touchEventsEnabled)
    {
	grabGesture(Qt::PinchGesture);
    }
    else
    {
	ungrabGesture(Qt::PinchGesture);
    }
#endif
}


void ODGLWidget::processDeferredEvents()
{
    QQueue<QEvent::Type> deferredEventQueueCopy;
    {
	QMutexLocker lock(&_deferredEventQueueMutex);
	deferredEventQueueCopy = _deferredEventQueue;
	_eventCompressor.clear();
	_deferredEventQueue.clear();
    }

    while (!deferredEventQueueCopy.isEmpty())
    {
	QEvent ev (deferredEventQueueCopy.dequeue());
	QGLWidget::event(&ev );
    }
}


bool ODGLWidget::event( QEvent* ev  )
{
#ifdef USE_GESTURES
    if ( ev ->type()==QEvent::Gesture )
	return gestureEvent(static_cast<QGestureEvent*>(ev ));
#endif

// QEvent::Hide
//
// workaround "Qt-workaround" that does glFinish before hiding the widget
// (the Qt workaround was seen at least in Qt 4.6.3 and 4.7.0)
//
// Qt makes the context current, performs glFinish,
// and releases the context.
// This makes the problem in OSG multithreaded environment as the context
// is active in another thread, thus it can not be made current for the
// purpose of glFinish in this thread.

// QEvent::ParentChange
//
// Reparenting GLWidget may create a new underlying window and a new GL
// context. Qt will then call doneCurrent on the GL context about
// to be deleted. The thread where old GL context was current has no longer
// current context to render to and we cannot make new GL context current
// in this thread.

// We workaround above problems by deferring execution of problematic
// event requests. These events has to be enqueue and executed later in a
// main GUI thread (GUI operations outside the main thread are not allowed)
// just before makeCurrent is called from the right thread. The good place
// for doing that is right after swap in a swapBuffersImplementation.

    if (ev ->type() == QEvent::Hide)
    {
	// enqueue only the last of QEvent::Hide and QEvent::Show
	enqueueDeferredEvent(QEvent::Hide, QEvent::Show);
	return true;
    }
    else if (ev ->type() == QEvent::Show)
    {
	// enqueue only the last of QEvent::Show or QEvent::Hide
	enqueueDeferredEvent(QEvent::Show, QEvent::Hide);
	return true;
    }
    else if (ev ->type() == QEvent::ParentChange)
    {
	// enqueue only the last QEvent::ParentChange
	enqueueDeferredEvent(QEvent::ParentChange);
	return true;
    }

    // perform regular event handling
    return QGLWidget::event( ev  );
}


void ODGLWidget::setKeyboardModifiers( QInputEvent* ev	)
{
    int modkey = ev ->modifiers() &
		 (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
    unsigned int modkeymask = 0;
    if ( modkey & Qt::ShiftModifier )
	modkeymask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if ( modkey & Qt::ControlModifier )
	modkeymask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if ( modkey & Qt::AltModifier )
	modkeymask |= osgGA::GUIEventAdapter::MODKEY_ALT;
    _gw->getEventQueue()->getCurrentEventState()->setModKeyMask( modkeymask );
}


void ODGLWidget::resizeEvent( QResizeEvent* ev	)
{
    if ( _gw == nullptr || !_gw->valid() )
	return;

    const QSize& qsz = ev ->size();
    int scaled_width = static_cast<int>(qsz.width()*_devicePixelRatio);
    int scaled_height = static_cast<int>(qsz.height()*_devicePixelRatio);
    _gw->resized( x(), y(), scaled_width,  scaled_height);
    _gw->getEventQueue()->windowResize( x(), y(), scaled_width, scaled_height );
    _gw->requestRedraw();
}


void ODGLWidget::moveEvent( QMoveEvent* ev  )
{
    if (_gw == nullptr || !_gw->valid())
	return;

    const QPoint& evpos = ev ->pos();
    int scaled_width = static_cast<int>(width()*_devicePixelRatio);
    int scaled_height = static_cast<int>(height()*_devicePixelRatio);
    _gw->resized( evpos.x(), evpos.y(), scaled_width,  scaled_height );
    _gw->getEventQueue()->windowResize( evpos.x(), evpos.y(),
					scaled_width, scaled_height );
}


void ODGLWidget::glDraw()
{
    _gw->requestRedraw();
}


void ODGLWidget::keyPressEvent( QKeyEvent* ev  )
{
    setKeyboardModifiers( ev  );
    int value = s_QtKeyboardMap.remapKey( ev  );
    _gw->getEventQueue()->keyPress( value );

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event
    // to the parent widgets
    if( _forwardKeyEvents )
	inherited::keyPressEvent( ev  );
}


void ODGLWidget::keyReleaseEvent( QKeyEvent* ev  )
{
    if( ev ->isAutoRepeat() )
    {
	ev ->ignore();
    }
    else
    {
	setKeyboardModifiers( ev  );
	int value = s_QtKeyboardMap.remapKey( ev  );
	_gw->getEventQueue()->keyRelease( value );
    }

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event
    // to the parent widgets
    if( _forwardKeyEvents )
	inherited::keyReleaseEvent( ev	);
}


void ODGLWidget::mousePressEvent( QMouseEvent* ev  )
{
    int button = 0;
    switch ( ev ->button() )
    {
	case Qt::LeftButton: button = 1; break;
	case Qt::MidButton: button = 2; break;
	case Qt::RightButton: button = 3; break;
	case Qt::NoButton: button = 0; break;
	default: button = 0; break;
    }
    setKeyboardModifiers( ev  );
    _gw->getEventQueue()->mouseButtonPress(
	ev ->x()*_devicePixelRatio, ev ->y()*_devicePixelRatio, button );
}


void ODGLWidget::mouseReleaseEvent( QMouseEvent* ev  )
{
    int button = 0;
    switch ( ev ->button() )
    {
	case Qt::LeftButton: button = 1; break;
	case Qt::MidButton: button = 2; break;
	case Qt::RightButton: button = 3; break;
	case Qt::NoButton: button = 0; break;
	default: button = 0; break;
    }
    setKeyboardModifiers( ev  );
    _gw->getEventQueue()->mouseButtonRelease(
	ev ->x()*_devicePixelRatio, ev ->y()*_devicePixelRatio, button );
}


void ODGLWidget::mouseDoubleClickEvent( QMouseEvent* ev  )
{
    int button = 0;
    switch ( ev ->button() )
    {
	case Qt::LeftButton: button = 1; break;
	case Qt::MidButton: button = 2; break;
	case Qt::RightButton: button = 3; break;
	case Qt::NoButton: button = 0; break;
	default: button = 0; break;
    }
    setKeyboardModifiers( ev  );
    _gw->getEventQueue()->mouseDoubleButtonPress(
	ev ->x()*_devicePixelRatio, ev ->y()*_devicePixelRatio, button );
}


void ODGLWidget::mouseMoveEvent( QMouseEvent* ev  )
{
    setKeyboardModifiers( ev  );
    _gw->getEventQueue()->mouseMotion(
		ev ->x()*_devicePixelRatio, ev ->y()*_devicePixelRatio );
}


void ODGLWidget::wheelEvent( QWheelEvent* ev  )
{
    setKeyboardModifiers( ev  );
    const QPoint delta = ev->angleDelta();
    const bool isvertical = abs(delta.y()) > abs(delta.x());
    _gw->getEventQueue()->mouseScroll(
	isvertical ? (delta.y()>0 ? osgGA::GUIEventAdapter::SCROLL_UP
				  : osgGA::GUIEventAdapter::SCROLL_DOWN)
		   : (delta.x()>0 ? osgGA::GUIEventAdapter::SCROLL_LEFT
				  : osgGA::GUIEventAdapter::SCROLL_RIGHT) );
}


#ifdef USE_GESTURES
static osgGA::GUIEventAdapter::TouchPhase translateQtGestureState(
						Qt::GestureState state )
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
#endif


bool ODGLWidget::gestureEvent( QGestureEvent* qevent )
{
#ifndef USE_GESTURES
    return false;
#else

    bool accept = false;

    if ( QPinchGesture* pinch = static_cast<QPinchGesture *>(
					qevent->gesture(Qt::PinchGesture) ) )
    {
	const QPointF qcenterf = pinch->centerPoint();
	const float angle = pinch->totalRotationAngle();
	const float scale = pinch->totalScaleFactor();

	const QPoint pinchCenterQt = mapFromGlobal(qcenterf.toPoint());
	const osg::Vec2 pinchCenter( pinchCenterQt.x(), pinchCenterQt.y() );

	//We don't have absolute positions of the two touches, only a scale and
	// rotation. Hence we create pseudo-coordinates which are reasonable,
	// and centered around the real position
	const float radius = float(width()+height())/4.0f;
	const osg::Vec2 vector( scale*cos(angle)*radius,
				scale*sin(angle)*radius);
	const osg::Vec2 p0 = pinchCenter+vector;
	const osg::Vec2 p1 = pinchCenter-vector;

	osg::ref_ptr<osgGA::GUIEventAdapter> ev  = nullptr;
	const osgGA::GUIEventAdapter::TouchPhase touchPhase =
				translateQtGestureState( pinch->state() );
	if ( touchPhase==osgGA::GUIEventAdapter::TOUCH_BEGAN )
	{
	    ev	= _gw->getEventQueue()->touchBegan(
				0, touchPhase, p0[0], p0[1] );
	}
	else if ( touchPhase==osgGA::GUIEventAdapter::TOUCH_MOVED )
	{
	    ev	= _gw->getEventQueue()->touchMoved(
				0, touchPhase, p0[0], p0[1] );
	}
	else
	{
	    ev	= _gw->getEventQueue()->touchEnded(
				0, touchPhase, p0[0], p0[1], 1 );
	}

	if ( ev  )
	{
	    ev ->addTouchPoint( 1, touchPhase, p1[0], p1[1] );
	    accept = true;
	}
    }

    if ( accept )
	qevent->accept();

    return accept;
#endif
}



ODGraphicsWindow::ODGraphicsWindow( osg::GraphicsContext::Traits* traits,
				    QWidget* prnt, const QGLWidget* shareWidget,
				    Qt::WindowFlags f )
    : _realized(false)
{

    _widget = nullptr;
    _traits = traits;
    init( prnt, shareWidget, f );
}


ODGraphicsWindow::ODGraphicsWindow( ODGLWidget* widget )
    : _realized(false)
{
    _widget = widget;
    _traits = _widget ? createTraits( _widget )
		      : new osg::GraphicsContext::Traits;
    init( nullptr, nullptr, Qt::WindowFlags() );
}


ODGraphicsWindow::~ODGraphicsWindow()
{
    close();

    // remove reference from ODGLWidget
    if ( _widget )
	_widget->_gw = nullptr;
}


bool ODGraphicsWindow::init( QWidget* prnt, const QGLWidget* shareWidget,
			     Qt::WindowFlags f )
{
    // update _widget and parent by WindowData
    WindowData* windowData = _traits.get() ?
	dynamic_cast<WindowData*>(_traits->inheritedWindowData.get()) : nullptr;
    if ( !_widget )
	_widget = windowData ? windowData->_widget : nullptr;
    if ( !prnt )
	prnt = windowData ? windowData->_parent : nullptr;

    // create widget if it does not exist
    _ownsWidget = _widget == nullptr;
    if ( !_widget )
    {
	// shareWidget
	if ( !shareWidget ) {
	    ODGraphicsWindow* sharedContextQt =
		dynamic_cast<ODGraphicsWindow*>(_traits->sharedContext.get());
	    if ( sharedContextQt )
		shareWidget = sharedContextQt->getGLWidget();
	}

	// WindowFlags
	Qt::WindowFlags flags = f | Qt::Window | Qt::CustomizeWindowHint;
	if ( _traits->windowDecoration )
	    flags |= Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint
		  | Qt::WindowSystemMenuHint
#if (QT_VERSION_CHECK(4, 5, 0) <= QT_VERSION)
		  | Qt::WindowCloseButtonHint
#endif
		;

	// create widget
	_widget = new ODGLWidget( traits2qglFormat( _traits.get() ), prnt,
				  shareWidget, flags );
    }

    // set widget name and position
    // (do not set it when we inherited the widget)
    if ( _ownsWidget )
    {
	_widget->setWindowTitle( _traits->windowName.c_str() );
	_widget->move( _traits->x, _traits->y );
	if ( !_traits->supportsResize )
	    _widget->setFixedSize( _traits->width, _traits->height );
	else
	    _widget->resize( _traits->width, _traits->height );
    }

    // initialize widget properties
    _widget->setAutoBufferSwap( false );
    _widget->setMouseTracking( true );
    _widget->setFocusPolicy( Qt::WheelFocus );
    _widget->setGraphicsWindow( this );
    useCursor( _traits->useCursor );

    // initialize State
    setState( new osg::State );
    getState()->setGraphicsContext(this);

    // initialize contextID
    if ( _traits.valid() && _traits->sharedContext.valid() )
    {
	getState()->setContextID(
			_traits->sharedContext->getState()->getContextID() );
	incrementContextIDUsageCount( getState()->getContextID() );
    }
    else
    {
	getState()->setContextID( osg::GraphicsContext::createNewContextID() );
    }

    // make sure the event queue has the correct window
    // rectangle size and input range
    getEventQueue()->syncWindowRectangleWithGraphicsContext();

    return true;
}


QGLFormat ODGraphicsWindow::traits2qglFormat(
		const osg::GraphicsContext::Traits* traits )
{
    QGLFormat glformat( QGLFormat::defaultFormat() );

    glformat.setAlphaBufferSize( traits->alpha );
    glformat.setRedBufferSize( traits->red );
    glformat.setGreenBufferSize( traits->green );
    glformat.setBlueBufferSize( traits->blue );
    glformat.setDepthBufferSize( traits->depth );
    glformat.setStencilBufferSize( traits->stencil );
    glformat.setSampleBuffers( traits->sampleBuffers );
    glformat.setSamples( traits->samples );

    glformat.setAlpha( traits->alpha>0 );
    glformat.setDepth( traits->depth>0 );
    glformat.setStencil( traits->stencil>0 );
    glformat.setDoubleBuffer( traits->doubleBuffer );
    glformat.setSwapInterval( traits->vsync ? 1 : 0 );
    glformat.setStereo( traits->quadBufferStereo ? 1 : 0 );

    return glformat;
}


void ODGraphicsWindow::qglFormat2traits( const QGLFormat& glformat,
					 osg::GraphicsContext::Traits* traits )
{
    traits->red = glformat.redBufferSize();
    traits->green = glformat.greenBufferSize();
    traits->blue = glformat.blueBufferSize();
    traits->alpha = glformat.alpha() ? glformat.alphaBufferSize() : 0;
    traits->depth = glformat.depth() ? glformat.depthBufferSize() : 0;
    traits->stencil = glformat.stencil() ? glformat.stencilBufferSize() : 0;

    traits->sampleBuffers = glformat.sampleBuffers() ? 1 : 0;
    traits->samples = glformat.samples();

    traits->quadBufferStereo = glformat.stereo();
    traits->doubleBuffer = glformat.doubleBuffer();

    traits->vsync = glformat.swapInterval() >= 1;
}


osg::GraphicsContext::Traits*
	ODGraphicsWindow::createTraits( const QGLWidget* widget )
{
    osg::GraphicsContext::Traits *traits = new osg::GraphicsContext::Traits;

    qglFormat2traits( widget->format(), traits );

    QRect r = widget->geometry();
    traits->x = r.x();
    traits->y = r.y();
    traits->width = r.width();
    traits->height = r.height();

    traits->windowName = widget->windowTitle().toLocal8Bit().data();
    Qt::WindowFlags f = widget->windowFlags();
    traits->windowDecoration = ( f & Qt::WindowTitleHint ) &&
			    ( f & Qt::WindowMinMaxButtonsHint ) &&
			    ( f & Qt::WindowSystemMenuHint );
    QSizePolicy sp = widget->sizePolicy();
    traits->supportsResize = sp.horizontalPolicy() != QSizePolicy::Fixed ||
			    sp.verticalPolicy() != QSizePolicy::Fixed;

    return traits;
}


bool ODGraphicsWindow::setWindowRectangleImplementation( int x, int y,
							 int width, int height )
{
    if ( _widget == nullptr )
	return false;

    _widget->setGeometry( x, y, width, height );
    return true;
}


void ODGraphicsWindow::getWindowRectangle( int& x, int& y,
					   int& width, int& height )
{
    if ( _widget )
    {
	const QRect& geom = _widget->geometry();
	x = geom.x();
	y = geom.y();
	width = geom.width();
	height = geom.height();
    }
}


bool ODGraphicsWindow::setWindowDecorationImplementation( bool windowDecoration)
{
    Qt::WindowFlags flags = Qt::Window | Qt::CustomizeWindowHint;
//				       | Qt::WindowStaysOnTopHint;
    if ( windowDecoration )
	flags |= Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint
	      | Qt::WindowSystemMenuHint;
    _traits->windowDecoration = windowDecoration;

    if ( _widget )
    {
	_widget->setWindowFlags( flags );
	return true;
    }

    return false;
}


bool ODGraphicsWindow::getWindowDecoration() const
{
    return _traits->windowDecoration;
}


void ODGraphicsWindow::grabFocus()
{
    if ( _widget )
	_widget->setFocus( Qt::ActiveWindowFocusReason );
}


void ODGraphicsWindow::grabFocusIfPointerInWindow()
{
    if ( _widget->underMouse() )
	_widget->setFocus( Qt::ActiveWindowFocusReason );
}

void ODGraphicsWindow::raiseWindow()
{
    if ( _widget )
	_widget->raise();
}


void ODGraphicsWindow::setWindowName( const std::string& name )
{
    if ( _widget )
	_widget->setWindowTitle( name.c_str() );
}


std::string ODGraphicsWindow::getWindowName()
{
    return _widget ? _widget->windowTitle().toStdString() : "";
}


void ODGraphicsWindow::useCursor( bool cursorOn )
{
    if ( _widget )
    {
	_traits->useCursor = cursorOn;
	if ( !cursorOn ) _widget->setCursor( Qt::BlankCursor );
	else _widget->setCursor( _currentCursor );
    }
}


void ODGraphicsWindow::setCursor( MouseCursor cursor )
{
    if ( cursor==InheritCursor && _widget )
    {
	_widget->unsetCursor();
    }

    switch ( cursor )
    {
    case NoCursor: _currentCursor = Qt::BlankCursor; break;
    case RightArrowCursor: case LeftArrowCursor:
				_currentCursor = Qt::ArrowCursor; break;
    case InfoCursor: _currentCursor = Qt::SizeAllCursor; break;
    case DestroyCursor: _currentCursor = Qt::ForbiddenCursor; break;
    case HelpCursor: _currentCursor = Qt::WhatsThisCursor; break;
    case CycleCursor: _currentCursor = Qt::ForbiddenCursor; break;
    case SprayCursor: _currentCursor = Qt::SizeAllCursor; break;
    case WaitCursor: _currentCursor = Qt::WaitCursor; break;
    case TextCursor: _currentCursor = Qt::IBeamCursor; break;
    case CrosshairCursor: _currentCursor = Qt::CrossCursor; break;
    case HandCursor: _currentCursor = Qt::OpenHandCursor; break;
    case UpDownCursor: _currentCursor = Qt::SizeVerCursor; break;
    case LeftRightCursor: _currentCursor = Qt::SizeHorCursor; break;
    case TopSideCursor: case BottomSideCursor:
				_currentCursor = Qt::UpArrowCursor; break;
    case LeftSideCursor: case RightSideCursor:
				_currentCursor = Qt::SizeHorCursor; break;
    case TopLeftCorner: _currentCursor = Qt::SizeBDiagCursor; break;
    case TopRightCorner: _currentCursor = Qt::SizeFDiagCursor; break;
    case BottomRightCorner: _currentCursor = Qt::SizeBDiagCursor; break;
    case BottomLeftCorner: _currentCursor = Qt::SizeFDiagCursor; break;
    default: break;
    };
    if ( _widget ) _widget->setCursor( _currentCursor );
}


bool ODGraphicsWindow::valid() const
{
    return _widget && _widget->isValid();
}


bool ODGraphicsWindow::realizeImplementation()
{
    // save the current context
    // note: this will save only Qt-based contexts
    const QGLContext *savedContext = QGLContext::currentContext();

    // initialize GL context for the widget
    if ( !valid() )
	_widget->glInit();

    // make current
    _realized = true;
    bool result = makeCurrent();
    _realized = false;

    // fail if we do not have current context
    if ( !result )
    {
	if ( savedContext )
	    const_cast< QGLContext* >( savedContext )->makeCurrent();

	OSG_WARN << "Window realize: Can make context current." << std::endl;
	return false;
    }

    _realized = true;

    // make sure the event queue has the correct
    // window rectangle size and input range
    getEventQueue()->syncWindowRectangleWithGraphicsContext();

    // make this window's context not current
    // note: this must be done as we will probably make the context current
    // from another thread
    //	     and it is not allowed to have one context current in two threads
    if( !releaseContext() )
	OSG_WARN << "Window realize: Can not release context." << std::endl;

    // restore previous context
    if ( savedContext )
	const_cast< QGLContext* >( savedContext )->makeCurrent();

    return true;
}


bool ODGraphicsWindow::isRealizedImplementation() const
{
    return _realized;
}


void ODGraphicsWindow::closeImplementation()
{
    if ( _widget )
	_widget->close();
    _realized = false;
}


void ODGraphicsWindow::runOperations()
{
    // While in graphics thread this is last chance to do something useful
    // before graphics thread will execute its operations.
    if (_widget->getNumDeferredEvents() > 0)
	_widget->processDeferredEvents();

    if (QGLContext::currentContext() != _widget->context())
	_widget->makeCurrent();

    GraphicsWindow::runOperations();
}


bool ODGraphicsWindow::makeCurrentImplementation()
{
    if (_widget->getNumDeferredEvents() > 0)
	_widget->processDeferredEvents();

    _widget->makeCurrent();

    return true;
}


bool ODGraphicsWindow::releaseContextImplementation()
{
    _widget->doneCurrent();
    return true;
}


void ODGraphicsWindow::swapBuffersImplementation()
{
    if ( !_widget || !_widget->windowHandle()->isExposed() )
	return;

// FIXME: the processDeferredEvents should really be executed in a
// GUI (main) thread context but I couln't find any reliable way to do this.
// For now, lets hope non of *GUI thread only operations* will be executed
// in a QGLWidget::event handler. On the other hand, calling GUI only operations
// in the QGLWidget event handler is an indication of a Qt bug.
    if (_widget->getNumDeferredEvents() > 0)
	_widget->processDeferredEvents();

// We need to call makeCurrent here to restore our previously current context
// which may be changed by the processDeferredEvents function.
    _widget->makeCurrent();
    _widget->swapBuffers();
}


void ODGraphicsWindow::requestWarpPointer( float x, float y )
{
    if ( _widget )
	QCursor::setPos( _widget->mapToGlobal(QPoint((int)x,(int)y)) );
}


class QtWindowingSystem : public osg::GraphicsContext::WindowingSystemInterface
{
public:

QtWindowingSystem()
{
    OSG_INFO << "QtWindowingSystemInterface()" << std::endl;
}

~QtWindowingSystem()
{
    if ( osg::Referenced::getDeleteHandler() )
    {
	osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
	osg::Referenced::getDeleteHandler()->flushAll();
    }
}


// Access the Qt windowing system through this singleton class.
static QtWindowingSystem* getInterface()
{
    mDefineStaticLocalObject( QtWindowingSystem*, qtInterface,
			      (new QtWindowingSystem) );
    return qtInterface;
}


// Return the number of screens present in the system
virtual unsigned int getNumScreens(
		const osg::GraphicsContext::ScreenIdentifier& /*si*/ )
{
    OSG_WARN << "uiOSG: getNumScreens() not implemented yet." << std::endl;
    return 0;
}


// Return the resolution of specified screen
// (0,0) is returned if screen is unknown
virtual void getScreenSettings( const osg::GraphicsContext::ScreenIdentifier&,
				osg::GraphicsContext::ScreenSettings& )
{
    OSG_WARN << "uiOSG: getScreenSettings() not implemented yet." << std::endl;
}


// Set the resolution for given screen
virtual bool setScreenSettings( const osg::GraphicsContext::ScreenIdentifier&,
				const osg::GraphicsContext::ScreenSettings& )
{
    OSG_WARN << "uiOSG: setScreenSettings() not implemented yet." << std::endl;
    return false;
}


// Enumerates available resolutions
virtual void enumerateScreenSettings(
		const osg::GraphicsContext::ScreenIdentifier&,
		osg::GraphicsContext::ScreenSettingsList& )
{
    OSG_WARN << "uiOSG: enumerateScreenSettings() not implemented yet."
	     << std::endl;
}


// Create a graphics context with given traits
virtual osg::GraphicsContext* createGraphicsContext(
		osg::GraphicsContext::Traits* traits )
{
    if ( traits->pbuffer )
    {
	OSG_WARN << "uiOSG: createGraphicsContext - "
		    "pbuffer not implemented yet." << std::endl;
	return nullptr;
    }
    else
    {
	osg::ref_ptr< ODGraphicsWindow > window =
				    new ODGraphicsWindow( traits );
	if ( window->valid() )
	    return window.release();
	else
	    return nullptr;
    }
}

private:

    // No implementation for these
    QtWindowingSystem( const QtWindowingSystem& );
    QtWindowingSystem& operator=( const QtWindowingSystem& );
};

#if OSG_VERSION_GREATER_OR_EQUAL(3, 5, 6)
REGISTER_WINDOWINGSYSTEMINTERFACE(Qt, QtWindowingSystem)
#else

// declare C entry point for static compilation.
extern "C" void graphicswindow_Qt(void)
{
    osg::GraphicsContext::setWindowingSystemInterface(
		QtWindowingSystem::getInterface() );
}


void initQtWindowingSystem()
{
    graphicswindow_Qt();
}
#endif


void setViewer( osgViewer::ViewerBase *viewer )
{
    HeartBeat::instance()->init( viewer );
}


void setOSGTimerCallbacks( const NotifierAccess& start,
			   const NotifierAccess& stop )
{
    HeartBeat::instance()->initCallbacks( start, stop );
}


/// Constructor. Must be called from main thread.
HeartBeat::HeartBeat()
    : _timerId( 0 )
{
    uiMainWin* uimw = uiMain::instance().topLevel();
    if ( uimw )
	mAttachCB( uimw->windowClosed, HeartBeat::stopTimerCB );
}


/// Destructor. Must be called from main thread.
HeartBeat::~HeartBeat()
{
    detachAllNotifiers();
    stopTimer();
}

HeartBeat* HeartBeat::instance()
{
    if (!heartBeat)
    {
	heartBeat = new HeartBeat();
    }
    return heartBeat;
}


void HeartBeat::stopTimerCB( CallBacker* )
{
    stopTimer();
}


void HeartBeat::startTimerCB( CallBacker* )
{
    if ( _timerId == 0	)
    {
	_timerId = startTimer( 0 );
	_lastFrameStartTime.setStartTick( 0 );
    }
}


void HeartBeat::stopTimer()
{
    if ( _timerId != 0 )
    {
	killTimer( _timerId );
	_timerId = 0;
    }
}


/// Initializes the loop for viewer. Must be called from main thread.
void HeartBeat::init( osgViewer::ViewerBase *viewer )
{
    if( _viewer == viewer )
	return;

    stopTimer();

    _viewer = viewer;

    if( viewer )
    {
	_timerId = startTimer( 0 );
	_lastFrameStartTime.setStartTick( 0 );
    }
}


void HeartBeat::initCallbacks( const NotifierAccess& start,
			       const NotifierAccess& stop )
{
    mAttachCB( start, HeartBeat::startTimerCB );
    mAttachCB( stop, HeartBeat::stopTimerCB );
}


void HeartBeat::timerEvent( QTimerEvent * )
{
    osg::ref_ptr< osgViewer::ViewerBase > viewer;
    if( !_viewer.lock( viewer ) )
    {
	// viewer has been deleted -> stop timer
	stopTimer();
	return;
    }

    // limit the frame rate
    if( viewer->getRunMaxFrameRate() > 0.0)
    {
	double dt = _lastFrameStartTime.time_s();
	double minFrameTime = 1.0 / viewer->getRunMaxFrameRate();
	if (dt < minFrameTime)
	    OpenThreads::Thread::microSleep(
			static_cast<unsigned int>(1000000.0*(minFrameTime-dt)));
    }
    else
    {
	// avoid excessive CPU loading when no frame is required
	// in ON_DEMAND mode
	if( viewer->getRunFrameScheme() == osgViewer::ViewerBase::ON_DEMAND )
	{
	    double dt = _lastFrameStartTime.time_s();
	    if (dt < 0.01)
		OpenThreads::Thread::microSleep(
			static_cast<unsigned int>(1000000.0*(0.01-dt)));
	}

	// record start frame time
	_lastFrameStartTime.setStartTick();

	// make frame
	if( viewer->getRunFrameScheme() == osgViewer::ViewerBase::ON_DEMAND )
	{
	    if( viewer->checkNeedToDoFrame() )
	    {
		viewer->frame();
	    }
	}
	else
	{
	    viewer->frame();
	}
    }
}
