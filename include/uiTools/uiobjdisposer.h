#ifndef uiobjdisposer_h
#define uiobjdisposer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2008
 RCS:           $Id: uiobjdisposer.h,v 1.5 2012-08-03 13:01:14 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "callback.h"
#include "sets.h"
class Timer;

/*!\brief Disposes after a couple of msecs to avoid all kinds of trouble.

  The CallBack should have the object to be disposed of as CallBacker.
  Usage like:

    nonmdldlg->windowClosed.notify( mCB(uiOBJDISP(),uiObjDisposer,go) );
 
 */

mClass(uiTools) uiObjDisposer : public CallBacker
{ 	
public:

    void			go(CallBacker*);

protected:

    ObjectSet<Timer>		timers_;
    ObjectSet<CallBacker>	objs_;

				uiObjDisposer();

    void			doDel(CallBacker*);
    mGlobal(uiTools) friend uiObjDisposer*	uiOBJDISP();

};

mGlobal(uiTools) uiObjDisposer* uiOBJDISP();


#endif

