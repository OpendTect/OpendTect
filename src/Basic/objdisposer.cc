/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "objdisposer.h"
#include "ptrman.h"
#include "timer.h"

static ObjectSet<Timer> todeltimers;


ObjDisposer* OBJDISP()
{
    mDefineStaticLocalObject( PtrMan<ObjDisposer>, theinst,
			      = new ObjDisposer );
    return theinst.ptr();
}


ObjDisposer::ObjDisposer()
{
}


void ObjDisposer::go( CallBacker* obj )
{
    if ( !obj ) return;

    objs_ += obj;
    Timer* newtimer = new Timer;
    newtimer->tick.notify( mCB(this,ObjDisposer,doDel) );
    timers_ += newtimer;
    newtimer->start( 250, true );
}


void ObjDisposer::doDel( CallBacker* in )
{
    deepErase( todeltimers );
    mDynamicCastGet(Timer*,tim,in)
    if ( !tim ) return; // Huh?

    const int idxof = timers_.indexOf( tim );
    if ( idxof < 0 ) return; // Huh?

    CallBacker* obj = objs_[idxof];
    objs_.removeSingle( idxof );
    timers_.removeSingle( idxof );

    delete obj;
    todeltimers += tim;
}
