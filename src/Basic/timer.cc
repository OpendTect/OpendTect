/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: timer.cc,v 1.2 2009-07-22 16:01:34 cvsbert Exp $";

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
