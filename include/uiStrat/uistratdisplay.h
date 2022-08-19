#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uidialog.h"
#include "uigraphicsview.h"

#include "uigraphicsitem.h"
#include "geometry.h"
#include "uistratdispdata.h"
#include "uistring.h"

class uiAxisHandler;
class uiGenInput;
class uiGraphicsScene;
class uiLabeledSpinBox;
class uiParent;
class uiPolygonItem;
class uiPolyLineItem;
class uiPushButton;
class uiStratViewControl;
class uiTextItem;
class uiToolButton;
class MouseEvent;



mExpClass(uiStrat) uiStratDrawer
{ mODTextTranslationClass(uiStratDrawer)
public:
				uiStratDrawer(uiGraphicsScene&,
						const StratDispData&);
				~uiStratDrawer();

    void			setZRange(const StepInterval<float>&);

    void			draw();
    void			drawColumns();
    void			setNewAxis(uiAxisHandler*,bool isx);

    uiAxisHandler*		xAxis()		{ return xax_; }
    uiAxisHandler*		yAxis()		{ return yax_; }
    const uiAxisHandler*	xAxis() const	{ return xax_; }
    const uiAxisHandler*	yAxis() const	{ return yax_; }

    mStruct(uiStrat) ColumnItem
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
    uiAxisHandler*		yax_;
    uiAxisHandler*		xax_;

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



mExpClass(uiStrat) uiStratDisplay : public uiGraphicsView
{ mODTextTranslationClass(uiStratDisplay)
public:
				uiStratDisplay(uiParent*,uiStratRefTree&);
				~uiStratDisplay();

    void			display(bool,bool shrk=false,bool max=false);
    void			setZRange(const Interval<float>&);

    void			addControl(uiToolBar*);
    uiStratViewControl*		control()	{ return uicontrol_; }

    void			setTree();

    void			setIsLocked( bool yn )	{ islocked_ = yn; }
    bool			isLocked() const	{ return islocked_; }

protected :

    uiStratTreeToDisp*		uidatagather_;
    uiStratDispToTree		uidatawriter_;

    uiStratViewControl*		uicontrol_;
    StratDispData		data_;
    uiStratDrawer		drawer_;

    uiGenInput*			rangefld_;
    uiLabeledSpinBox*		stepfld_;
    uiGroup*			dispparamgrp_;
    uiPushButton*		fillbutton_;
    uiPushButton*		viewcolbutton_;
    bool			islocked_;

    Interval<float>		maxrg_;

    void			createDispParamGrp();
    void			setRange();

    bool			handleUserClick(const MouseEvent&);

    int				getColIdxFromPos() const;
    StratDispData::Column*	getColFromPos() const;
    const StratDispData::Unit*	getUnitFromPos() const;
    const StratDispData::Unit*	getParentUnitFromPos() const;
    const StratDispData::Unit*	getUnitFromPos(int colidx) const;
    const StratDispData::Level* getLevelFromPos() const;
    Geom::Point2D<float>	getPos() const;

    void			controlRange(CallBacker*);
    void			dispParamChgd(CallBacker*);
    void			reDraw(CallBacker*);
    void			selCols(CallBacker*);
    void			usrClickCB(CallBacker*);
    void			doubleClickCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
};


mExpClass(uiStrat) uiStratViewControl : public CallBacker
{ mODTextTranslationClass(uiStratViewControl)
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
				~uiStratViewControl()	{detachAllNotifiers();}

    void			setRange( const Interval<float>& rg )
				{ range_ = rg; }
    const Interval<float>&	range() const
				{ return range_; }

    void			setSensitive(bool);

    Notifier<uiStratViewControl> rangeChanged;

protected:

    uiGraphicsView&		viewer_;
    uiToolButton*		rubbandzoombut_;
    uiToolButton*		vertzoominbut_;
    uiToolButton*		vertzoomoutbut_;
    uiToolButton*		cancelzoombut_;
    uiToolBar*			tb_;

    Interval<float>		range_;
    Interval<float>		boundingrange_;

    float			startdragpos_;
    bool			mousepressed_;

    MouseEventHandler&		mouseEventHandler();
    void			updatePosButtonStates();

    void			zoomCB(CallBacker*);
    void			cancelZoomCB(CallBacker*);
    void			handDragged(CallBacker* );
    void			handDragStarted(CallBacker*);
    void			handDragging(CallBacker*);
    void			keyPressed(CallBacker*);
    void			rubBandCB(CallBacker*);
    void			dragModeCB(CallBacker*);
    void			wheelMoveCB(CallBacker*);
};
