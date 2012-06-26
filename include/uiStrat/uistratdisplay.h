#ifndef uistratdisplay_h
#define uistratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uistratdisplay.h,v 1.32 2012-06-26 07:36:10 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uigraphicsitem.h"
#include "uiaxishandler.h"
#include "uidialog.h"
#include "geometry.h"
#include "uistratdispdata.h"

class uiGenInput;
class uiGraphicsScene;
class uiLabeledSpinBox;
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
    				{ if ( yax_ ) yax_->setBounds(rg); }
    
    void			draw();
    void			drawColumns();
    void			setNewAxis(uiAxisHandler*,bool isx);
    
    uiAxisHandler* 		xAxis() 	{ return xax_; }
    uiAxisHandler* 		yAxis() 	{ return yax_; }
    const uiAxisHandler* 	xAxis() const	{ return xax_; }
    const uiAxisHandler* 	yAxis() const	{ return yax_; }

    mStruct ColumnItem
    {
				ColumnItem(const char* nm)
				    : name_(nm)
				    , borderitm_(0)
				    , bordertxtitm_(0)		{}

	BufferString		name_;
	int			size_;
	int			pos_;
	
	uiPolyLineItem*		borderitm_;
	uiTextItem*		bordertxtitm_;
	uiGraphicsItemSet	txtitms_;
	uiGraphicsItemSet	unititms_;
	uiGraphicsItemSet	lvlitms_;
    };
    
    const ColumnItem&		colItem( int idx ) const
    				{ return *colitms_[idx]; }

protected:

    ObjectSet<ColumnItem>	colitms_;
    uiTextItem*			emptyitm_;

    uiGraphicsScene&		scene_;
    uiAxisHandler* 		yax_; 
    uiAxisHandler* 		xax_; 
    
    //data
    const StratDispData&	data_;
    
    //graphics
    void			addUnit(float);
    void			drawBorders(ColumnItem&);
    void			drawLevels(ColumnItem&);
    void			drawUnits(ColumnItem&);
    void			drawEmptyText();
    void			eraseAll();
    void			initAxis();
    void			updateAxis(); 
};



mClass uiStratDisplay : public uiGraphicsView
{
public:
				uiStratDisplay(uiParent*,uiStratRefTree&);
				~uiStratDisplay();
    
    void			display(bool,bool shrk=false,bool max=false);
    void			setZRange(Interval<float>);
    
    void			addControl(uiToolBar*);
    uiStratViewControl*		control() 	{ return uicontrol_; }

    void			setTree();

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
    
    Interval<float>		maxrg_;
   
    void			createDispParamGrp();
    void			setRange();

    bool			handleUserClick(const MouseEvent&);

    int				getColIdxFromPos() const; 
    StratDispData::Column*	getColFromPos() const; 
    const StratDispData::Unit* 	getUnitFromPos() const;
    const StratDispData::Unit* 	getParentUnitFromPos() const;
    const StratDispData::Unit* 	getUnitFromPos(int colidx) const;
    const StratDispData::Level* getLevelFromPos() const;
    Geom::Point2D<float> 	getPos() const;

    void			controlRange(CallBacker*);
    void			dispParamChgd(CallBacker*);
    void			reDraw(CallBacker*);
    void			selCols(CallBacker*);
    void			usrClickCB(CallBacker*);
};


mClass uiStratViewControl : public CallBacker
{
public:

    struct Setup
    {
				Setup( const Interval<float>& rg )
				    : maxrg_(rg)
				    , tb_(0)		{}

	mDefSetupMemb(uiToolBar*,tb)
	mDefSetupMemb(Interval<float>,maxrg)
    };

				uiStratViewControl(uiGraphicsView&,Setup&);
				~uiStratViewControl()	{}

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
    bool 			mousepressed_;
    bool 			viewdragged_;
    bool 			manip_;

    MouseEventHandler& 		mouseEventHandler();

    void			zoomCB(CallBacker*);
    void			handDragged(CallBacker* );
    void 			handDragStarted(CallBacker*);
    void			handDragging(CallBacker*);
    void			keyPressed(CallBacker*);
    void			rubBandCB(CallBacker*);
    void			stateCB(CallBacker*);
    void			wheelMoveCB(CallBacker*);
};


#endif
