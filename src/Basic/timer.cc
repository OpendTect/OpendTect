/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: timer.cc,v 1.3 2009-12-04 14:36:42 cvsjaap Exp $";

#include "timer.h"
#include "qtimercomm.h" 

#include "thread.h"

static CallBack* telltale_ = 0;
static Threads::Mutex telltalemutex_;
static ObjectSet<const Timer> firstshots_;

#define mTelltale( statements ) \
    if ( telltale_ ) \
	{ telltalemutex_.lock(); statements; telltalemutex_.unLock(); }


Timer::Timer( const char* nm )
    : NamedObject(nm)
    , tick(this)
    , timer_(new QTimer(0))
    , comm_(new QTimerComm(timer_,this))
{}


Timer::~Timer()
{ 
    mTelltale( firstshots_ -= this );
    comm_->deactivate();
    delete timer_;
    delete comm_;
}


bool Timer::isActive() const
{ return timer_->isActive(); }


void Timer::start( int msec, bool sshot )
{
    mTelltale( firstshots_ -= this; firstshots_ += this );
    timer_->setSingleShot( sshot );
    timer_->start( msec );
}


void Timer::stop() 
{
    mTelltale( firstshots_ -= this );
    timer_->stop();
}


void Timer::notifyHandler()
{
    bool peachon = false;
    mTelltale( peachon = firstshots_.indexOf(this) >= 0 );

    if ( peachon )
	telltale_->doCall( this );

    mTelltale( firstshots_ -= this );
    tick.trigger( this );

    if ( peachon )
	telltale_->doCall( this );
}


int Timer::nrFirstShotTimers()
{
    int sz = -1;
    mTelltale( sz = firstshots_.size() );
    return sz;
}


void Timer::unsetTelltale()
{
    mTelltale( delete telltale_; telltale_ = 0 );
}


void Timer::setTelltale( const CallBack& cb )
{
    unsetTelltale();
    firstshots_.erase();
    telltale_ = new CallBack( cb );
}
