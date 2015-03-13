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

#include "uibasemap.h"
#include "uimainwin.h"

class uiBaseMapTBMgr;
class uiBasemapTreeTop;
class uiDockWin;
class uiLineItem;
class uiMapScaleObject;
class uiPixmapItem;
class uiSurveyBoxObject;
class uiToolBar;
class uiToolButton;
class uiTreeView;
class BaseMap;
class MouseCursorExchange;


mExpClass(uiBasemap) uiBasemapView : public uiBaseMap
{
public:
			uiBasemapView(uiParent*);
			~uiBasemapView();

    uiMapScaleObject*	getScaleBar();
    uiPixmapItem*	getNorthArrow();
    uiSurveyBoxObject*	getSurveyBox();

protected:
    void		addStdItems();
    void		init();
    void		mouseMoveCB(CallBacker*);
    virtual void	reDraw(bool deep=true);

    uiMapScaleObject*	scalebar_;
    uiPixmapItem*	northarrow_;
    uiSurveyBoxObject*	survbox_;

    uiLineItem*		horline_;
    uiLineItem*		vertline_;
};


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

    uiBasemapView*	basemapview_;
    uiBaseMapTBMgr*	tbmgr_;
    uiDockWin*		treedw_;
    uiTreeView*		tree_;
    uiBasemapTreeTop*	topitem_;
    MouseCursorExchange* mousecursorexchange_;

    TypeSet<int>	ids_;
};

#endif

