/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/

#include "odopenglwidget.h"
#include "uiosgutil.h"

#include <osgViewer/CompositeViewer>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <QApplication>
#include <QDesktopWidget>
#include <QInputEvent>
#include <QMouseEvent>
#include <QWheelEvent>


class QtKeyboardMap
{
public:
QtKeyboardMap()
{
    keymap_[Qt::Key_Escape     ] = osgGA::GUIEventAdapter::KEY_Escape;
    keymap_[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_Delete;
    keymap_[Qt::Key_Home       ] = osgGA::GUIEventAdapter::KEY_Home;
    keymap_[Qt::Key_Enter      ] = osgGA::GUIEventAdapter::KEY_KP_Enter;
    keymap_[Qt::Key_End        ] = osgGA::GUIEventAdapter::KEY_End;
    keymap_[Qt::Key_Return     ] = osgGA::GUIEventAdapter::KEY_Return;
    keymap_[Qt::Key_PageUp     ] = osgGA::GUIEventAdapter::KEY_Page_Up;
    keymap_[Qt::Key_PageDown   ] = osgGA::GUIEventAdapter::KEY_Page_Down;
    keymap_[Qt::Key_Left       ] = osgGA::GUIEventAdapter::KEY_Left;
    keymap_[Qt::Key_Right      ] = osgGA::GUIEventAdapter::KEY_Right;
    keymap_[Qt::Key_Up         ] = osgGA::GUIEventAdapter::KEY_Up;
    keymap_[Qt::Key_Down       ] = osgGA::GUIEventAdapter::KEY_Down;
    keymap_[Qt::Key_Backspace  ] = osgGA::GUIEventAdapter::KEY_BackSpace;
    keymap_[Qt::Key_Tab        ] = osgGA::GUIEventAdapter::KEY_Tab;
    keymap_[Qt::Key_Space      ] = osgGA::GUIEventAdapter::KEY_Space;
    keymap_[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_Delete;
    keymap_[Qt::Key_Alt        ] = osgGA::GUIEventAdapter::KEY_Alt_L;
    keymap_[Qt::Key_Shift      ] = osgGA::GUIEventAdapter::KEY_Shift_L;
    keymap_[Qt::Key_Control    ] = osgGA::GUIEventAdapter::KEY_Control_L;
    keymap_[Qt::Key_Meta       ] = osgGA::GUIEventAdapter::KEY_Meta_L;
    keymap_[Qt::Key_F1         ] = osgGA::GUIEventAdapter::KEY_F1;
    keymap_[Qt::Key_F2         ] = osgGA::GUIEventAdapter::KEY_F2;
    keymap_[Qt::Key_F3         ] = osgGA::GUIEventAdapter::KEY_F3;
    keymap_[Qt::Key_F4         ] = osgGA::GUIEventAdapter::KEY_F4;
    keymap_[Qt::Key_F5         ] = osgGA::GUIEventAdapter::KEY_F5;
    keymap_[Qt::Key_F6         ] = osgGA::GUIEventAdapter::KEY_F6;
    keymap_[Qt::Key_F7         ] = osgGA::GUIEventAdapter::KEY_F7;
    keymap_[Qt::Key_F8         ] = osgGA::GUIEventAdapter::KEY_F8;
    keymap_[Qt::Key_F9         ] = osgGA::GUIEventAdapter::KEY_F9;
    keymap_[Qt::Key_F10        ] = osgGA::GUIEventAdapter::KEY_F10;
    keymap_[Qt::Key_F11        ] = osgGA::GUIEventAdapter::KEY_F11;
    keymap_[Qt::Key_F12        ] = osgGA::GUIEventAdapter::KEY_F12;
    keymap_[Qt::Key_F13        ] = osgGA::GUIEventAdapter::KEY_F13;
    keymap_[Qt::Key_F14        ] = osgGA::GUIEventAdapter::KEY_F14;
    keymap_[Qt::Key_F15        ] = osgGA::GUIEventAdapter::KEY_F15;
    keymap_[Qt::Key_F16        ] = osgGA::GUIEventAdapter::KEY_F16;
    keymap_[Qt::Key_F17        ] = osgGA::GUIEventAdapter::KEY_F17;
    keymap_[Qt::Key_F18        ] = osgGA::GUIEventAdapter::KEY_F18;
    keymap_[Qt::Key_F19        ] = osgGA::GUIEventAdapter::KEY_F19;
    keymap_[Qt::Key_F20        ] = osgGA::GUIEventAdapter::KEY_F20;
    keymap_[Qt::Key_hyphen     ] = '-';
    keymap_[Qt::Key_Equal      ] = '=';
    keymap_[Qt::Key_division   ] = osgGA::GUIEventAdapter::KEY_KP_Divide;
    keymap_[Qt::Key_multiply   ] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
    keymap_[Qt::Key_Minus      ] = '-';
    keymap_[Qt::Key_Plus       ] = '+';
    keymap_[Qt::Key_Insert     ] = osgGA::GUIEventAdapter::KEY_KP_Insert;
}


~QtKeyboardMap()
{
}


int remapKey( QKeyEvent* event )
{
    std::map<unsigned int,int>::iterator itr = keymap_.find( event->key() );
    if ( itr == keymap_.end() )
	return int(*(event->text().toLatin1().data()));

    return itr->second;
}


private:
    std::map<unsigned int,int> keymap_;
};

static QtKeyboardMap sKeyboardMap;



// ODOpenGLWidget
ODOpenGLWidget::ODOpenGLWidget( QWidget* parent, Qt::WindowFlags flags )
    : QOpenGLWidget(parent,flags)
    , graphicswindow_(new osgViewer::GraphicsWindowEmbedded(this->x(),this->y(),
				this->width(),this->height()))
{
    setAttribute( Qt::WA_NativeWindow );
    setMouseTracking( true );
    setFocusPolicy( Qt::WheelFocus );

    graphicswindow_->setWindowRectangle( x(), y(), width(), height() );
    getEventQueue()->syncWindowRectangleWithGraphicsContext();
    scalex_ = scaley_ = QApplication::desktop()->devicePixelRatio();
}


ODOpenGLWidget::~ODOpenGLWidget()
{
}


void ODOpenGLWidget::initializeGL()
{
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    ds->setNvOptimusEnablement( 1 );
    ds->setStereo( false );
}


void ODOpenGLWidget::paintGL()
{
    OpenThreads::ScopedReadLock locker( mutex_ );

    if ( isfirstframe_ )
    {
	isfirstframe_ = false;

/*
	std::vector<osg::Camera*> cameras;
	viewer_->getCameras( cameras );
	for ( auto* cam : cameras )
	    cam->getGraphicsContext()->setDefaultFboId(
						defaultFramebufferObject() );
*/
    }

    if ( viewer_ )
	viewer_->frame();
}


void ODOpenGLWidget::resizeGL( int w, int h )
{
    getEventQueue()->windowResize( x()*scalex_, y()*scaley_,
				   w*scalex_, h*scaley_ );
    graphicswindow_->resized( x()*scalex_, y()*scaley_, w*scalex_, h*scaley_ );

/*
    std::vector<osg::Camera*> cameras;
    viewer_->getCameras( cameras );
    for ( auto* cam : cameras )
	cam->setViewport( 0, 0, this->width(), this->height() );
*/

    update();
}


void ODOpenGLWidget::setKeyboardModifiers( QInputEvent* ev )
{
    unsigned int modkey = ev ->modifiers() &
		 (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
    unsigned int modkeymask = 0;
    if ( modkey & Qt::ShiftModifier )
	modkeymask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if ( modkey & Qt::ControlModifier )
	modkeymask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if ( modkey & Qt::AltModifier )
	modkeymask |= osgGA::GUIEventAdapter::MODKEY_ALT;

    getEventQueue()->getCurrentEventState()->setModKeyMask( modkeymask );
}


void ODOpenGLWidget::keyPressEvent( QKeyEvent* event )
{
    setKeyboardModifiers( event );
    const int value = sKeyboardMap.remapKey( event );
    getEventQueue()->keyPress( value );
}


void ODOpenGLWidget::keyReleaseEvent( QKeyEvent* event )
{
    if ( event->isAutoRepeat() )
	event->ignore();
    else
    {
	setKeyboardModifiers( event );
	const int value = sKeyboardMap.remapKey( event );
	getEventQueue()->keyRelease( value );
    }
}


void ODOpenGLWidget::mousePressEvent( QMouseEvent* ev  )
{
    unsigned int button = 0;
    switch ( ev ->button() )
    {
	case Qt::LeftButton: button = 1; break;
	case Qt::MidButton: button = 2; break;
	case Qt::RightButton: button = 3; break;
	case Qt::NoButton: button = 0; break;
	default: button = 0; break;
    }

    setKeyboardModifiers( ev  );
    getEventQueue()->mouseButtonPress( ev->x()*scalex_, ev->y()*scaley_,
	    			       button );
}


void ODOpenGLWidget::mouseReleaseEvent( QMouseEvent* ev  )
{
    unsigned int button = 0;
    switch ( ev ->button() )
    {
	case Qt::LeftButton: button = 1; break;
	case Qt::MidButton: button = 2; break;
	case Qt::RightButton: button = 3; break;
	case Qt::NoButton: button = 0; break;
	default: button = 0; break;
    }

    setKeyboardModifiers( ev  );
    getEventQueue()->mouseButtonRelease(
			ev->x()*scalex_, ev->y()*scaley_, button );
}


void ODOpenGLWidget::mouseDoubleClickEvent( QMouseEvent* ev  )
{
    unsigned int button = 0;
    switch ( ev ->button() )
    {
	case Qt::LeftButton: button = 1; break;
	case Qt::MidButton: button = 2; break;
	case Qt::RightButton: button = 3; break;
	case Qt::NoButton: button = 0; break;
	default: button = 0; break;
    }

    setKeyboardModifiers( ev  );
    getEventQueue()->mouseButtonPress( ev->x()*scalex_, ev->y()*scaley_,
				       button );
}


void ODOpenGLWidget::mouseMoveEvent( QMouseEvent* ev  )
{
    setKeyboardModifiers( ev  );
    getEventQueue()->mouseMotion( ev->x()*scalex_, ev->y()*scaley_ );
}


void ODOpenGLWidget::wheelEvent( QWheelEvent* ev  )
{
    setKeyboardModifiers( ev  );

    getEventQueue()->mouseMotion( ev->position().x()*scalex_,
				  ev->position().y()*scaley_ );

    const QPoint delta = ev->angleDelta();
    const bool isvertical = abs(delta.y()) > abs(delta.x());
    getEventQueue()->mouseScroll(
	isvertical ? (delta.y()>0 ? osgGA::GUIEventAdapter::SCROLL_UP
				  : osgGA::GUIEventAdapter::SCROLL_DOWN)
		   : (delta.x()>0 ? osgGA::GUIEventAdapter::SCROLL_LEFT
				  : osgGA::GUIEventAdapter::SCROLL_RIGHT) );
}


osgGA::EventQueue* ODOpenGLWidget::getEventQueue() const
{
    return graphicswindow_->getEventQueue();
}


void ODOpenGLWidget::setViewer( osgViewer::ViewerBase* vwr )
{
    viewer_ = vwr;
}


// ODOpenGLWindow
ODOpenGLWindow::ODOpenGLWindow( QWidget* )
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate,nullptr)
    , graphicswindow_(new osgViewer::GraphicsWindowEmbedded(this->x(),this->y(),
				this->width(),this->height()))
{
    qwidget_ = QWidget::createWindowContainer( this );

    graphicswindow_->setWindowRectangle( x(), y(), width(), height() );
    getEventQueue()->syncWindowRectangleWithGraphicsContext();
    scalex_ = scaley_ = QApplication::desktop()->devicePixelRatio();
}


ODOpenGLWindow::~ODOpenGLWindow()
{
}


void ODOpenGLWindow::initializeGL()
{
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    ds->setNvOptimusEnablement( 1 );
    ds->setStereo( false );
}


void ODOpenGLWindow::paintGL()
{
    OpenThreads::ScopedReadLock locker( mutex_ );

    if ( isfirstframe_ )
    {
	isfirstframe_ = false;

	std::vector<osg::Camera*> cameras;
	viewer_->getCameras( cameras );
	for ( auto* cam : cameras )
	    cam->getGraphicsContext()->setDefaultFboId(
						defaultFramebufferObject() );
    }

    viewer_->frame();
}


void ODOpenGLWindow::resizeGL( int w, int h )
{
    getEventQueue()->windowResize( x()*scalex_, y()*scaley_,
				   w*scalex_, h*scaley_ );
    graphicswindow_->resized( x()*scalex_, y()*scaley_, w*scalex_, h*scaley_ );

    std::vector<osg::Camera*> cameras;
    viewer_->getCameras( cameras );
    for ( auto* cam : cameras )
	cam->setViewport( 0, 0, this->width(), this->height() );

    update();
}


void ODOpenGLWindow::setKeyboardModifiers( QInputEvent* ev )
{
    unsigned int modkey = ev ->modifiers() &
		 (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
    unsigned int modkeymask = 0;
    if ( modkey & Qt::ShiftModifier )
	modkeymask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if ( modkey & Qt::ControlModifier )
	modkeymask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if ( modkey & Qt::AltModifier )
	modkeymask |= osgGA::GUIEventAdapter::MODKEY_ALT;

    getEventQueue()->getCurrentEventState()->setModKeyMask( modkeymask );
}


void ODOpenGLWindow::keyPressEvent( QKeyEvent* event )
{
    setKeyboardModifiers( event );
    const int value = sKeyboardMap.remapKey( event );
    getEventQueue()->keyPress( value );
}


void ODOpenGLWindow::keyReleaseEvent( QKeyEvent* event )
{
    if ( event->isAutoRepeat() )
	event->ignore();
    else
    {
	setKeyboardModifiers( event );
	const int value = sKeyboardMap.remapKey( event );
	getEventQueue()->keyRelease( value );
    }
}


void ODOpenGLWindow::mousePressEvent( QMouseEvent* ev  )
{
    unsigned int button = 0;
    switch ( ev ->button() )
    {
	case Qt::LeftButton: button = 1; break;
	case Qt::MidButton: button = 2; break;
	case Qt::RightButton: button = 3; break;
	case Qt::NoButton: button = 0; break;
	default: button = 0; break;
    }

    setKeyboardModifiers( ev  );
    getEventQueue()->mouseButtonPress( ev->x()*scalex_, ev->y()*scaley_,
	    			       button );
}


void ODOpenGLWindow::mouseReleaseEvent( QMouseEvent* ev  )
{
    unsigned int button = 0;
    switch ( ev ->button() )
    {
	case Qt::LeftButton: button = 1; break;
	case Qt::MidButton: button = 2; break;
	case Qt::RightButton: button = 3; break;
	case Qt::NoButton: button = 0; break;
	default: button = 0; break;
    }

    setKeyboardModifiers( ev  );
    getEventQueue()->mouseButtonRelease(
			ev->x()*scalex_, ev->y()*scaley_, button );
}


void ODOpenGLWindow::mouseDoubleClickEvent( QMouseEvent* ev  )
{
    unsigned int button = 0;
    switch ( ev ->button() )
    {
	case Qt::LeftButton: button = 1; break;
	case Qt::MidButton: button = 2; break;
	case Qt::RightButton: button = 3; break;
	case Qt::NoButton: button = 0; break;
	default: button = 0; break;
    }

    setKeyboardModifiers( ev  );
    getEventQueue()->mouseButtonPress( ev->x()*scalex_, ev->y()*scaley_,
				       button );
}


void ODOpenGLWindow::mouseMoveEvent( QMouseEvent* ev  )
{
    setKeyboardModifiers( ev  );
    getEventQueue()->mouseMotion( ev->x()*scalex_, ev->y()*scaley_ );
}


void ODOpenGLWindow::wheelEvent( QWheelEvent* ev  )
{
    setKeyboardModifiers( ev  );

    getEventQueue()->mouseMotion( ev->position().x()*scalex_,
				  ev->position().y()*scaley_ );

    const QPoint delta = ev->angleDelta();
    const bool isvertical = abs(delta.y()) > abs(delta.x());
    getEventQueue()->mouseScroll(
	isvertical ? (delta.y()>0 ? osgGA::GUIEventAdapter::SCROLL_UP
				  : osgGA::GUIEventAdapter::SCROLL_DOWN)
		   : (delta.x()>0 ? osgGA::GUIEventAdapter::SCROLL_LEFT
				  : osgGA::GUIEventAdapter::SCROLL_RIGHT) );
}


osgGA::EventQueue* ODOpenGLWindow::getEventQueue() const
{
    return graphicswindow_->getEventQueue();
}


void ODOpenGLWindow::setViewer( osgViewer::ViewerBase* vwr )
{
    viewer_ = vwr;
}
