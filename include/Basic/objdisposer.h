#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "callback.h"
#include "sets.h"
class Timer;

/*!\brief Disposes after a couple of msecs to avoid all kinds of trouble.

  The CallBack should have the object to be disposed of as CallBacker.
  Usage like:

    nonmdldlg->windowClosed.notify( mCB(OBJDISP(),ObjDisposer,go) );

 */

mExpClass(Basic) ObjDisposer : public CallBacker
{
public:

    void			go(CallBacker*);

protected:

    ObjectSet<Timer>		timers_;
    ObjectSet<CallBacker>	objs_;

				ObjDisposer();

    void			doDel(CallBacker*);
    mGlobal(Basic) friend ObjDisposer*	OBJDISP();

};

mGlobal(Basic) ObjDisposer* OBJDISP();
