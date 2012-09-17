/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: ui3dindirectviewer.cc,v 1.3 2012/02/16 14:02:49 cvskris Exp $";


#include "ui3dindirectviewer.h"

#include <QWidget>
#include <QPainter>
#include <QInputEvent>
#include <QQueue>
#include <QSet>
#include <QMutexLocker>

#include <osgViewer/GraphicsWindow>

//#include <osg/DeleteHandler>
//#include <osgQt/GraphicsWindowQt>
//#include <osgViewer/ViewerBase>

class QtKeyboardMap
{

public:
    QtKeyboardMap()
    {
        mKeyMap[Qt::Key_Escape     ] = osgGA::GUIEventAdapter::KEY_Escape;
        mKeyMap[Qt::Key_Delete   ] = osgGA::GUIEventAdapter::KEY_Delete;
        mKeyMap[Qt::Key_Home       ] = osgGA::GUIEventAdapter::KEY_Home;
        mKeyMap[Qt::Key_Enter      ] = osgGA::GUIEventAdapter::KEY_KP_Enter;
        mKeyMap[Qt::Key_End        ] = osgGA::GUIEventAdapter::KEY_End;
        mKeyMap[Qt::Key_Return     ] = osgGA::GUIEventAdapter::KEY_Return;
        mKeyMap[Qt::Key_PageUp     ] = osgGA::GUIEventAdapter::KEY_Page_Up;
        mKeyMap[Qt::Key_PageDown   ] = osgGA::GUIEventAdapter::KEY_Page_Down;
        mKeyMap[Qt::Key_Left       ] = osgGA::GUIEventAdapter::KEY_Left;
        mKeyMap[Qt::Key_Right      ] = osgGA::GUIEventAdapter::KEY_Right;
        mKeyMap[Qt::Key_Up         ] = osgGA::GUIEventAdapter::KEY_Up;
        mKeyMap[Qt::Key_Down       ] = osgGA::GUIEventAdapter::KEY_Down;
        mKeyMap[Qt::Key_Backspace  ] = osgGA::GUIEventAdapter::KEY_BackSpace;
        mKeyMap[Qt::Key_Tab        ] = osgGA::GUIEventAdapter::KEY_Tab;
        mKeyMap[Qt::Key_Space      ] = osgGA::GUIEventAdapter::KEY_Space;
        mKeyMap[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_Delete;
        mKeyMap[Qt::Key_Alt      ] = osgGA::GUIEventAdapter::KEY_Alt_L;
        mKeyMap[Qt::Key_Shift    ] = osgGA::GUIEventAdapter::KEY_Shift_L;
        mKeyMap[Qt::Key_Control  ] = osgGA::GUIEventAdapter::KEY_Control_L;
        mKeyMap[Qt::Key_Meta     ] = osgGA::GUIEventAdapter::KEY_Meta_L;

        mKeyMap[Qt::Key_F1     ] = osgGA::GUIEventAdapter::KEY_F1;
        mKeyMap[Qt::Key_F2     ] = osgGA::GUIEventAdapter::KEY_F2;
        mKeyMap[Qt::Key_F3     ] = osgGA::GUIEventAdapter::KEY_F3;
        mKeyMap[Qt::Key_F4     ] = osgGA::GUIEventAdapter::KEY_F4;
        mKeyMap[Qt::Key_F5     ] = osgGA::GUIEventAdapter::KEY_F5;
        mKeyMap[Qt::Key_F6     ] = osgGA::GUIEventAdapter::KEY_F6;
        mKeyMap[Qt::Key_F7     ] = osgGA::GUIEventAdapter::KEY_F7;
        mKeyMap[Qt::Key_F8     ] = osgGA::GUIEventAdapter::KEY_F8;
        mKeyMap[Qt::Key_F9     ] = osgGA::GUIEventAdapter::KEY_F9;
        mKeyMap[Qt::Key_F10    ] = osgGA::GUIEventAdapter::KEY_F10;
        mKeyMap[Qt::Key_F11    ] = osgGA::GUIEventAdapter::KEY_F11;
        mKeyMap[Qt::Key_F12    ] = osgGA::GUIEventAdapter::KEY_F12;
        mKeyMap[Qt::Key_F13    ] = osgGA::GUIEventAdapter::KEY_F13;
        mKeyMap[Qt::Key_F14    ] = osgGA::GUIEventAdapter::KEY_F14;
        mKeyMap[Qt::Key_F15    ] = osgGA::GUIEventAdapter::KEY_F15;
        mKeyMap[Qt::Key_F16    ] = osgGA::GUIEventAdapter::KEY_F16;
        mKeyMap[Qt::Key_F17    ] = osgGA::GUIEventAdapter::KEY_F17;
        mKeyMap[Qt::Key_F18    ] = osgGA::GUIEventAdapter::KEY_F18;
        mKeyMap[Qt::Key_F19    ] = osgGA::GUIEventAdapter::KEY_F19;
        mKeyMap[Qt::Key_F20    ] = osgGA::GUIEventAdapter::KEY_F20;

        mKeyMap[Qt::Key_hyphen         ] = '-';
        mKeyMap[Qt::Key_Equal         ] = '=';

        mKeyMap[Qt::Key_division] = osgGA::GUIEventAdapter::KEY_KP_Divide;
        mKeyMap[Qt::Key_multiply] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
        mKeyMap[Qt::Key_Minus   ] = '-';
        mKeyMap[Qt::Key_Plus    ] = '+';
        //mKeyMap[Qt::Key_H     ] = osgGA::GUIEventAdapter::KEY_KP_Home;
        //mKeyMap[Qt::Key_      ] = osgGA::GUIEventAdapter::KEY_KP_Up;
        //mKeyMap[92            ] = osgGA::GUIEventAdapter::KEY_KP_Page_Up;
        //mKeyMap[86            ] = osgGA::GUIEventAdapter::KEY_KP_Left;
        //mKeyMap[87            ] = osgGA::GUIEventAdapter::KEY_KP_Begin;
        //mKeyMap[88            ] = osgGA::GUIEventAdapter::KEY_KP_Right;
        //mKeyMap[83            ] = osgGA::GUIEventAdapter::KEY_KP_End;
        //mKeyMap[84            ] = osgGA::GUIEventAdapter::KEY_KP_Down;
        //mKeyMap[85            ] = osgGA::GUIEventAdapter::KEY_KP_Page_Down;
        mKeyMap[Qt::Key_Insert  ] = osgGA::GUIEventAdapter::KEY_KP_Insert;
        //mKeyMap[Qt::Key_Delete] = osgGA::GUIEventAdapter::KEY_KP_Delete;
    }

    ~QtKeyboardMap()
    {
    }

    int remapKey(QKeyEvent* event)
    {
        KeyMap::iterator itr = mKeyMap.find(event->key());
        if (itr == mKeyMap.end())
        {
            return int(*(event->text().toAscii().data()));
        }
        else
            return itr->second;
    }

    private:
    typedef std::map<unsigned int, int> KeyMap;
    KeyMap mKeyMap;
};

static QtKeyboardMap s_QtKeyboardMap;

/*
void GLWidget::glDraw()
{
    _gw->requestRedraw();
}

void GraphicsWindowQt::useCursor( bool cursorOn )
{
    if ( qwidget_ )
    {
        _traits->useCursor = cursorOn;
        if ( !cursorOn ) qwidget_->setCursor( Qt::BlankCursor );
        else qwidget_->setCursor( _currentCursor );
    }
}

void GraphicsWindowQt::setCursor( MouseCursor cursor )
{
    if ( cursor==InheritCursor && qwidget_ )
    {
        qwidget_->unsetCursor();
    }

    switch ( cursor )
    {
    case NoCursor: _currentCursor = Qt::BlankCursor; break;
    case RightArrowCursor: case LeftArrowCursor: _currentCursor = Qt::ArrowCursor; break;
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
    case TopSideCursor: case BottomSideCursor: _currentCursor = Qt::UpArrowCursor; break;
    case LeftSideCursor: case RightSideCursor: _currentCursor = Qt::SizeHorCursor; break;
    case TopLeftCorner: _currentCursor = Qt::SizeBDiagCursor; break;
    case TopRightCorner: _currentCursor = Qt::SizeFDiagCursor; break;
    case BottomRightCorner: _currentCursor = Qt::SizeBDiagCursor; break;
    case BottomLeftCorner: _currentCursor = Qt::SizeFDiagCursor; break;
    default: break;
    };
    if ( qwidget_ ) qwidget_->setCursor( _currentCursor );
}

*/

class GraphicsWindowIndirect;

template <class T>
class OsgIndirectViewWidget : public T
{
    typedef 	QWidget inherited;
public:
    		OsgIndirectViewWidget(T* parent)
		    : QWidget( parent )
		    , forwardKeyEvents_( false )
		{}

