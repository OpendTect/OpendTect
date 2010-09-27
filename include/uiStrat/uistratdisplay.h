#ifndef uistratdisplay_h
#define uistratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uistratdisplay.h,v 1.19 2010-09-27 11:05:19 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uigraphicsitem.h"
#include "uiaxishandler.h"
#include "uidialog.h"
#include "geometry.h"
#include "menuhandler.h"
#include "uistratdispdata.h"

class uiGenInput;
class uiGraphicsScene;
class uiLabeledSpinBox;
class uiMenuHandler;
class uiParent;
class uiPolygonItem;
class uiPolyLineItem;
class uiTextItem;
class uiPushButton;
class uiToolButton;
class uiStratViewControl; 
class MouseEvent;



mClass uiStratDrawer 
{
public:
				uiStratDrawer(uiGraphicsScene&,
						const StratDispData&);
				~uiStratDrawer();

    void			setZRange( StepInterval<float> rg ) 
    				{ yax_->setBounds(rg); draw(); }
    
    void			draw();
    void			setNewAxis(uiAxisHandler*,bool isx);
    
    uiAxisHandler* 		xAxis() 	{ return xax_; }
    uiAxisHandler* 		yAxis() 	{ return yax_; }
    const uiAxisHandler* 	xAxis() const	{ return xax_; }
    const uiAxisHandler* 	yAxis() const	{ return yax_; }

    mStruct ColumnItem
    {
				ColumnItem(const char* nm)
				    : name_(nm)
				    {}

	BufferString		name_;
	int			size_;
	int			pos_;
	
	uiPolyLineItem*		borderitm_;
	uiTextItem*		bordertxtitm_;
	ObjectSet<uiLineItem>	mrkitms_;
	ObjectSet<uiTextItem>	mrktxtitms_;
	ObjectSet<uiTextItem>	unittxtitms_;
	ObjectSet<uiPolygonItem> unititms_;
    };
    
    const ColumnItem&		colItem(int idx) const { return *colitms_[idx];}

protected:

    ObjectSet<ColumnItem>	colitms_;

    uiGraphicsScene&		scene_;
    uiAxisHandler* 		yax_; 
    uiAxisHandler* 		xax_; 
    
    //data
    const StratDispData&	data_;
    
    //graphics
    void			addUnit(float);
    void			drawColumns();
    void			drawBorders(ColumnItem&,int);
    void			drawMarkers(ColumnItem&,int);
    void			drawUnits(ColumnItem&,int);
    void			eraseAll();
    void			updateAxis(); 
};



mClass uiStratDisplay : public uiGraphicsView
{
public:
				uiStratDisplay(uiParent*,uiStratRefTree&);
				~uiStratDisplay(){};
    
    void			display(bool,bool shrk=false,bool max=false);
    void			setZRange(Interval<float>);
    void			setTree(Strat::RefTree&);
    
    void			addControl(uiToolBar*);
    uiStratViewControl*		control() 	{ return uicontrol_; }

protected :

    uiStratTreeToDispTransl*	uidatagather_;
    uiStratDispToTreeTransl	uidatawriter_;

    uiStratViewControl*		uicontrol_;
    StratDispData		data_;
    uiStratDrawer		drawer_;

    uiGenInput*                 rangefld_;
    uiLabeledSpinBox*           stepfld_;
    uiGroup*			dispparamgrp_;
    uiPushButton*		fillbutton_;
    uiPushButton*		viewcolbutton_;
    
    MenuItem            	assignlvlmnuitem_;

    Interval<float>		maxrg_;
   
    bool			isUnitBelowCurrent() const;
    void			createDispParamGrp();
    void			resetRangeFromUnits();

    void                	createMenuCB(CallBacker*);
    void                	handleMenuCB(CallBacker*);
    bool			handleUserClick(const MouseEvent&);
    void			controlRange(CallBacker*);
    void			dataChanged(CallBacker*);
    void			dispParamChgd(CallBacker*);
    void			selCols(CallBacker*);


    int				getColIdxFromPos() const; 
    StratDispData::Column*	getColFromPos() const; 
    const StratDispData::Unit* 	getUnitFromPos(bool nocolidx=false) const;
    Geom::Point2D<float> 	getPos() const;

    void			reSized(CallBacker*);
    void			usrClickCB(CallBacker*);
};


mClass uiStratViewControl : public CallBacker
{
public:

    struct Setup
    {
				Setup(Interval<float>& rg)
				    : maxrg_(rg)
				    {}

	mDefSetupMemb(uiToolBar*,tb)
	mDefSetupMemb(Interval<float>,maxrg)
    };

				uiStratViewControl(uiGraphicsView&,Setup&);
				~uiStratViewControl()
				{};

    void			setRange( Interval<float> rg )
    				{ range_ = rg; }	
    const Interval<float>&	range() const 	
    				{ return range_; }
    
    void			setSensitive(bool);

    Notifier<uiStratViewControl> rangeChanged;

protected:

    uiGraphicsView&		viewer_;
    uiToolButton*       	manipdrawbut_;
    uiToolButton*       	zoominbut_;
    uiToolButton*       	zoomoutbut_;
    uiToolBar*			tb_;

    Interval<float>		range_;
    Interval<float>		boundingrange_;
    float 			zoomfac_;

    float			startdragpos_;
    float			stopdragpos_;
    bool 			mousepressed_;
    bool 			viewdragged_;
    bool 			manip_;

    MouseEventHandler& 		mouseEventHandler();

    void			zoomCB(CallBacker*);
    void			handDragged(CallBacker* );
    void 			handDragStarted(CallBacker*);
    void			handDragging(CallBacker*);
    void			keyPressed(CallBacker*);
    void			stateCB(CallBacker*);
    void			wheelMoveCB(CallBacker*);
};

#endif
