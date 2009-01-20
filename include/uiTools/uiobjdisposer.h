#ifndef uiobjdisposer_h
#define uiobjdisposer_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2008
 RCS:           $Id: uiobjdisposer.h,v 1.3 2009-01-20 04:38:46 cvsranojay Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "sets.h"
class Timer;

/*!\brief Disposes after a couple of msecs to avoid all kinds of trouble.

  The CallBack should have the object to be disposed of as CallBacker.
  Usage like:

    nonmdldlg->windowClosed.notify( mCB(uiOBJDISP(),uiObjDisposer,go) );
 
 */

mClass uiObjDisposer : public CallBacker
{ 	
public:

    void			go(CallBacker*);

protected:

    ObjectSet<Timer>		timers_;
    ObjectSet<CallBacker>	objs_;

				uiObjDisposer();

    void			doDel(CallBacker*);
    mGlobal friend uiObjDisposer*	uiOBJDISP();

};

mGlobal uiObjDisposer* uiOBJDISP();


#endif
