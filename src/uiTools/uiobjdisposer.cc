/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiobjdisposer.cc,v 1.5 2012/01/12 12:28:56 cvsranojay Exp $";

#include "uiobjdisposer.h"
#include "timer.h"

static ObjectSet<Timer> todeltimers;


uiObjDisposer* uiOBJDISP()
{
    static uiObjDisposer* theinst = 0;
    if ( !theinst )
	theinst = new uiObjDisposer;
    return theinst;
}


uiObjDisposer::uiObjDisposer()
{
}


void uiObjDisposer::go( CallBacker* obj )
{
    if ( !obj ) return;

    objs_ += obj;
    Timer* newtimer = new Timer;
    newtimer->tick.notify( mCB(this,uiObjDisposer,doDel) );
    timers_ += newtimer;
    newtimer->start( 250, true );
}


void uiObjDisposer::doDel( CallBacker* in )
{
    deepErase( todeltimers );
    mDynamicCastGet(Timer*,tim,in)
    if ( !tim ) return; // Huh?

    const int idxof = timers_.indexOf( tim );
    if ( idxof < 0 ) return; // Huh?

    CallBacker* obj = objs_[idxof];
    objs_.remove( idxof );
    timers_.remove( idxof );

    delete obj;
    todeltimers += tim;
}
