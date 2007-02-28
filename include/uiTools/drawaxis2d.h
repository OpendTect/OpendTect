#ifndef drawaxis2d_h
#define drawaxis2d_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Jan 2005
 RCS:           $Id: drawaxis2d.h,v 1.5 2007-02-28 22:30:36 cvskris Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "uigeom.h"

class ioDrawArea;
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

class DrawAxis2D
{
public:
			DrawAxis2D(ioDrawArea* = 0);
    void		setDrawArea(ioDrawArea*);
    void		setDrawRectangle(const uiRect*);
    			/*!<Specifies a rectangle on the canvas where the
			    axis should be drawn. If set to zero, drawer will
			    draw in the full draw area. */
    void		setup(const uiWorldRect&);
    void		setup(const StepInterval<float>& xrg,
	    		      const StepInterval<float>& yrg);

    void		annotInside( bool yn )	{ inside_ = yn; }
    void		drawAxisLine( bool yn )	{ drawaxisline_ = yn; }

    void		drawAxes(bool xdir,bool ydir,
	    			 bool topside,bool leftside) const;
    void		drawXAxis(bool topside) const;
    void		drawYAxis(bool leftside) const;
    void		drawGridLines(bool xdir,bool ydir) const;

private:
    uiRect		getDrawArea() const;

    ioDrawArea*		drawarea_;

    StepInterval<float>	xaxis_;
    StepInterval<float>	yaxis_;

    uiRect		uirect_;
    bool		useuirect_;

    bool		inside_;
    bool		drawaxisline_;
};


#endif
