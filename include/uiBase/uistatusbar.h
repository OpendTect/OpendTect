#ifndef uistatusbar_h
#define uistatusbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.h,v 1.8 2004-04-29 12:33:29 arend Exp $
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

    int			addMsgFld(const char* lbltxt=0,
	    			  const char* tooltip =0,
				  TxtAlign al=Left,
	    			  int stretch=1);

    int			addMsgFld(const char* tooltip,
				  TxtAlign al=Left,
	    			  int stretch=1);

    void		setToolTip(int,const char*);
    void		setTxtAlign(int,TxtAlign);
    void		setLabelTxt(int,const char*);

    void 		message(const char*,int fldidx=0, int msecs=-1);

protected:

                        uiStatusBar(uiMainWin*,const char*,QStatusBar&); 
private:

    uiStatusBarBody*	body_;
    uiStatusBarBody&	mkbody(uiMainWin*, const char*, QStatusBar&);
};


#endif
