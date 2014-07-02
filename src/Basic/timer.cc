/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "timer.h"
#ifndef OD_NO_QT
#include "qtimercomm.h" 
#endif

mUseQtnamespace

Timer::Timer( const char* nm )
    : NamedObject(nm)
    , tick(this)
#ifndef OD_NO_QT
    , timer_(new QTimer(0))
    , comm_(new QTimerComm(timer_,this))
#else
    , timer_( 0 )
    , comm_ ( 0 )
#endif
    , scriptpolicy_(DefaultPolicy)
{}


Timer::~Timer()
{ 
    if ( isActive() )
	stop();

#ifndef OD_NO_QT
    comm_->deactivate();
    delete timer_;
    delete comm_;
#endif
}


bool Timer::isActive() const
{
#ifdef OD_NO_QT
    return false;
#else
    return timer_->isActive();
#endif
}


bool Timer::isSingleShot() const
{
#ifdef OD_NO_QT
    return false;
#else
    return timer_->isSingleShot();
#endif
}



void Timer::start( int msec, bool sshot )
{
#ifdef OD_NO_QT
    return;
#else
    timer_->setSingleShot( sshot );
    timer_->setInterval( msec );

    timerStarts()->trigger( this );
    timer_->start();
#endif
}


void Timer::stop() 
{
#ifdef OD_NO_QT
    return;
#else
    timer_->stop();
    timerStopped()->trigger( this );
#endif
}


void Timer::notifyHandler()
{
    timerShoots()->trigger( this );
    tick.trigger( this );
    timerShot()->trigger( this );
}


static bool userwaitflag_ = false;

void Timer::setScriptPolicy( ScriptPolicy policy )
{
    if ( policy == UserWait )
    {
	scriptpolicy_ = DontWait;
	setUserWaitFlag( true );
    }
    else
	scriptpolicy_ = policy;
}


Timer::ScriptPolicy Timer::scriptPolicy() const
{ return scriptpolicy_; }


bool Timer::setUserWaitFlag( bool yn )
{
    const bool oldval = userwaitflag_;
    userwaitflag_ = yn;
    return oldval;
}


#define mImplStaticNotifier( func ) \
    Notifier<Timer>* Timer::func() \
    { \
	static Notifier<Timer> func##notifier(0); \
	return &func##notifier; \
    }

mImplStaticNotifier( timerStarts )
mImplStaticNotifier( timerStopped )
mImplStaticNotifier( timerShoots )
mImplStaticNotifier( timerShot )
