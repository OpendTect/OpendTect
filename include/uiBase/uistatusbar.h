#ifndef uistatusbar_h
#define uistatusbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.h,v 1.2 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include <uihandle.h>

class uiStatusBarBody;
class QStatusBar;
class uiMainWin;

class uiStatusBar : public uiObjHandle
{
//friend class i_QStatusBar;
friend class uiMainWinBody;
public:

    void 		message( const char* msg);

protected:

                        uiStatusBar( uiMainWin* parnt, const char* nm,  
                                     QStatusBar& ); 
private:

    uiStatusBarBody*	body_;
    uiStatusBarBody&	mkbody(uiMainWin*, const char*, QStatusBar&);
};

#endif // uiStatusBar
