/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "timer.h"
#include "qtimercomm.h" 

mUseQtnamespace

Timer::Timer( const char* nm )
    : NamedObject(nm)
    , tick(this)
    , timer_(new QTimer(0))
    , comm_(new QTimerComm(timer_,this))
    , scriptpolicy_(DefaultPolicy)
{}


Timer::~Timer()
{ 
    if ( isActive() )
	stop();

    comm_->deactivate();
    delete timer_;
    delete comm_;
}


bool Timer::isActive() const
{ return timer_->isActive(); }


bool Timer::isSingleShot() const
{ return timer_->isSingleShot(); }



void Timer::start( int msec, bool sshot )
{
    timer_->setSingleShot( sshot );
    timer_->setInterval( msec );

    timerStarts()->trigger( this );
    timer_->start();
}


void Timer::stop() 
{
    timer_->stop();
    timerStopped()->trigger( this );
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
