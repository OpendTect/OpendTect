#ifndef uistatusbar_h
#define uistatusbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.h,v 1.1 2000-11-27 10:19:28 bert Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class QStatusBar;
//template <class T> class i_QObjWrapper;
//typedef i_QObjWrapper<QStatusBar> i_QStatusBar;

class uiStatusBar : public uiNoWrapObj<QStatusBar>
{
//friend class i_QStatusBar;
friend class uiMainWin;
public:

    void 		message( const char* msg);

protected:

                        uiStatusBar( uiMainWin* parnt, const char* nm,  
                                     QStatusBar& ); 
			//!< for use by uiMainWin
    const QWidget*	qWidget_() const;
};

#endif // uiStatusBar
