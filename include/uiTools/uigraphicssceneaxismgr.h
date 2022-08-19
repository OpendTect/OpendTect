#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "draw.h"
#include "fontdata.h"
#include "ranges.h"
#include "samplingdata.h"
#include "uigeom.h"

class uiAxisHandler;
class uiBorder;
class uiFont;
class uiGraphicsItemGroup;
class uiGraphicsScene;
class uiGraphicsView;
class uiLineItem;
class uiRectItem;
class uiTextItem;

/*!\brief To draw simple axes for a 2D coordinate system.

  This class is designed for normal axis drawing and is not applicable to axes
  which are oblique to the X/Y axis of window. The area for drawing axis can be
  on the same canvas with the main graph area or on different canvas. Refer to
  uiCrossplot or uiDistributionMap for an example.

  Axis values are marked at every step. Grid lines can also be drawn at each
  step. Before calling the drawing routines, make sure the proper line pattern
  and pen color are set for the 'ioDrawTool' and there is enough room in the
  window to draw the axis and number annotation.

  USAGE:
  1) Call setupAxis() to set up the position for drawing axis and a pointer
  to uiWorld2Ui which provides the coordinate conversion functions. The axis
  range and step is calculated automatically from the class uiWorld2Ui.
  2) If the default range and step are not to be used for axis annotation,
  call setFixedDataRangeandStep() to override after calling setupAxis().
  3) Call the actual draw functions
*/


//Deprecated . Now FlatView Axis Drawing handled by uiAxisHandler
mExpClass(uiTools) uiGraphicsSceneAxis
{
public:
			~uiGraphicsSceneAxis();
			uiGraphicsSceneAxis(uiGraphicsScene&);

    void		setPosition(bool isx,bool istoporleft,bool isinside);
    void		setWorldCoords(const Interval<double>&);
    void		setViewRect(const uiRect&);

    void		setZValue(int nv);
    void		turnOn(bool);

    void		setAnnotInside(bool yn) { inside_ = yn; reDraw(); }
    void		enableAxisLine(bool yn) { drawaxisline_=yn; reDraw(); }
    void		enableGridLines(bool yn){ drawgridlines_=yn; reDraw(); }
    void		setAnnotInInt(bool yn)	{ annotinint_=yn; }

    void		enableMask(bool yn);
    void		setTextFactor(int n)	{ txtfactor_ = n; reDraw(); }
			// Values displayed along axis are multiplied by this
			// factor.
    void		setLineStyle(const OD::LineStyle& lst)
			{ ls_=lst; reDraw(); }
    void		setGridLineStyle(const OD::LineStyle& gls)
			{ gridls_ = gls; reDraw(); }
    void		setFontData(const FontData&);

protected:

    void			reDraw();
    void			drawAtPos(float worldpos, bool drawgrid,
					  int& curtextitm,int& curlineitm);
    int				getNrAnnotChars() const;

    bool			inside_;
    bool			isx_;
    bool			istop_;
    bool			drawaxisline_;
    bool			drawgridlines_;
    bool			annotinint_;

    uiGraphicsScene&		scene_;
    uiGraphicsItemGroup*	itmgrp_;
    uiRectItem*			mask_;

    uiRect			viewrect_;

    ObjectSet<uiLineItem>	lines_;
    ObjectSet<uiTextItem>	texts_;

    Interval<double>		rg_;

    OD::LineStyle		ls_;
    OD::LineStyle		gridls_;
    SamplingData<double>	sampling_;
    int				txtfactor_;
    FontData			fontdata_;
};


/*!
\brief Manages uiGraphicsSceneAxis.
*/

mExpClass(uiTools) uiGraphicsSceneAxisMgr : public CallBacker
{
public:
			uiGraphicsSceneAxisMgr(uiGraphicsView&);
    virtual		~uiGraphicsSceneAxisMgr();

    virtual void	setZValue(int z);
    virtual void	setWorldCoords(const uiWorldRect&);
    void		setWorldCoords(const StepInterval<float>& xrg,
				       const StepInterval<float>& yrg);
    virtual void	setViewRect(const uiRect&);

    uiAxisHandler*	axis(OD::Edge);
    const uiAxisHandler* axis(OD::Edge) const;
    void		enableAxis(OD::Edge,bool yn=true);
    void		enableXAxis(bool yn);
    void		enableYAxis(bool yn);

    int			getZValue() const;
    int			getNeededWidth() const;
    int			getNeededHeight() const;

    void		setAnnotInside(bool yn);
    void		enableAxisLine(bool yn);
    void		setBorder(const uiBorder&);
    void		setXLineStyle(const OD::LineStyle&);
    void		setYLineStyle(const OD::LineStyle&);
    void		setGridLineStyle(const OD::LineStyle&);
    void		setAuxLineStyle(const OD::LineStyle&,bool forx,
					bool forhl=false);
    void		setAnnotInInt(bool xaxis,bool dowant);
    void		showAuxPositions(bool forx,bool yn);
    void		setAuxAnnotPositions(const TypeSet<PlotAnnotation>&,
						bool forx);

    void		setMaskColor(const OD::Color&);
    OD::Color		getMaskColor() const;

    virtual void	updateScene();
    NotifierAccess&	layoutChanged();

protected:

    uiAxisHandler*		xaxis_top_;
    uiAxisHandler*		yaxis_left_;
    uiAxisHandler*		xaxis_bottom_;
    uiAxisHandler*		yaxis_right_;

    uiFont&			uifont_;
    uiGraphicsView&		view_;
    uiRectItem*			topmask_;
    uiRectItem*			bottommask_;
    uiRectItem*			leftmask_;
    uiRectItem*			rightmask_;

    OD::Color			maskcolor_;

    void			updateFontSizeCB(CallBacker*);
};
