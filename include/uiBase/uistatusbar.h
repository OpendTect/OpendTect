#ifndef uistatusbar_h
#define uistatusbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.h,v 1.3 2002-01-18 14:27:39 arend Exp $
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

    enum		txtAlign{ left, centre, right };

    void 		message( const char* msg, int fldidx=0);
    void		addMsgFld( const char* tooltip ="", 
                                   txtAlign al=left, int stretch=1 );

protected:

                        uiStatusBar( uiMainWin* parnt, const char* nm,  
                                     QStatusBar& ); 
private:

    uiStatusBarBody*	body_;
    uiStatusBarBody&	mkbody(uiMainWin*, const char*, QStatusBar&);
};

#endif // uiStatusBar
