#ifndef uimainwin_h
#define uimainwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.h,v 1.7 2001-08-24 14:23:42 arend Exp $
________________________________________________________________________

-*/

#include <uiparent.h>
#include <uihandle.h>

class uiMainWinBody;
class uiStatusBar;
class uiMenuBar;
class uiObject;

class uiMainWin : public uiParent
{

public:
			uiMainWin( uiParent* parnt=0, 
				   const char* nm="uiMainWin",
				   bool wantStatusBar = true, 
				   bool wantMenuBar = true );


    uiStatusBar* 	statusBar();
    uiMenuBar* 		menuBar();

    void		setCaption( const char* txt );

    void                show();

    uiObject*		uiObj();
    const uiObject*	uiObj() const;

    void		shallowRedraw( CallBacker* =0 )		{reDraw(false);}
    void		deepRedraw( CallBacker* =0 )		{reDraw(true); }
    void		reDraw(bool deep);

protected:

//    void		doPolish(CallBacker*);


private:

    uiMainWinBody*	body_;
};

#endif
