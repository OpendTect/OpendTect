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

#include "draw.h"
#include "ranges.h"
#include "samplingdata.h"
#include "uigeom.h"

class uiGraphicsScene;
class uiGraphicsView;
class uiGraphicsItemGroup;
class uiRectItem;
class uiLineItem;
template <class T> class StepInterval;

/*\!Draw simple axis for 2D axis.

    This class is designed for normal axis drawing and is not applicable to
    axises which are oblique to the X/Y axis of window. The area for drawing
    axis can be on the same canvas with the main graph area or on different 
    canvas. Refer to uiCrossplot or uiDistributionMap for an example.

    Axis values is marked at every step. Grid lines can also be drawn at 
    each step. Before calling the drawing routines, make sure the proper line
    pattern and pen color are set for the 'ioDrawTool' and there is enough
    room in the window to draw the axis and number annotation.
    
    USAGE:
    1) call setupAxis() to set up the position for drawing axis and a pointer
       to uiWorld2Ui which provides the corrdinate conversion functions.
       The axis range and step is calculated automatically from the class
       uiWorld2Ui.
    2) If the default range and step are not to be used for axis annotation,
       call setFixedDataRangeandStep() to override after calling setupAxis().
    3) Call the actual draw functions

 */

mClass DrawAxis2D
{
public:
			DrawAxis2D(uiGraphicsView&);
			~DrawAxis2D();

    void		setDrawRectangle(const uiRect*);
    			/*!<Specifies a rectangle on the canvas where the
			    axis should be drawn. If set to zero, drawer will
			    draw in the full draw area. */
    void		setup(const uiWorldRect&,float xfactor=1,
	    		      float yfactor=1);
    void		setup(const StepInterval<float>& xrg,
	    		      const StepInterval<float>& yrg);

    void		annotInside( bool yn )	{ inside_ = yn; }
    void		drawAxisLine( bool yn )	{ drawaxisline_ = yn; }

    void		drawAxes(bool xdir,bool ydir,
	    			 bool topside,bool leftside);
    void		drawXAxis(bool topside);
    void		drawYAxis(bool leftside);
    void		drawGridLines(bool xdir,bool ydir);

    void		setXLineStyle(const LineStyle&);
    void		setYLineStyle(const LineStyle&);
    void		setGridLineStyle(const LineStyle&);

    void		setZvalue( int z )	{ zValue_ = z; }

protected:

    uiRect		getDrawArea() const;
    virtual double	getAnnotTextAndPos( bool isx, double proposedpos,
					    BufferString* text = 0) const;
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

    uiGraphicsScene&	drawscene_;
    uiGraphicsView&	drawview_;
    uiGraphicsItemGroup* xaxlineitmgrp_;
    uiGraphicsItemGroup* yaxlineitmgrp_;
    uiGraphicsItemGroup* xaxgriditmgrp_;
    uiGraphicsItemGroup* yaxgriditmgrp_;
    uiGraphicsItemGroup* xaxtxtitmgrp_;
    uiGraphicsItemGroup* yaxtxtitmgrp_;
    uiRectItem*		xaxrectitem_;
    uiLineItem*		yaxlineitem_;

    Interval<double>	xrg_;
    Interval<double>	yrg_;

    LineStyle		xls_;
    LineStyle		yls_;
    LineStyle		gridls_;

    SamplingData<double> xaxis_;
    SamplingData<double> yaxis_;

    float		xfactor_;
    float		yfactor_;

    uiRect		uirect_;
    bool		useuirect_;

    bool		inside_;
    bool		drawaxisline_;
    int			zValue_;
};


#endif
