#ifndef uimainwin_h
#define uimainwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.h,v 1.4 2001-06-07 21:22:51 windev Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

template <class T> class i_QObjWrapper;
class QMainWindow;
mTemplTypeDefT( i_QObjWrapper, QMainWindow, i_QMainWindow )

class i_LayoutMngr;
class uiStatusBar;
class uiMenuBar;
class uiGroup;

class uiMainWin : public uiObject
{
public:
			uiMainWin( uiParent* parnt=0, 
				   const char* nm="uiMainWin",
				   bool wantStatusBar = true, 
				   bool wantMenuBar = true );
    virtual		~uiMainWin();

    uiStatusBar* 	statusBar();
    uiMenuBar* 		menuBar();

protected:
    const QWidget*	qWidget_() const;
    void		qThingDel( i_QObjWrp* qth );

    virtual i_LayoutMngr* mLayoutMngr() 	{ return mLoMngr; } 
    virtual i_LayoutMngr* prntLayoutMngr() 	{ return 0; }
    
    virtual void	forceRedraw_( bool deep );
    virtual void	finalise_();

    // don't change order of these 3 attributes!
    i_LayoutMngr*	mLoMngr;
    i_QMainWindow*	mQtThing;
    uiGroup*		mCentralWidget;

    uiStatusBar* 	mStatusBar;
    uiMenuBar* 		mMenuBar;

    virtual const uiParent& clientWidget_() 	const;

    virtual void	polish();

};


#endif