		~OsgIndirectViewWidget();
    void	setGraphicsWindow(GraphicsWindowIndirect* w) { gw_=w; }

    int		getNumDeferredEvents();
    void	enqueueDeferredEvent(QEvent::Type eventType,
	    		QEvent::Type removeEventType = QEvent::None);
    void	processDeferredEvents();

    void	setKeyboardModifiers( QInputEvent* );
    void	resizeEvent( QResizeEvent* );
    void	moveEvent( QMoveEvent* );
    void	keyPressEvent( QKeyEvent* );
    void	keyReleaseEvent( QKeyEvent* );
    void	mousePressEvent( QMouseEvent* );
    void	mouseReleaseEvent( QMouseEvent* );
    void	mouseDoubleClickEvent( QMouseEvent* );
    void	mouseMoveEvent( QMouseEvent* );
    void	wheelEvent( QWheelEvent* );
    void	paintEvent(QPaintEvent *);


protected:
    bool			forwardKeyEvents_;
    GraphicsWindowIndirect*	gw_;
    QMutex			deferredEventQueueMutex_;
    QQueue<QEvent::Type>	deferredEventQueue_;
    QSet<QEvent::Type>		eventCompressor_;
};


class GraphicsWindowIndirect : public osgViewer::GraphicsWindow
{
public:
	    GraphicsWindowIndirect( OsgIndirectViewWidget<QWidget>* widget );
	    ~GraphicsWindowIndirect();

