/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		January 2022
________________________________________________________________________

-*/


#include "odosgviewer.h"
#include "odopenglwidget.h"

#include <QThread>

/*
   This class is a slightly modified version of the OSGRenderer class
   available at: https://github.com/openscenegraph/osgQt
*/

// ODOSGViewer
ODOSGViewer::ODOSGViewer( QObject* parent )
    : QObject(parent)
    , osgViewer::Viewer()
{
}


ODOSGViewer::~ODOSGViewer()
{
}


void ODOSGViewer::update()
{
    ODOpenGLWidget* glwidget = dynamic_cast<ODOpenGLWidget*>(parent());
    glwidget->update();
}


void ODOSGViewer::doInit()
{
    setKeyEventSetsDone( 0 );
    setReleaseContextAtEndOfFrameHint( false );

    timerid_ = startTimer( 10, Qt::PreciseTimer );
    lastframestarttime_.setStartTick( 0 );
}


bool ODOSGViewer::checkEvents()
{
    // check events from any attached sources
    for ( Devices::iterator eitr = _eventSources.begin();
	  eitr != _eventSources.end(); ++eitr)
    {
	osgGA::Device* es = eitr->get();
	if ( es->getCapabilities() & osgGA::Device::RECEIVE_EVENTS )
	{
	    if ( es->checkEvents())
		return true;
	}
    }

    // get events from all windows attached to Viewer.
    Windows windows;
    getWindows(windows);

    for ( Windows::iterator witr = windows.begin();
	  witr != windows.end(); ++witr )
    {
	if ( (*witr)->checkEvents() )
	    return true;
    }

    return false;
}


bool ODOSGViewer::checkNeedToDoFrame()
{
    // check if any event handler has prompted a redraw
    if ( _requestRedraw )
	return true;

    if ( _requestContinousUpdate )
	return true;

    // check if the view needs to update the scene graph
    // this check if camera has update callback and if scene requires to update scene graph
    if ( requiresUpdateSceneGraph() )
	return true;

    // check if the database pager needs to update the scene
    if ( getDatabasePager()->requiresUpdateSceneGraph() )
	return true;

    // check if the image pager needs to update the scene
    if ( getImagePager()->requiresUpdateSceneGraph() )
	return true;


    // check if the scene needs to be redrawn
    if ( requiresRedraw() )
	return true;

    // check if events are available and need processing
    if ( checkEvents() )
	return true;

    // and check again if any event handler has prompted a redraw
    if ( _requestRedraw )
	return true;

    if ( _requestContinousUpdate )
	return true;

    return false;
}


// called from ViewerWidget paintGL() method
void ODOSGViewer::frame( double simulationtime )
{
    // limit the frame rate
    if ( getRunMaxFrameRate() > 0.0 )
    {
	const double dt = lastframestarttime_.time_s();
	const double minframetime = 1.0 / getRunMaxFrameRate();

	if ( dt < minframetime )
	    QThread::usleep(
		static_cast<unsigned int>(1000000.0*(minframetime-dt)) );
    }

    // avoid excessive CPU loading when no frame is required in ON_DEMAND mode
    if ( getRunFrameScheme() == osgViewer::ViewerBase::ON_DEMAND )
    {
	const double dt = lastframestarttime_.time_s();
	if ( dt < 0.01 )
	    OpenThreads::Thread::microSleep(
		static_cast<unsigned int>(1000000.0 *(0.01 - dt)) );
    }

    // record start frame time
    lastframestarttime_.setStartTick();

    // make frame
#if 1
    osgViewer::Viewer::frame( simulationtime );
#else

    if ( _done) return;

    // OSG_NOTICE<<std::endl<<"CompositeViewer::frame()"<<std::endl<<std::endl;

    if ( _firstFrame )
    {
	viewerInit();

	if ( !isRealized() )
	    realize();

	_firstFrame = false;
    }

    advance( simulationtime );

    eventTraversal();
    updateTraversal();
    renderingTraversals();
#endif
}


void ODOSGViewer::timerEvent( QTimerEvent* )
{
    // application is about to quit, just return
    if ( applicationabouttoquit_ )
	return;

    // ask ViewerWidget to update 3D view
    if ( getRunFrameScheme() != osgViewer::ViewerBase::ON_DEMAND ||
	checkNeedToDoFrame())
    {
	update();
    }
}
