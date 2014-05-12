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
class uiTreeView;
class MouseCursorExchange;

mClass(uiBasemap) uiBasemapWin : public uiMainWin
{
public:
    			uiBasemapWin(uiParent*);
			~uiBasemapWin();

    void		setMouseCursorExchange(MouseCursorExchange*);

private:
    void		initWin(CallBacker*);
    void		initView();
    void		initTree();
    void		initToolBars();
    void		iconClickCB(CallBacker*);
    void		mouseCursorExchangeCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);
    bool		closeOK();

    uiSurveyMap*	basemapview_;
    uiToolBar*		vwtoolbar_;
    uiToolBar*		itemtoolbar_;
    uiDockWin*		treedw_;
    uiTreeView*		tree_;
    uiBasemapTreeTop*	topitem_;
    MouseCursorExchange* mousecursorexchange_;

    TypeSet<int>		ids_;
};

#endif

