#ifndef uistatusbar_h
#define uistatusbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.h,v 1.4 2002-01-29 11:13:20 bert Exp $
________________________________________________________________________

-*/

#include <uihandle.h>

class uiStatusBarBody;
class QStatusBar;
class uiMainWin;


class uiStatusBar : public uiObjHandle
{

    friend class	uiMainWinBody;

public:

    enum		TxtAlign{ Left, Centre, Right };

    void 		message(const char*,int fldidx=0);
    void		addMsgFld(const char* tooltip ="",TxtAlign al=Left,
	    			  int stretch=1);

protected:

                        uiStatusBar(uiMainWin*,const char*,QStatusBar&); 
private:

    uiStatusBarBody*	body_;
    uiStatusBarBody&	mkbody(uiMainWin*, const char*, QStatusBar&);
};


#endif
