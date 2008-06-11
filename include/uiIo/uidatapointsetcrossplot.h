#ifndef uidatapointsetcrossplot_h
#define uidatapointsetcrossplot_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplot.h,v 1.4 2008-06-11 13:35:49 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiaxishandler.h"
#include "uidatapointset.h"
#include "datapointset.h"
#include "uirgbarraycanvas.h"
class ioDrawTool;
class MouseEvent;
class uiDataPointSet;

/*!\brief Data Point Set Cross Plotter */

class uiDataPointSetCrossPlotter : public uiRGBArrayCanvas
{
public:

    struct Setup
    {
			Setup();

	mDefSetupMemb(bool,noedit)
	mDefSetupMemb(uiBorder,minborder)
	mDefSetupMemb(MarkerStyle2D,markerstyle) // None => uses drawPoint
	mDefSetupMemb(LineStyle,xstyle)
	mDefSetupMemb(LineStyle,ystyle)
	mDefSetupMemb(LineStyle,y2style)
    };

    			uiDataPointSetCrossPlotter(uiParent*,uiDataPointSet&,
						   const Setup&);
    			~uiDataPointSetCrossPlotter();

    const Setup&	setup() const		{ return setup_; }
    Setup&		setup()			{ return setup_; }

    void		setCols(DataPointSet::ColID x,
	    			DataPointSet::ColID y,
				DataPointSet::ColID y2);

    void		setRange(int ax,StepInterval<float>);


    Notifier<uiDataPointSetCrossPlotter>	selectionChanged;
    Notifier<uiDataPointSetCrossPlotter>	removeRequest;
    DataPointSet::RowID	selRow() const		{ return selrow_; }
    bool			isY2() const	{ return isy2_; }

    void			dataChanged();

protected:

    struct AxisData
    {
				AxisData(uiDataPointSetCrossPlotter&,
					 uiRect::Side);
				~AxisData();

	void			stop();
	void			start();
	void			setCol(DataPointSet::ColID);

	uiDataPointSetCrossPlotter& cp_;
	DataPointSet::ColID	colid_;
	uiAxisHandler*		axis_;
	uiAxisHandler::Setup	defsu_;
	StepInterval<float>	rg_;

	bool			needautoscale_;
	float			clipratio_;
	void			handleAutoScale();
	void			newDevSize();
    };

    uiDataPointSet&		uidps_;
    const DataPointSet&		dps_;
    Setup			setup_;
    MouseEventHandler&		meh_;
    AxisData			x_;
    AxisData			y_;
    AxisData			y2_;
    bool			doy2_;
    bool			dobd_;
    int				eachrow_;
    int				curgrp_;
    const DataPointSet::ColID	mincolid_;
    DataPointSet::RowID		selrow_;
    bool			isy2_;

    void 			initDraw(CallBacker*);
    virtual void		mkNewFill();
    void 			drawContent(CallBacker*);

    int				getRow(const AxisData&,uiPoint) const;

    bool			selNearest(const MouseEvent&);
    void 			mouseClick(CallBacker*);
    void 			mouseRel(CallBacker*);

    void 			drawData(const AxisData&);

    friend class		uiDataPointSetCrossPlotWin;
    friend class		AxisData;
};


#endif
