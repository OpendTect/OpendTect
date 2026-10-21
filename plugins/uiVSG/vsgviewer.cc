/*+
________________________________________________________________________

 Copyright: (C) 1995-2026 dGB Beheer B.V.
 License:   https://dgbes.com/licensing
________________________________________________________________________

 -*/

#include "vsgviewer.h"

#include <QCoreApplication>

//Adapted from vsgQt 0.5.0 (https://github.com/vsg-dev/vsgQt).

dGBVSGViewer::dGBVSGViewer( int msectimerinterval )
{
    if ( msectimerinterval > 0 )
	setInterval( msectimerinterval );
}


dGBVSGViewer::~dGBVSGViewer()
{
    stop();
}


bool dGBVSGViewer::pollEvents( bool discardpreviousevents )
{
    if ( discardpreviousevents )
	_events.clear();

    for ( auto& window : _windows )
    {
	_events.splice( _events.end(), window->bufferedEvents );
	window->bufferedEvents.clear();
    }

    return !_events.empty();
}


void dGBVSGViewer::request()
{
    ++requests_;
}


void dGBVSGViewer::stop()
{
    timer_.stop();
}


void dGBVSGViewer::render( double simulationtime )
{
    if ( !continuousupdate_ && requests_.load()==0 )
	return;

    if ( advanceToNextFrame(simulationtime) )
    {
	handleEvents();
	update();
	recordAndSubmit();
	present();
    }
    else if ( status->cancel() )
	QCoreApplication::quit();

    requests_ = 0;
}


void dGBVSGViewer::setInterval( int msectimerinterval )
{
    timer_.setInterval( msectimerinterval );
    QObject::connect( &timer_, &QTimer::timeout, [this]() { render(); } );
    timer_.start();
}
