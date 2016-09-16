#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2008
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
