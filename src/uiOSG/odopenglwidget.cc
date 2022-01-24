/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/

#include "odopenglwidget.h"

#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <QApplication>
#include <QDesktopWidget>
#include <QInputEvent>
#include <QMouseEvent>
#include <QWheelEvent>

// ODGraphicsWindow
ODGraphicsWindow2::ODGraphicsWindow2()
    : osgViewer::GraphicsWindow()
{}


ODGraphicsWindow2::~ODGraphicsWindow2()
{}


// ODOpenGLWidget
ODOpenGLWidget::ODOpenGLWidget( QWidget* parent, Qt::WindowFlags flags )
    : QOpenGLWidget(parent,flags)
    , graphicswindow_(new ODGraphicsWindow2)
    , viewer_(new osgViewer::Viewer)
{
    graphicswindow_->setWindowRectangle( x(), y(), width(), height() );

    scalex_ = scaley_ = QApplication::desktop()->devicePixelRatio();
    osg::Camera* camera = new osg::Camera;
    camera->setViewport( 0, 0, width(), height() );
    camera->setClearColor( osg::Vec4( 0.9f, 0.9f, 1.f, 1.f ) );
    const double aspectratio =
	sCast(double,this->width()) / sCast(double,this->height());
    camera->setProjectionMatrixAsPerspective( 30., aspectratio, 1., 1000. );
    camera->setGraphicsContext( graphicswindow_ );

    viewer_->setCamera( camera );
//    viewer_->setSceneData(geode);
    auto* manipulator = new osgGA::TrackballManipulator;
    manipulator->setAllowThrow( false );
    setMouseTracking( true );
    viewer_->setCameraManipulator( manipulator );
    viewer_->setThreadingModel( osgViewer::Viewer::SingleThreaded );
    viewer_->realize();
}


ODOpenGLWidget::~ODOpenGLWidget()
{
}


void ODOpenGLWidget::initializeGL()
{
    QOpenGLWidget::initializeGL();
}


void ODOpenGLWidget::paintGL()
{
    QOpenGLWidget::paintGL();
}


void ODOpenGLWidget::resizeGL( int w, int h )
{
    getEventQueue()->windowResize( x()*scalex_, y()*scaley_,
				   w*scalex_, h*scaley_ );
    graphicswindow_->resized( x()*scalex_, y()*scaley_, w*scalex_, h*scaley_ );
    osg::Camera* camera = viewer_->getCamera();
    camera->setViewport( 0, 0, this->width()*scalex_, this->height()*scaley_ );
    QOpenGLWidget::resizeGL( w, h );
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
    getEventQueue()->mouseButtonPress( ev->x()*scalex_, ev->y()*scaley_,
	    			       button );
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
    osgGA::EventQueue* eventQueue = graphicswindow_->getEventQueue();
    return eventQueue;
}
