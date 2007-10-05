#ifndef uihistogramdisplay_h
#define uihistogramdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Jan 2005
 RCS:           $Id: uihistogramdisplay.h,v 1.4 2007-10-05 11:36:24 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "color.h"
#include "samplingdata.h"
#include "uiworld2ui.h"

class ioDrawArea;
class uiRect;


/*!\brief
Paints a histogram in a ioDrawArea.
*/

class uiHistogramDisplay : public CallBacker
{
public:
				uiHistogramDisplay(ioDrawArea*);
				~uiHistogramDisplay();

    void			setHistogram(const TypeSet<float>&,
				 	     const SamplingData<float>& xaxis);
    void			setBoundaryRect(const uiRect&);

    float			getXValue(int pixel) const;
    int				getPixel(float val) const;

    void			setColor(const Color&);
    const Color&		getColor() const;

    void			setIgnoresExtremes(bool yn);
    				//!<Tells the object to ignore first and last
				//!<histogram sample
    bool			ignoresExtremes() const;

    void			drawXAxis( const StepInterval<float>& );
    void			setXAxis( const StepInterval<float>& );
    void			reDraw(CallBacker* = 0);

protected:

    ioDrawArea*			drawarea_;
    TypeSet<float>		histogram_;
    uiRect			boundary_;

    TypeSet<uiPoint>		pointlist_;
    int				height_;
    int				width_;

    SamplingData<float>		scale_;

    Color			color_;
    bool			ignoreextremes_;

    StepInterval<float>         xrg_;
};

#endif
