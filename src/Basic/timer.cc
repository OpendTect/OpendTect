/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: timer.cc,v 1.1 2009-03-18 04:24:39 cvsnanne Exp $";

#include "timer.h"
#include "qtimercomm.h" 

Timer::Timer( const char* nm )
    : NamedObject(nm)
    , tick(this)
    , timer_(new QTimer(0))
    , comm_(new QTimerComm(timer_,this))
{}


Timer::~Timer()
{ 
    comm_->deactivate();
    delete timer_;
    delete comm_;
}


bool Timer::isActive() const
{ return timer_->isActive(); }


void Timer::start( int msec, bool sshot )
{
    timer_->setSingleShot( sshot );
    timer_->start( msec );
}


void Timer::stop() 
{ timer_->stop(); }
