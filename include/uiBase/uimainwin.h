#ifndef uimainwin_h
#define uimainwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.h,v 1.11 2001-12-17 10:47:55 arend Exp $
________________________________________________________________________

-*/

#include <uiparent.h>
#include <uihandle.h>

class uiMainWinBody;
class uiStatusBar;
class uiToolBar;
class uiMenuBar;
class uiObject;
class QWidget;

class uiMainWin : public uiParent
{

public:
			uiMainWin( uiParent* parnt=0, 
				   const char* nm="uiMainWin",
				   bool wantStatusBar = true, 
				   bool wantMenuBar = true,
				   bool wantToolBar = false,
				   bool modal=false );

    virtual		~uiMainWin();

    uiStatusBar* 	statusBar();
    uiMenuBar* 		menuBar();
    uiToolBar* 		toolBar();

    static uiMainWin*	activeWindow();

			//! get uiMainWin for mwimpl if it is a uiMainWinBody
    static uiMainWin*	gtUiWinIfIsBdy(QWidget* mwimpl);

    void		setCaption( const char* txt );
    void                show();
    void		toStatusBar(const char*);

    uiObject*		uiObj();
    const uiObject*	uiObj() const;

    void		shallowRedraw( CallBacker* =0 )		{reDraw(false);}
    void		deepRedraw( CallBacker* =0 )		{reDraw(true); }
    void		reDraw(bool deep);

protected:

//    void		doPolish(CallBacker*);

			uiMainWin( const char* );

    uiMainWinBody*	body_;
};

#endif