    bool	init();
    void	getWindowRectangle(int&,int&,int&,int&);

    void	grabFocus();
    void	grabFocusIfPointerInWindow();
    void	raiseWindow();
    bool	valid() const;

    void	runOperations();

//    void	bindPBufferToTextureImplementation(GLenum)	{}
    void	closeImplementation();
    bool	isRealizedImplementation() const	{ return realized_; }
    bool	makeCurrentImplementation();
    bool	realizeImplementation();
    bool	releaseContextImplementation();
    void	swapBuffersImplementation();

    void	requestWarpPointer( float x, float y );

    QWidget*	getWidget() { return qwidget_; }
    void	removeWidget() { qwidget_ = 0; }

protected:

    OsgIndirectViewWidget<QWidget>*	qwidget_;
    bool				realized_;
};

template <class T> inline
OsgIndirectViewWidget<T>::~OsgIndirectViewWidget()
{
    // close GraphicsWindowQt and remove the reference to us
    if ( gw_ )
    {
        gw_->close();
	gw_->removeWidget();
        gw_ = 0;
    }
}


template <class T> inline
int OsgIndirectViewWidget<T>::getNumDeferredEvents()
{
    QMutexLocker lock( &deferredEventQueueMutex_ );
    return deferredEventQueue_.count();
}


template <class T> inline
void OsgIndirectViewWidget<T>::enqueueDeferredEvent(QEvent::Type eventType,
					       QEvent::Type removeEventType )
{
    QMutexLocker lock( &deferredEventQueueMutex_ );

    if ( removeEventType != QEvent::None)
    {
	if ( deferredEventQueue_.removeOne(removeEventType) )
	    eventCompressor_.remove(eventType);
    }

    if ( eventCompressor_.find(eventType) == eventCompressor_.end())
    {
	deferredEventQueue_.enqueue(eventType);
	eventCompressor_.insert(eventType);
    }
}



template <class T> inline
void OsgIndirectViewWidget<T>::processDeferredEvents()
{
    QQueue<QEvent::Type> deferredEventQueueCopy;
    {
        QMutexLocker lock(&deferredEventQueueMutex_);
        deferredEventQueueCopy = deferredEventQueue_;
        eventCompressor_.clear();
        deferredEventQueue_.clear();
    }

    while (!deferredEventQueueCopy.isEmpty())
    {
        QEvent event(deferredEventQueueCopy.dequeue());
        QWidget::event(&event);
    }
}


template <class T> inline
void OsgIndirectViewWidget<T>::setKeyboardModifiers( QInputEvent* event )
{
    int modkey = event->modifiers() &
	(Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
    unsigned int mask = 0;
    if ( modkey & Qt::ShiftModifier )
	mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if ( modkey & Qt::ControlModifier )
	mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if ( modkey & Qt::AltModifier )
	mask |= osgGA::GUIEventAdapter::MODKEY_ALT;
    gw_->getEventQueue()->getCurrentEventState()->setModKeyMask( mask );
}


template <class T> inline
void OsgIndirectViewWidget<T>::resizeEvent( QResizeEvent* event )
{
    const QSize& size = event->size();
    gw_->resized( this->x(), this->y(), size.width(), size.height() );
    gw_->getEventQueue()->windowResize( this->x(), this->y(), size.width(), size.height() );
    gw_->requestRedraw();
}


template <class T> inline
void OsgIndirectViewWidget<T>::moveEvent( QMoveEvent* event )
{
    const QPoint& pos = event->pos();
    gw_->resized( pos.x(), pos.y(), this->width(), this->height() );
    gw_->getEventQueue()->windowResize( pos.x(), pos.y(), this->width(), this->height() );
}


template <class T> inline
void OsgIndirectViewWidget<T>::keyPressEvent( QKeyEvent* event )
{
    setKeyboardModifiers( event );
    int value = s_QtKeyboardMap.remapKey( event );
    gw_->getEventQueue()->keyPress( value );

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event
    // to the parent widgets
    if( forwardKeyEvents_ )
        inherited::keyPressEvent( event );
}


template <class T> inline
void OsgIndirectViewWidget<T>::keyReleaseEvent( QKeyEvent* event )
{
    setKeyboardModifiers( event );
    gw_->getEventQueue()->keyRelease(
	(osgGA::GUIEventAdapter::KeySymbol) *(event->text().toAscii().data()) );

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to
    // the parent widgets
    if( forwardKeyEvents_ )
        inherited::keyReleaseEvent( event );
}


template <class T> inline
void OsgIndirectViewWidget<T>::mousePressEvent( QMouseEvent* event )
{
    int button = 0;
    switch ( event->button() )
    {
        case Qt::LeftButton: button = 1; break;
        case Qt::MidButton: button = 2; break;
        case Qt::RightButton: button = 3; break;
        case Qt::NoButton: button = 0; break;
        default: button = 0; break;
    }
    setKeyboardModifiers( event );
    gw_->getEventQueue()->mouseButtonPress( event->x(), event->y(), button );
}


template <class T> inline
void OsgIndirectViewWidget<T>::mouseReleaseEvent( QMouseEvent* event )
{
    int button = 0;
    switch ( event->button() )
    {
        case Qt::LeftButton: button = 1; break;
        case Qt::MidButton: button = 2; break;
        case Qt::RightButton: button = 3; break;
        case Qt::NoButton: button = 0; break;
        default: button = 0; break;
    }
    setKeyboardModifiers( event );
    gw_->getEventQueue()->mouseButtonRelease( event->x(), event->y(), button );
}


template <class T> inline
void OsgIndirectViewWidget<T>::mouseDoubleClickEvent( QMouseEvent* event )
{
    int button = 0;
    switch ( event->button() )
    {
        case Qt::LeftButton: button = 1; break;
        case Qt::MidButton: button = 2; break;
        case Qt::RightButton: button = 3; break;
        case Qt::NoButton: button = 0; break;
        default: button = 0; break;
    }
    setKeyboardModifiers( event );
    gw_->getEventQueue()->mouseDoubleButtonPress(
	    event->x(), event->y(), button );
}


template <class T> inline
void OsgIndirectViewWidget<T>::mouseMoveEvent( QMouseEvent* event )
{
    setKeyboardModifiers( event );
    gw_->getEventQueue()->mouseMotion( event->x(), event->y() );
}


template <class T> inline
void OsgIndirectViewWidget<T>::wheelEvent( QWheelEvent* event )
{
    setKeyboardModifiers( event );
    gw_->getEventQueue()->mouseScroll( event->orientation() == Qt::Vertical
	    ?  (event->delta()>0
		? osgGA::GUIEventAdapter::SCROLL_UP
		: osgGA::GUIEventAdapter::SCROLL_DOWN)
	    : (event->delta()>0
		? osgGA::GUIEventAdapter::SCROLL_LEFT
		: osgGA::GUIEventAdapter::SCROLL_RIGHT)
	    );
}


template <class T> inline
void OsgIndirectViewWidget<T>::paintEvent(QPaintEvent *)
{
    //Get Image into pixmap
    QPixmap pixmap;
    QPainter painter(this);
    painter.drawPixmap( 0, 0, pixmap );
}

GraphicsWindowIndirect::GraphicsWindowIndirect(
	OsgIndirectViewWidget<QWidget>* widget )
    : qwidget_( widget )
    , realized_( false )
{
    init();
}

		
GraphicsWindowIndirect::~GraphicsWindowIndirect()
{
    close();
    qwidget_->setGraphicsWindow( 0 );
}


bool GraphicsWindowIndirect::init()
{
    // initialize widget properties
    //qwidget_->setAutoBufferSwap( false );
    qwidget_->setMouseTracking( true );
    qwidget_->setFocusPolicy( Qt::WheelFocus );
    qwidget_->setGraphicsWindow( this );

    // initialize State
    setState( new osg::State );
    getState()->setGraphicsContext(this);

    getState()->setContextID( osg::GraphicsContext::createNewContextID() );
    return true;
}


void GraphicsWindowIndirect::runOperations()
{ 
    // While in graphics thread this is last chance to do something useful
    // before graphics thread will execute its operations. 
    if (qwidget_->getNumDeferredEvents() > 0)
        qwidget_->processDeferredEvents();

    /* TODO: No clue how to do this
    if (QGLContext::currentContext() != qwidget_->context())
        qwidget_->makeCurrent();
	*/

    GraphicsWindow::runOperations();
}


void GraphicsWindowIndirect::getWindowRectangle( int& x, int& y,
						 int& width, int& height )
{
    if ( qwidget_ )
    {
	const QRect& geom = qwidget_->geometry();
	x = geom.x();
	y = geom.y();
	width = geom.width();
	height = geom.height();
    }
}


void GraphicsWindowIndirect::grabFocus()
{
    if ( qwidget_ )
        qwidget_->setFocus( Qt::ActiveWindowFocusReason );
}

void GraphicsWindowIndirect::grabFocusIfPointerInWindow()
{
    if ( qwidget_->underMouse() )
        qwidget_->setFocus( Qt::ActiveWindowFocusReason );
}


void GraphicsWindowIndirect::raiseWindow()
{
    if ( qwidget_ )
        qwidget_->raise();
}


bool GraphicsWindowIndirect::valid() const
{
    //TODO check context is created
    return qwidget_; // && qwidget_->isValid();
}


bool GraphicsWindowIndirect::makeCurrentImplementation()
{
    if (qwidget_->getNumDeferredEvents() > 0)
        qwidget_->processDeferredEvents();

    // TODO No idea how to do this
    // qwidget_->makeCurrent();

    return true;
}


bool GraphicsWindowIndirect::releaseContextImplementation()
{
    /* TODO No clue what to do */
    //qwidget_->doneCurrent();
    return true;
}


void GraphicsWindowIndirect::swapBuffersImplementation()
{
    //TODO: What does swap buffers do?
    //qwidget_->swapBuffers();

    // FIXME: the processDeferredEvents should really be executed in a GUI
    // (main) thread context but I couln't find any reliable way to do this.
    // For now, lets hope non of *GUI thread only operations* will
    // be executed in a QGLWidget::event handler. On the other hand, calling
    // GUI only operations in the QGLWidget event handler is an indication of
    // a Qt bug.
    if (qwidget_->getNumDeferredEvents() > 0)
        qwidget_->processDeferredEvents();

    // We need to call makeCurrent here to restore our previously current
    // context which may be changed by the processDeferredEvents function.

    /* TODO: No clue if this is needed
    if (QGLContext::currentContext() != qwidget_->context())
        qwidget_->makeCurrent();
	*/
}


void GraphicsWindowIndirect::requestWarpPointer( float x, float y )
{
    if ( qwidget_ )
        QCursor::setPos( qwidget_->mapToGlobal(QPoint((int)x,(int)y)) );
}


bool GraphicsWindowIndirect::realizeImplementation()
{
    return true;
    /* TODO No clue what to do here.
    // save the current context
    // note: this will save only Qt-based contexts
    const QGLContext *savedContext = QGLContext::currentContext();

    // initialize GL context for the widget
    if ( !valid() )
        qwidget_->glInit();

    // make current
    realized_ = true;
    bool result = makeCurrent();
    realized_ = false;

    // fail if we do not have current context
    if ( !result )
    {
        if ( savedContext )
            const_cast< QGLContext* >( savedContext )->makeCurrent();

        OSG_WARN << "Window realize: Can make context current." << std::endl;
        return false;
    }

    realized_ = true;

    // make this window's context not current
    // note: this must be done as we will probably make the context current
    // from another thread and it is not allowed to have one context current in
    // two threads
    if( !releaseContext() )
        OSG_WARN << "Window realize: Can not release context." << std::endl;

    // restore previous context
    if ( savedContext )
        const_cast< QGLContext* >( savedContext )->makeCurrent();

    return true;
   */
}


void GraphicsWindowIndirect::closeImplementation()
{
    if ( qwidget_ )
        qwidget_->close();
    realized_ = false;
}


ui3DIndirectViewBody::ui3DIndirectViewBody( ui3DViewer& hndl,
	uiParent* parnt )
    : ui3DViewerBody( hndl, parnt )
{
    OsgIndirectViewWidget<QWidget>* widget =
	new OsgIndirectViewWidget<QWidget>( parnt->pbody()->managewidg() );
    graphicswin_ = new GraphicsWindowIndirect( widget );
    graphicswin_->ref();
    setStretch(2,2);
}


ui3DIndirectViewBody::~ui3DIndirectViewBody()
{
    graphicswin_->unref();
}


const QWidget* ui3DIndirectViewBody::qwidget_() const
{ return graphicswin_->getWidget(); }


osgGA::GUIActionAdapter& ui3DIndirectViewBody::getActionAdapter()
{
    return *graphicswin_;
}


osg::GraphicsContext* ui3DIndirectViewBody::getGraphicsContext()
{
    return graphicswin_;
}
