#ifndef uistatusbar_h
#define uistatusbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.h,v 1.5 2002-04-23 09:57:20 bert Exp $
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
    void		setToolTip(int,const char*);
    void		setStretch(int,int);
    void		setTxtAlign(int,TxtAlign);
    void		setLabelTxt(int,const char*);

    void 		message(const char*,int fldidx=0);

protected:

                        uiStatusBar(uiMainWin*,const char*,QStatusBar&); 
private:

    uiStatusBarBody*	body_;
    uiStatusBarBody&	mkbody(uiMainWin*, const char*, QStatusBar&);
};


#endif
