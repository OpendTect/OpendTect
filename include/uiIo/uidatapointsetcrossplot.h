#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapointset.h"
#include "linear.h"
#include "odpair.h"
#include "rowcol.h"

#include "uiaxisdata.h"
#include "uiaxishandler.h"
#include "uidatapointset.h"
#include "uidpscrossplottools.h"
#include "uirgbarraycanvas.h"

class LinStats2D;
class MouseEvent;
class RowCol;

class uiColTabItem;
class uiDataPointSet;
class uiGraphicsItem;
class uiGraphicsItemGroup;
class uiLineItem;
class uiPolyLineItem;
class uiPolygonItem;
class uiRect;
class uiRectItem;
template <class T> class Array1D;
namespace Math { class Expression; }

/*!
\brief DataPointSet crossplotter.
*/

mExpClass(uiIo) uiDataPointSetCrossPlotter : public uiRGBArrayCanvas
{ mODTextTranslationClass(uiDataPointSetCrossPlotter);
public:

    struct Setup
    {
			Setup();

	mDefSetupMemb(bool,noedit)
	mDefSetupMemb(uiBorder,minborder)
	mDefSetupMemb(MarkerStyle2D,markerstyle) // None => uses drawPoint
	mDefSetupMemb(OD::LineStyle,xstyle)
	mDefSetupMemb(OD::LineStyle,ystyle)
	mDefSetupMemb(OD::LineStyle,y2style)
	mDefSetupMemb(bool,showy1cc)		// corr coefficient
	mDefSetupMemb(bool,showy2cc)		// corr coefficient
	mDefSetupMemb(bool,showy1regrline)
	mDefSetupMemb(bool,showy2regrline)
	mDefSetupMemb(bool,showy1userdefpolyline)
	mDefSetupMemb(bool,showy2userdefpolyline)
	mDefSetupMemb(OD::LineStyle,y1userdeflinestyle)
	mDefSetupMemb(OD::LineStyle,y2userdeflinestyle)
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
    Notifier<uiDataPointSetCrossPlotter>	mouseReleased;
    Notifier<uiDataPointSetCrossPlotter>	dataChgd;
    Notifier<uiDataPointSetCrossPlotter>	pointsSelected;
    Notifier<uiDataPointSetCrossPlotter>	removeRequest;
    Notifier<uiDataPointSetCrossPlotter>	selectionChanged;
    CNotifier<uiDataPointSetCrossPlotter,bool>	drawTypeChanged;
    CNotifier<uiDataPointSetCrossPlotter,Interval<float> > coltabRgChanged;
    DataPointSet::RowID	selRow() const		{ return selrow_; }
    bool		selRowIsY2() const	{ return selrowisy2_; }

    void		dataChanged();

    //!< Only use if you know what you're doing
    mExpClass(uiIo) AxisData :	public uiAxisData
    { mODTextTranslationClass(AxisData);
	public:
				AxisData(uiDataPointSetCrossPlotter&,
					 uiRect::Side);
				~AxisData();

	void			stop() override;
	void			setCol(DataPointSet::ColID);

	void			newColID();

	uiDataPointSetCrossPlotter& cp_;
	DataPointSet::ColID	colid_;
    };

    AxisData			x_;
    AxisData			y_;
    AxisData			y2_;
    int				getRow(const AxisData&,uiPoint) const;
    void			drawData(const AxisData&,bool y2,
					 bool rempts = false);
    void			drawRegrLine(uiAxisHandler*,
					     const Interval<int>&);

    void			prepareItems(bool y2);
    void			addItemIfNew(int itmidx,MarkerStyle2D&,
					uiGraphicsItemGroup*,uiAxisHandler&,
					uiDataPointSet::DRowID,bool);
    void			setItem(uiGraphicsItem*,bool y2,const uiPoint&);
    void			setAnnotEndTxt(uiAxisHandler&);
    void			drawDensityPlot();
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

    void			setMathObj(Math::Expression*);
    void			setMathObjStr( const char* str )
							{ mathobjstr_ = str; }
    const OD::String&		mathObjStr() const	{ return mathobjstr_; }
    Math::Expression*		mathObj() const		{ return mathobj_; }
    void			setModifiedColIds(
				    const TypeSet<uiDataPointSet::DColID>& ids )
				{ modcolidxs_ = ids; }
    const TypeSet<int>&		modifiedColIds() const	{ return modcolidxs_; }

    ConstRefMan<DataPointSet>	dps() const		{ return dps_; }
    RefMan<DataPointSet>	dps()			{ return dps_; }

    const uiDataPointSet&	uidps() const		{ return uidps_; }
    uiDataPointSet&		uidps()			{ return uidps_; }

    mDeprecatedObs
    const TypeSet<RowCol>&	getSelectedCells();
    const TypeSet<RowCol>&	getDPSSelectedCells()
				{ return dpsselrowcols_; }

    int				nrYSels() const		{ return selyitems_; }
    int				nrY2Sels() const	{ return sely2items_; }

    TypeSet<OD::Color>&		y1grpColors()		{ return y1grpcols_; }
    TypeSet<OD::Color>&		y2grpColors()		{ return y2grpcols_; }

    void			setColTab( const ColTab::Sequence& ctseq )
				{ ctab_ = ctseq; }
    void			setCTMapper(const ColTab::MapperSetup&);
    void			showY2(bool);
    void			drawContent( bool withaxis = true );
    void			drawColTabItem(bool isy1);
    bool			isY2Shown() const;
    bool			isY1Selectable() const
				{ return isy1selectable_; }
    bool			isY2Selectable() const
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
    int				selAreaSize() const;
    int				curGroup() const	{ return curgrp_; }
    void			reDrawSelections();
    TypeSet<OD::Color>		selGrpCols() const;
    void			setCurSelGrp(int grp)	{ curselgrp_ = grp; }
    int				curSelGrp() const	{ return curselgrp_; }

    int				getNewSelAreaID() const;
    bool			isSelAreaValid(int id) const;
    int				getSelGrpIdx(int selareaid) const;
    int				totalNrItems() const;
    void			getRandRowids();
    void			setMultiColMode(bool yn)
				{ multclron_ = yn; }
    bool			isMultiColMode() const	{ return multclron_; }
    bool			isSelectionValid(uiDataPointSet::DRowID);

    void			setNrBins(OD::Pair<int,int>);
    OD::Pair<int,int>		nrBins() const;

    void			setOverlayY1AttMapr(const ColTab::MapperSetup&);
    void			setOverlayY2AttMapr(const ColTab::MapperSetup&);
    void			setOverlayY1AttSeq(const ColTab::Sequence&);
    void			setOverlayY2AttSeq(const ColTab::Sequence&);

    void			setUserDefDrawType(bool dodrw,bool isy2,
							bool drwln = false);
    void			setUserDefPolyLine(TypeSet<uiWorldPoint>&,bool);
    void			drawUserDefPolyLine(bool);

    void			updateOverlayMapper(bool isy1);
    OD::Color			getOverlayColor(uiDataPointSet::DRowID,bool);

    int				y3Colid() const		{ return y3colid_; }
    int				y4Colid() const		{ return y4colid_; }
    const ColTab::Mapper&	y3Mapper() const	{ return y3mapper_; }
    const ColTab::Mapper&	y4Mapper() const	{ return y4mapper_; }
    const ColTab::Sequence&	y3CtSeq() const		{ return y3ctab_; }
    const ColTab::Sequence&	y4CtSeq() const		{ return y4ctab_; }

    float			getVal(int colid,int rid) const;

protected:

    int				y3colid_		= mUdf(int);
    int				y4colid_		= mUdf(int);

    uiDataPointSet&		uidps_;
    RefMan<DataPointSet>	dps_;
    Setup			setup_;
    Math::Expression*		mathobj_		= nullptr;
    BufferString		mathobjstr_;

    uiPolygonItem*		selectionpolygonitem_	= nullptr;
    uiRectItem*			selectionrectitem_	= nullptr;
    uiGraphicsItemGroup*	yptitems_		= nullptr;
    uiGraphicsItemGroup*	y2ptitems_		= nullptr;
    uiGraphicsItemGroup*	selrectitems_		= nullptr;
    uiGraphicsItemGroup*	selpolyitems_		= nullptr;
    uiLineItem*			y1regrlineitm_		= nullptr;
    uiLineItem*			y2regrlineitm_		= nullptr;
    uiPolyLineItem*		y1userdefpolylineitm_	= nullptr;
    uiPolyLineItem*		y2userdefpolylineitm_	= nullptr;
    uiColTabItem*		y1overlayctitem_	= nullptr;
    uiColTabItem*		y2overlayctitem_	= nullptr;

    ColTab::Sequence		ctab_;
    ColTab::Mapper		ctmapper_;
    ColTab::Sequence		y3ctab_;
    ColTab::Sequence		y4ctab_;
    ColTab::Mapper		y3mapper_;
    ColTab::Mapper		y4mapper_;
    LinStats2D&			lsy1_;
    LinStats2D&			lsy2_;
    bool			showy3_			= false;
    bool			showy4_			= false;
    bool			selectable_		= false;
    bool			mousepressed_		= false;
    bool			rectangleselection_	= true;
    bool			isdensityplot_		= false;
    bool			drawuserdefline_	= false;
    bool			drawy1userdefpolyline_	= false;
    bool			drawy2userdefpolyline_	= false;
    bool			drawy2_			= false;
    float			plotperc_		= 1;
    int				curgrp_			= 0;
    int				selyitems_		= 0;
    int				sely2items_		= 0;
    int				curselarea_		= 0;
    int				curselgrp_		= 0;
    OD::Pair<int,int>		nrbins_;
    const DataPointSet::ColID	mincolid_;
    DataPointSet::RowID		selrow_			= -1;
    Interval<int>		usedxpixrg_;
    mDeprecatedDef TypeSet<RowCol>	selrowcols_;
    TypeSet<RowCol>		dpsselrowcols_;
    TypeSet<OD::Color>		y1grpcols_;
    TypeSet<OD::Color>		y2grpcols_;
    TypeSet<uiWorldPoint>	y1userdefpts_;
    TypeSet<uiWorldPoint>	y2userdefpts_;
    Array1D<char>*		yrowidxs_		= nullptr;
    Array1D<char>*		y2rowidxs_		= nullptr;
    TypeSet<uiDataPointSet::DColID> modcolidxs_;
    bool			selrowisy2_		= false;
    uiPoint			startpos_;

    ObjectSet<SelectionArea>	selareaset_;
    ObjectSet<SelectionGrp>	selgrpset_;
    bool			isy1selectable_		= true;
    bool			isy2selectable_		= false;
    bool			multclron_		= false;

    void			initDraw();
    void			setDraw();
    void			calcStats();
    void			setWorldSelArea(int);
    void			reDrawSelArea();
    bool			drawRID(uiDataPointSet::DRowID,
					uiGraphicsItemGroup*,
					const AxisData&,bool y2,
					MarkerStyle2D&,int idmidx,bool rempt);

    bool			selNearest(const MouseEvent&);
    void			reDrawCB(CallBacker*);
    void			reSizeDrawCB(CallBacker*);
    void			mouseClickedCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    void			mouseReleasedCB(CallBacker*);
};
