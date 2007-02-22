#ifndef drawaxis2d_h
#define drawaxis2d_h

#include "geometry.h"
class ioDrawTool;
class uiWorld2Ui;

/*+
  ________________________________________________________________________

   CopyRight:     (C) dGB Beheer B.V.
   Author:        Duntao Wei
   Date:          Jan 2005
   RCS:           $Id: drawaxis2d.h,v 1.2 2007-02-22 15:54:47 cvsbert Exp $
   ________________________________________________________________________

-*/

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
			DrawAxis2D();
			DrawAxis2D(const uiWorld2Ui*,const uiRect* rc=0);

			//! If the axis is drawn right at the position where
			//! X=Xmin/Xmax for Y axis and Y = Ymin/Ymax for X
			//! axis, just pass a NULL uiRect pointer to setup.
			//! Otherwise set uiRect for the axis drawing position.
			//! set left/right member for left/right Y axis position
			//! and set top/bottom member for the top/bottom x axis
			//! position.

    void		setupAxis(const uiWorld2Ui*,const uiRect* rc=0);
    void		setFixedDataRangeAndStep(float minx,float maxx,
					   	 float miny,float maxy,
						 float xstep,float ystep);

    void		drawAxes(ioDrawTool&,bool xdir,bool ydir,
	    				     bool topside,bool leftside) const;
    void		drawXAxis(ioDrawTool&,bool topside) const;
    void		drawYAxis(ioDrawTool&,bool leftside) const;
    void		drawGridLines(ioDrawTool&,bool xdir,bool ydir) const;

private:

    float		minx_;
    float		maxx_;
    float		miny_;
    float		maxy_;
    float		stepx_;
    float		stepy_;
    const uiWorld2Ui*	w2u_;

    Geom::Rectangle<int> axisrect_;
    bool		axislineposset_;
    const int		ticlen_;

};


#endif
