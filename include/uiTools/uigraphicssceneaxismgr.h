#ifndef drawaxis2d_h
#define drawaxis2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Duntao Wei
 Date:          Jan 2005
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "draw.h"
#include "ranges.h"
#include "samplingdata.h"
#include "uigeom.h"
#include "uifont.h"
#include "uiaxishandler.h"

class uiGraphicsScene;
class uiGraphicsView;
class uiGraphicsItemGroup;
class uiRectItem;
class uiTextItem;
class uiLineItem;
class uiBorder;

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
    void                setViewRect(const uiRect&);
    
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
    void		setLineStyle(const LineStyle& lst) {ls_=lst; reDraw(); }
    void		setGridLineStyle(const LineStyle& gls)
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

    uiRect                      viewrect_;

    ObjectSet<uiLineItem>	lines_;
    ObjectSet<uiTextItem>	texts_;
    
    Interval<double>		rg_;
    
    LineStyle			ls_;
    LineStyle			gridls_;
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
    void		setViewRect(const uiRect&);

    void		enableXAxis(bool yn) { xaxis_->setVisible(yn); }
    void		enableYAxis(bool yn) { yaxis_->setVisible(yn); }

    int			getZValue() const;
    int			getNeededWidth() const;
    int			getNeededHeight() const;

    void		setAnnotInside(bool yn);
    void		enableAxisLine(bool yn);
    void		setBorder(const uiBorder&);
    void		setXLineStyle(const LineStyle&);
    void		setYLineStyle(const LineStyle&);
    void		setGridLineStyle(const LineStyle&);
    void		setAnnotInInt( bool xaxis, bool dowant )
			{ xaxis ? xaxis_->setup().annotinint(dowant)
				: yaxis_->setup().annotinint(dowant); }

    virtual void	updateScene()
			{ xaxis_->updateScene(); yaxis_->updateScene(); }
    NotifierAccess&	layoutChanged();

protected:
    
    uiAxisHandler*		xaxis_;
    uiAxisHandler*		yaxis_;

    uiFont&			uifont_;
    uiGraphicsView&		view_;
    uiRectItem*			topmask_;
    uiRectItem*			bottommask_;
    uiRectItem*			leftmask_;
    uiRectItem*			rightmask_;

    void			updateFontSizeCB(CallBacker*);
};

#endif

