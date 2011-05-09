#ifndef uiobjectitemviewwin_h
#define uiobjectitemviewwin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
 RCS:           $Id: uiobjectitemviewwin.h,v 1.4 2011-05-09 08:48:09 cvsbert Exp $
________________________________________________________________________

-*/


/*! brief A uiFMainWin that holds embedded uiObjects and controls !*/

#include "uimainwin.h"
#include "uigroup.h"
#include "uiobjectitemview.h"

class uiCheckBox;
class uiObjectItem;
class uiObjectItemView;
class uiObjectItemViewInfoBar;
class uiObjectItemViewControl;
class uiSliderExtra;
class uiToolBar;
class uiToolButton;

mClass uiObjectItemViewWin : public uiMainWin
{
public:    
			uiObjectItemViewWin(uiParent*,const char* title);

    uiObjectItemView*	mainViewer() 	{ return mainviewer_; }

    void 		addObject(uiObject* grp,uiObject* infogrp=0);
    void 		addGroup(uiGroup* grp,uiGroup* infogrp=0);
    void 		addItem(uiObjectItem* itm,uiObjectItem* infoitm=0);

    void		removeAllItems();

protected:

    uiObjectItemView*	mainviewer_;

    uiObjectItemViewInfoBar* infobar_;

    uiSliderExtra*	versliderfld_;
    uiSliderExtra*	horsliderfld_;
    uiToolButton*	fittoscreenbut_;
    uiCheckBox*		zoomratiofld_;

    int			startwidth_;
    int			startheight_;

    float		hslval_;
    float		vslval_;

    void		init();
    void		makeSliders();
    void 		setUpView();
    void		reSizeItems();

    void		fitToScreen(CallBacker*);
    void		reSizeSld(CallBacker*);
    void		scrollBarCB(CallBacker*);
};


mClass uiObjectItemViewInfoBar : public uiObjectItemView
{
public:    

			uiObjectItemViewInfoBar(uiParent*);

    void		addItem(uiObjectItem*,uiObjectItem* coupleditm);
    virtual void	addItem( uiObjectItem* itm, int stretch=1 )
			{ return uiObjectItemView::addItem(itm,stretch); }
    void		removeItem(uiObjectItem* itm);
    void		updateItemsPos();
    void		reSizeItems();

protected:
    ObjectSet<uiObjectItem> coupleditems_; 
};



mClass uiObjectItemViewControl : public uiGroup
{
public :
			uiObjectItemViewControl(uiObjectItemView&);

    virtual uiToolBar* 	toolBar() { return toolbar_;}

protected:

    uiObjectItemView&	mainviewer_;
    uiToolBar*          toolbar_;
    uiToolButton*	manipdrawbut_;
    MouseCursor 	cursor_;
    bool		manip_;

    void 		stateCB(CallBacker*);
};



#endif
