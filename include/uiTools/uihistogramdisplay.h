#ifndef uihistogramdisplay_h
#define uihistogramdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Duntao Wei
 Date:          Jan 2005
 RCS:           $Id: uihistogramdisplay.h,v 1.2 2007-01-31 14:34:40 cvskris Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "color.h"
#include "samplingdata.h"
#include "uiworld2ui.h"

class ioDrawArea;


/*!\brief
Paints a histogram in a ioDrawArea.
*/

class uiHistogramDisplay : public CallBacker
{
public:
    			uiHistogramDisplay(ioDrawArea*);
    			~uiHistogramDisplay();

    void		setHistogram(const TypeSet<float>&,
				 const SamplingData<float>& xaxis, bool copy);
    			/*!<If copy is false, the data is assumed to remain
			    in mem for the lifetime of this object or until
			    next setHistogram call. */
    void		touch();
    			/*!<Should be called if data has changed. */

    void		setTransform(const uiWorld2Ui& w2u);
    const uiWorld2Ui&	getTransform() const;

    void		setColor(const Color&);
    const Color&	getColor() const;

    bool		ignoresExtremes() const;
    void		setIgnoresExtremes(bool yn);

    void		reDraw(CallBacker* = 0);

protected:

    ioDrawArea*			drawarea_;
    TypeSet<float>		ownhistogram_;
    const TypeSet<float>*	histogram_;

    TypeSet<uiPoint>		pointlist_;

    SamplingData<float>		ownscale_;
    const SamplingData<float>*	scale_;

    uiWorld2Ui			transform_;
    Color			color_;
    bool			ignoreextremes_;
};

#endif
