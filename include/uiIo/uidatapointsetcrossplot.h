#ifndef uidatapointsetcrossplot_h
#define uidatapointsetcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uidatapointsetcrossplot.h,v 1.25 2009-08-12 08:09:02 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiaxishandler.h"
#include "uidatapointset.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "datapointset.h"
#include "uirgbarraycanvas.h"
#include "uiaxisdata.h"
#include "rowcol.h"

class Coord;
class RowCol;
class uiComboBox;
class MathExpression;
class MouseEvent;
class LinStats2D;
class Timer;
class uiDataPointSet;
class uiPolygonItem;
class uiLineItem;
class uiRectItem;
class uiGraphicsItemGroup;
class uiGraphicsItem;
class uiRect;
template <class T> class ODPolygon;

/*!\brief Data Point Set Cross Plotter */

mClass uiDataPointSetCrossPlotter : public uiRGBArrayCanvas
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
	mDefSetupMemb(bool,showcc)		// corr coefficient
	mDefSetupMemb(bool,showregrline)
	mDefSetupMemb(bool,showy1userdefline)
	mDefSetupMemb(bool,showy2userdefline)
    };

    			uiDataPointSetCrossPlotter(uiParent*,uiDataPointSet&,
						   const Setup&);
    			~uiDataPointSetCrossPlotter();

    const Setup&	setup() const		{ return setup_; }
    Setup&		setup()			{ return setup_; }

    void		setCols(DataPointSet::ColID x,
	    			DataPointSet::ColID y,
				DataPointSet::ColID y2);

    Notifier<uiDataPointSetCrossPlotter>	pointsSelected;
    Notifier<uiDataPointSetCrossPlotter>	removeRequest;
    Notifier<uiDataPointSetCrossPlotter>	selectionChanged;
    CNotifier<uiDataPointSetCrossPlotter,bool>	drawTypeChanged;
    CNotifier<uiDataPointSetCrossPlotter,Interval<float> > coltabRgChanged;
    DataPointSet::RowID	selRow() const		{ return selrow_; }
    bool		selRowIsY2() const	{ return selrowisy2_; }

    void		dataChanged();

    //!< Only use if you know what you're doing
    mClass AxisData : 	public uiAxisData
    {
	public:
				AxisData(uiDataPointSetCrossPlotter&,
					 uiRect::Side);

	void			stop();
	void			setCol(DataPointSet::ColID);

	void			newColID();

	uiDataPointSetCrossPlotter& cp_;
	DataPointSet::ColID	colid_;
    };

    mStruct SelectionArea
    {
	enum SelectionType	{ Rectangle, Polygon };

	    			SelectionArea(uiRect*);
	    			SelectionArea(ODPolygon<int>*);
				~SelectionArea();

	SelectionType		type_;
	bool			isInside(const uiPoint&) const;
	bool			isValid() const;

	uiRect*			rect_;
	ODPolygon<int>*		poly_;
	uiWorldRect*		worldrect_;
	ODPolygon<double>*	worldpoly_;
    };

    AxisData			x_;
    AxisData			y_;
    AxisData			y2_;
    int				getRow(const AxisData&,uiPoint) const;
    void 			drawData(const AxisData&,bool y2,
	    				 bool rempts = false);
    void 			drawRegrLine(uiAxisHandler&,
	    				     const Interval<int>&);

    void			prepareItems(bool y2);
    void			addItemIfNew(int itmidx,MarkerStyle2D&,
	    				uiGraphicsItemGroup*,uiAxisHandler&,
					uiDataPointSet::DRowID,bool);
    void			setItem(uiGraphicsItem*,bool y2,const uiPoint&);
    void			setAnnotEndTxt(uiAxisHandler&);
    int				calcDensity(Array2D<float>*,bool chgdps=false,
	    				    bool removesel=false);
    void			drawDensityPlot(bool removesel=false);
    bool			drawPoints(uiGraphicsItemGroup*,
	    				   const AxisData&,bool y2,
	    				   MarkerStyle2D&,bool rempt = false);
    void			removeSelectionItems();

    void			setSceneSelectable( bool yn )	
				{ selectable_ = yn; }
    bool			isSceneSelectable() const	
    				{ return selectable_; }
    void			setSelectable( bool y1, bool y2 );
    void			removeSelections();
    void			deleteSelections();
    void			checkSelection(uiDataPointSet::DRowID,
				   uiGraphicsItem*,bool,const AxisData&,
				   bool rempt = false);
    AxisData::AutoScalePars&	autoScalePars( int ax )	//!< 0=x 1=y 2=y2
				{ return axisData(ax).autoscalepars_; }
    uiAxisHandler*		axisHandler( int ax )	//!< 0=x 1=y 2=y2
				{ return axisData(ax).axis_; }
    const LinStats2D&		linStats( bool y1=true ) const
				{ return y1 ? lsy1_ : lsy2_; }

    AxisData&			axisData( int ax )
				{ return ax ? (ax == 2 ? y2_ : y_) : x_; }

    friend class		uiDataPointSetCrossPlotWin;
    friend class		AxisData;
    
    LinePars&			userdefy1lp_;
    LinePars&			userdefy2lp_;
    
    void			setMathObj(MathExpression*);
    void			setMathObjStr(const BufferString& str )
				{ mathobjstr_ = str; }
    const BufferString&		mathObjStr() const	{ return mathobjstr_; }
    MathExpression*		mathObj() const		{ return mathobj_; }
    void			setModifiedColIds(
				    const TypeSet<uiDataPointSet::DColID>& ids )
				{ modcolidxs_ = ids; }
    const TypeSet<int>&		modifiedColIds() const 	{ return modcolidxs_; }

    const DataPointSet&		dps() const 		{ return dps_; }
    const TypeSet<RowCol>&	getSelectedCells()	{ return selrowcols_; }

    void			drawYUserDefLine(const Interval<int>&,bool draw,
	    					 bool isy1);

    int				nrYSels() const		{ return selyitems_; }
    int				nrY2Sels() const	{ return sely2items_; }

    void			setColTab( const ColTab::Sequence& ctseq )
				{ ctab_ = ctseq; }
    void			setCTMapper(const ColTab::MapperSetup&);
    void			showY2(bool);
    void 			drawContent( bool withaxis = true );
    bool			isY2Shown() const;
    bool			isADensityPlot() const { return isdensityplot_;}
    bool			isRectSelection() const	
    				{ return rectangleselection_; }
    void			setRectSelection( bool yn )
				{ rectangleselection_ = yn; }
    void			setSelectionAreas(
				    const ObjectSet<SelectionArea>&);
    void			setTRMsg( const char* msg )
				{ trmsg_ = msg; }
