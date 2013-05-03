#ifndef drawaxis2d_h
#define drawaxis2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Duntao Wei
 Date:          Jan 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "draw.h"
#include "ranges.h"
#include "samplingdata.h"
#include "uigeom.h"
#include "uifont.h"

class uiGraphicsScene;
class uiGraphicsView;
class uiGraphicsItemGroup;
class uiRectItem;
class uiTextItem;
class uiLineItem;
template <class T> class StepInterval;

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
    
    void		drawAxisLine(bool yn) { drawaxisline_=yn; update(); }
    void		drawGridLines(bool yn) { drawgridlines_=yn; update(); }

    void		drawMask(bool yn);
    void		setTextFactor(int n) { txtfactor_ = n; update(); }
    void		setLineStyle(const LineStyle& lst) {ls_=lst; update(); }
    void		setGridLineStyle(const LineStyle& gls)
			{ gridls_ = gls; update(); }
    void		setFontData(const FontData&);
    
protected:
    
    void			update();
    
    bool			inside_;
    bool			isx_;
    bool			istop_;
    bool			drawaxisline_;
    bool			drawgridlines_;
    
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

    virtual void 	setZvalue(int z);
    virtual void	setViewRect(const uiRect&);
    void		setWorldCoords(const uiWorldRect&);
    void		setWorldCoords(const StepInterval<float>& xrg,
				       const StepInterval<float>& yrg);

    void		enableXAxis(bool yn) { xaxis_->turnOn(yn); }
    void		enableYAxis(bool yn) { yaxis_->turnOn(yn); }

    int 		getZvalue() const;
    int			getNeededWidth();
    int			getNeededHeight();

    void		annotInside( bool yn );
    void		drawAxisLine( bool yn );
    void		setXFactor(int);
    void		setYFactor(int);
    void		setXLineStyle(const LineStyle&);
    void		setYLineStyle(const LineStyle&);
    void		setGridLineStyle(const LineStyle&);

    NotifierAccess&	layoutChanged();

protected:
    
    
    // virtual double	getAnnotTextAndPos( bool isx, double proposedpos,
	//				    BufferString* text = 0) const;
    			/*!<When drawing the axis, the object proposes
			    an annotation at proposedpos. proposedpos may
			    however not be a good location (e.g. it may not
			    be on an even sample). Default is to display
			    at the proposedpos, with the proposedpos as the
			    text, but inheriting classes may customize this.
			    \param isx		true if x-axis, false if y-axis.
			    \param proposedpos	ideal place of annotation
			    \param text	where	the text to display
			    \returns 		the actual display pos is
			 */
    
    uiGraphicsSceneAxis*	xaxis_;
    uiGraphicsSceneAxis*	yaxis_;

    uiFont&			uifont_;
    uiGraphicsView&		view_;
    uiRectItem*			topmask_;
    uiRectItem*			bottommask_;
    uiRectItem*			leftmask_;
    uiRectItem*			rightmask_;
    void			updateFontSizeCB(CallBacker*);
};

#endif

