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


osgGA::EventQueue* ODOpenGLWidget::getEventQueue() const
{
    osgGA::EventQueue* eventQueue = graphicswindow_->getEventQueue();
    return eventQueue;
}
