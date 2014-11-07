#ifndef uibasemapwin_h
#define uibasemapwin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"

#include "uimainwin.h"

class uiBasemapTreeTop;
class uiDockWin;
class uiSurveyMap;
class uiToolBar;
class uiToolButton;
class uiTreeView;
class BaseMap;
class MouseCursorExchange;

mClass(uiBasemap) uiBasemapWin : public uiMainWin
{ mODTextTranslationClass(uiBasemapWin)
public:
    			uiBasemapWin(uiParent*);
			~uiBasemapWin();

    void		setMouseCursorExchange(MouseCursorExchange*);
    BaseMap*		getBasemap();

private:

    void		initWin(CallBacker*);
    void		initView();
    void		initTree();
    void		initToolBars();
    void		viewCB(CallBacker*);
    void		removeCB(CallBacker*);
    void		iconClickCB(CallBacker*);
    void		mouseCursorExchangeCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);
    void		updateViewMode();
    bool		closeOK();

    uiSurveyMap*	basemapview_;
    uiToolBar*		vwtoolbar_;
    uiToolBar*		itemtoolbar_;
    uiDockWin*		treedw_;
    uiTreeView*		tree_;
    uiBasemapTreeTop*	topitem_;
    MouseCursorExchange* mousecursorexchange_;

    bool		pickmode_;
    uiToolButton*	viewbut_;
    uiToolButton*	removebut_;

    TypeSet<int>	ids_;
};

#endif

