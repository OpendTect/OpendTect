#ifndef uidatapointsetcrossplot_h
#define uidatapointsetcrossplot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiaxishandler.h"
#include "uidatapointset.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "datapointset.h"
#include "uirgbarraycanvas.h"
#include "uiaxisdata.h"
#include "rowcol.h"
#include "linear.h"
#include "uidpscrossplottools.h"

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
class uiPolyLineItem;
class uiRectItem;
class uiGraphicsItemGroup;
class uiGraphicsItem;
class uiColTabItem;
class uiRect;
template <class T> class Array1D;

/*!\brief Data Point Set Cross Plotter */


mClass(uiIo) uiDataPointSetCrossPlotter : public uiRGBArrayCanvas
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
	mDefSetupMemb(bool,showy1userdefpolyline)
	mDefSetupMemb(bool,showy2userdefpolyline)
    };

    			uiDataPointSetCrossPlotter(uiParent*,uiDataPointSet&,
						   const Setup&);
    			~uiDataPointSetCrossPlotter();

    const Setup&	setup() const		{ return setup_; }
    Setup&		setup()			{ return setup_; }

    void		setCols(DataPointSet::ColID x,
	    			DataPointSet::ColID y,
				DataPointSet::ColID y2);
    void		setOverlayY1Cols(DataPointSet::ColID y3);
    void		setOverlayY2Cols(DataPointSet::ColID y3);

    Notifier<uiDataPointSetCrossPlotter>	lineDrawn;
    Notifier<uiDataPointSetCrossPlotter>	pointsSelected;
    Notifier<uiDataPointSetCrossPlotter>	removeRequest;
    Notifier<uiDataPointSetCrossPlotter>	selectionChanged;
    CNotifier<uiDataPointSetCrossPlotter,bool>	drawTypeChanged;
    CNotifier<uiDataPointSetCrossPlotter,Interval<float> > coltabRgChanged;
    DataPointSet::RowID	selRow() const		{ return selrow_; }
    bool		selRowIsY2() const	{ return selrowisy2_; }

    void		dataChanged();

    //!< Only use if you know what you're doing
    mClass(uiIo) AxisData : 	public uiAxisData
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
	    				    bool removesel=false,
					    bool isy2=false, int areatyp=0,
					    Interval<int>* cellsz = 0,
					    Array2D<float>* freqdata = 0);
    int				calculateDensity(Array2D<float>*,
	    					 bool chgdps=false,
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
    void			removeSelections(bool relfrmselgrp = true);
    void			deleteSelections();
    float			getSelectedness(uiDataPointSet::DRowID,
	    					bool fory2);
    void			checkSelection(uiDataPointSet::DRowID,
				   uiGraphicsItem*,bool,const AxisData&,
				   bool rempt = false);
    bool			checkSelArea(const SelectionArea&) const;
    AxisData::AutoScalePars&	autoScalePars( int ax )	//!< 0=x 1=y 2=y2
				{ return axisData(ax).autoscalepars_; }
    uiAxisHandler*		axisHandler( int ax )	//!< 0=x 1=y 2=y2
				{ return axisData(ax).axis_; }
    const uiAxisHandler*	axisHandler( int ax ) const
				{ return axisData(ax).axis_; }
    const LinStats2D&		linStats( bool y1=true ) const
				{ return y1 ? lsy1_ : lsy2_; }

    AxisData&			axisData( int ax )
				{ return ax ? (ax == 2 ? y2_ : y_) : x_; }
    const AxisData&		axisData( int ax ) const
				{ return ax ? (ax == 2 ? y2_ : y_) : x_; }

    friend class		uiDataPointSetCrossPlotWin;
    friend class		AxisData;
    
    LinePars&			userdefy1lp_;
    LinePars&			userdefy2lp_;

    BufferString&		userdefy1str_;
    BufferString&		userdefy2str_;

    BufferString&		y1rmserr_;
    BufferString&		y2rmserr_;

    bool			y1clipping_;
    bool			y2clipping_;

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
    DataPointSet&		dps() 			{ return dps_; }

    const uiDataPointSet&	uidps() const		{ return uidps_; }
    uiDataPointSet&		uidps()			{ return uidps_; }
    
    const TypeSet<RowCol>&	getSelectedCells()	{ return selrowcols_; }

    int				nrYSels() const		{ return selyitems_; }
    int				nrY2Sels() const	{ return sely2items_; }

    TypeSet<Color>&		y1grpColors()		{ return y1grpcols_; }
    TypeSet<Color>&		y2grpColors()		{ return y2grpcols_; }

    void			setColTab( const ColTab::Sequence& ctseq )
				{ ctab_ = ctseq; }
    void			setCTMapper(const ColTab::MapperSetup&);
    void			showY2(bool);
    void 			drawContent( bool withaxis = true );
    void 			drawColTabItem(bool isy1);
    bool			isY2Shown() const;
    bool                        isY1Selectable() const
    				{ return isy1selectable_; }
    bool                        isY2Selectable() const
    				{ return isy2selectable_; }
    bool			showY3() const		{ return showy3_; }
    bool			showY4() const		{ return showy4_; }
    void			setShowY3(bool);
    void			setShowY4(bool);
    bool			isADensityPlot() const { return isdensityplot_;}
    void			setDensityPlot(bool yn,bool showy2);
    bool			isRectSelection() const	
    				{ return rectangleselection_; }
    void			setRectSelection( bool yn )
				{ rectangleselection_ = yn; }

    SelectionArea&		getCurSelArea();
    void			setSelArea(const SelectionArea&,int selgrpidx);
    bool			getSelArea(SelectionArea&,int selareaid);
    ObjectSet<SelectionGrp>&	selectionGrps()		{ return selgrpset_; }
    const ObjectSet<SelectionGrp>& selectionGrps() const
    				{ return selgrpset_; }
    int 			selAreaSize() const;
    void			reDrawSelections();
    TypeSet<Color>		selGrpCols() const;
    void			setCurSelGrp(int grp)	{ curselgrp_ = grp; }
    int				curSelGrp() const	{ return curselgrp_; }

    int 			getNewSelAreaID() const;
    bool			isSelAreaValid(int id) const;
    int				getSelGrpIdx(int selareaid) const;
    void			setTRMsg( const char* msg )
				{ trmsg_ = msg; }
    int				totalNrItems() const;
    void			getRandRowids();
    void			setMultiColMode(bool yn)
    				{ multclron_ = yn; }
    bool                        isMultiColMode() const	{ return multclron_; }
    void			setCellSize( int sz ) 	{ cellsize_ = sz; }
    int				cellSize() const	{ return cellsize_; }
    bool 			isSelectionValid(uiDataPointSet::DRowID);

    void			setOverlayY1AttMapr(const ColTab::MapperSetup&);
    void			setOverlayY2AttMapr(const ColTab::MapperSetup&);
    void			setOverlayY1AttSeq(const ColTab::Sequence&);
    void			setOverlayY2AttSeq(const ColTab::Sequence&);

    void			setUserDefDrawType(bool dodrw,bool isy2,
	    						bool drwln = false);
    void			setUserDefPolyLine(TypeSet<uiWorldPoint>&,bool);
    void			drawUserDefPolyLine(bool);

    void			updateOverlayMapper(bool isy1);
    Color			getOverlayColor(uiDataPointSet::DRowID,bool);
    
    int				y3Colid() const		{ return y3colid_; }
    int				y4Colid() const		{ return y4colid_; }
    const ColTab::Mapper&	y3Mapper() const	{ return y3mapper_; }
    const ColTab::Mapper&	y4Mapper() const	{ return y4mapper_; }
    const ColTab::Sequence&	y3CtSeq() const		{ return y3ctab_; }
    const ColTab::Sequence&	y4CtSeq() const		{ return y4ctab_; }

    float			getVal(int colid,int rid) const;

protected:

    int				y3colid_;
    int				y4colid_;

    uiDataPointSet&		uidps_;
    DataPointSet&		dps_;
    Setup			setup_;
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
    uiPolyLineItem*		y1userdefpolylineitm_;
    uiPolyLineItem*		y2userdefpolylineitm_;
    uiColTabItem*		y1overlayctitem_;
    uiColTabItem*		y2overlayctitem_;

    ColTab::Sequence		ctab_;
    ColTab::Mapper		ctmapper_;
    ColTab::Sequence		y3ctab_;
    ColTab::Sequence		y4ctab_;
    ColTab::Mapper		y3mapper_;
    ColTab::Mapper		y4mapper_;
    LinStats2D&			lsy1_;
    LinStats2D&			lsy2_;
    Timer&			timer_;
    bool			showy3_;
    bool			showy4_;
    bool			doy2_;
    bool			selectable_;
    bool			mousepressed_;
    bool			rectangleselection_;
    bool                        isdensityplot_;
    bool                        drawuserdefline_;
    bool			drawy1userdefpolyline_;
    bool                        drawy2userdefpolyline_;
    bool			drawy2_;
    float			plotperc_;
    int				curgrp_;
    int				selyitems_;
    int				sely2items_;
    int				curselarea_;
    int				curselgrp_;
    int				cellsize_;
    const DataPointSet::ColID	mincolid_;
    DataPointSet::RowID		selrow_;
    Interval<int>		usedxpixrg_;
    TypeSet<RowCol>		selrowcols_;
    TypeSet<Color>		y1grpcols_;
    TypeSet<Color>		y2grpcols_;
    TypeSet<uiWorldPoint>	y1userdefpts_;
    TypeSet<uiWorldPoint>	y2userdefpts_;
    Array1D<char>*		yrowidxs_;
    Array1D<char>*		y2rowidxs_;
    TypeSet<uiDataPointSet::DColID> modcolidxs_;
    bool			selrowisy2_;
    uiPoint			startpos_;

    ObjectSet<SelectionArea>	selareaset_;
    ObjectSet<SelectionGrp>	selgrpset_;
    bool                        isy1selectable_;
    bool                        isy2selectable_;
    bool                        multclron_;
 
    void 			initDraw();
    void 			setDraw();
    void 			calcStats();
    void			setWorldSelArea(int);
    void			reDrawSelArea();
    bool			drawRID(uiDataPointSet::DRowID,
	    				uiGraphicsItemGroup*,
	    				const AxisData&,bool y2,
	    				MarkerStyle2D&,int idmidx,bool rempt);

    bool			selNearest(const MouseEvent&);
    void 			reDraw(CallBacker*);
    void 			reSizeDraw(CallBacker*);
    void                        mouseClicked(CallBacker*);
    void                        mouseMove(CallBacker*);
    void                        mouseReleased(CallBacker*);
    void                        removeSelections(CallBacker*);
};

#endif

