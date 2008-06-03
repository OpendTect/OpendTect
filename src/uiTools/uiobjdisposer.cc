/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2008
 RCS:           $Id: uiobjdisposer.cc,v 1.1 2008-06-03 11:34:35 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjdisposer.h"
#include "timer.h"


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
    newtimer->start( 50, true );
}


void uiObjDisposer::doDel( CallBacker* in )
{
    mDynamicCastGet(Timer*,tim,in)
    if ( !tim ) return; // Huh?

    const int idxof = timers_.indexOf( tim );
    if ( idxof < 0 ) return; // Huh?

    CallBacker* obj = objs_[idxof];
    objs_.remove( idxof );
    timers_.remove( idxof );

    delete tim; delete obj;
}