protected:

    uiDataPointSet&		uidps_;
    DataPointSet&		dps_;
    Setup			setup_;
    MouseEventHandler&		meh_;
    MathExpression*		mathobj_;
    BufferString		mathobjstr_;
    BufferString		trmsg_;
    uiPolygonItem*		selectionpolygonitem_;
    uiRectItem*			selectionrectitem_;
    uiGraphicsItemGroup*	yptitems_;
    uiGraphicsItemGroup*	y2ptitems_;
    uiGraphicsItemGroup*	selrectitems_;
    uiGraphicsItemGroup*	selpolyitems_;
    uiLineItem*			regrlineitm_;
    uiLineItem*			y1userdeflineitm_;
    uiLineItem*			y2userdeflineitm_;
    ColTab::Sequence		ctab_;
    ColTab::Mapper		ctmapper_;
    LinStats2D&			lsy1_;
    LinStats2D&			lsy2_;
    Timer&			timer_;
    bool			doy2_;
    bool			dobd_;
    bool			selectable_;
    bool			mousepressed_;
    bool			rectangleselection_;
    bool                        isdensityplot_;
    int				eachrow_;
    int				eachcount_;
    int				curgrp_;
    int				selyitems_;
    int				sely2items_;
    int				curselarea_;
    const DataPointSet::ColID	mincolid_;
    DataPointSet::RowID		selrow_;
    Interval<int>		usedxpixrg_;
    TypeSet<RowCol>		selrowcols_;
    TypeSet<int>		yrowidxs_;
    TypeSet<int>		y2rowidxs_;
    TypeSet<uiDataPointSet::DColID> modcolidxs_;
    bool			selrowisy2_;

    ObjectSet<SelectionArea>	selareaset_;
    bool                        isy1selectable_;
    bool                        isy2selectable_;
 
    void 			initDraw();
    void 			setDraw();
    virtual void		mkNewFill();
    void 			calcStats();
    bool 			isSelectionValid(uiDataPointSet::DRowID);
    void			setWorldSelArea(int);
    void			reDrawSelArea();

    bool			selNearest(const MouseEvent&);
    void 			reDraw(CallBacker*);
    void 			reSizeDraw(CallBacker*);
    void 			mouseClick(CallBacker*);
    void 			mouseRel(CallBacker*);
    void                        getSelStarPos(CallBacker*);
    void                        drawPolygon(CallBacker*);
    void                        itemsSelected(CallBacker*);
    void                        removeSelections(CallBacker*);
};
#endif
