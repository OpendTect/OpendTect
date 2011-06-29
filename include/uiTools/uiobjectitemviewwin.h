#ifndef uiobjectitemviewwin_h
#define uiobjectitemviewwin_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
 RCS:           $Id: uiobjectitemviewwin.h,v 1.8 2011-06-29 11:16:11 cvsbruno Exp $
________________________________________________________________________

-*/


/*! brief A uiMainWin that holds embedded uiObjects and controls !*/

#include "uimainwin.h"
#include "uigroup.h"
#include "uiobjectitemview.h"

class uiCheckBox;
class uiObjectItem;
class uiObjectItemView;
class uiObjectItemViewInfoBar;
class uiObjectItemViewAxisPainter;
class uiObjectItemViewControl;
class uiSliderExtra;
class uiToolBar;
class uiToolButton;
class uiGraphicsObjectScene;
class uiAxisHandler;
class uiBorder;

mClass uiObjectItemViewWin : public uiMainWin
{
public:    

    mClass Setup
    {
    public:

			Setup(const char* title)
			    : wintitle_(title)
			    , startwidth_(400)
			    , startheight_(600)
			    , infoheight_(50)
			    , layoutpos_(0,0)		     
			    {} 

	 mDefSetupMemb(BufferString,wintitle)
	 mDefSetupMemb(int,startwidth)
	 mDefSetupMemb(int,startheight)
	 mDefSetupMemb(int,infoheight)
	 mDefSetupMemb(uiPoint,layoutpos)
    };

			uiObjectItemViewWin(uiParent*,const Setup&);


    uiObjectItemView*	mainViewer() 	{ return mainviewer_; }

    void 		addObject(uiObject* grp,uiObject* infogrp=0);
    void 		addGroup(uiGroup* grp,uiGroup* infogrp=0);

    void		removeAllItems();

    virtual void 	fillPar(IOPar&) const;
    virtual void 	usePar(const IOPar&); 

    static const char*  sKeyHZoomVal() 	{ return "Horizontal Zoom Value"; }
    static const char*  sKeyVZoomVal() 	{ return "Vertical Zoom Value"; }

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
    virtual void	reSizeItems();

    void 		addItem(uiObjectItem* itm,uiObjectItem* infoitm=0);
    void		insertItem(int idx,uiObjectItem*,uiObjectItem* info=0);

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
    void   		insertItem(uiObjectItem*,uiObjectItem* cplitm,int idx);
    virtual void	insertItem(uiObjectItem* itm,int pos,int st=1)
			{ return uiObjectItemView::insertItem(itm,pos,st); }

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

    void  	        changeStatus();

protected:

    uiObjectItemView&	mainviewer_;
    uiToolBar*          toolbar_;
    uiToolButton*	manipdrawbut_;
    MouseCursor 	cursor_;
    bool		manip_;

    void        	setToolButtons();
    void 		stateCB(CallBacker*);
};


mClass uiObjectItemViewAxisPainter : public CallBacker
{
public:
			uiObjectItemViewAxisPainter(uiObjectItemView&);

    void 		setZRange(Interval<float>);
    uiAxisHandler*	getAxis() const 	{ return zax_; }

protected:	

    uiObjectItemView&	viewer_;
    uiGraphicsObjectScene* scene_;
    uiAxisHandler* 	zax_;
    uiBorder 		border_;

    void 		setAxisRelations();
    void 		plotAxis(CallBacker*);
};


#endif
