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

class uiBaseMapTBMgr;
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

    void		mouseCursorExchangeCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);
    bool		closeOK();

    uiSurveyMap*	basemapview_;
    uiBaseMapTBMgr*	tbmgr_;
    uiDockWin*		treedw_;
    uiTreeView*		tree_;
    uiBasemapTreeTop*	topitem_;
    MouseCursorExchange* mousecursorexchange_;

    TypeSet<int>	ids_;
};

#endif

