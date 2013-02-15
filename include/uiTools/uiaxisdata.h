#ifndef uiaxisdata_h
#define uiaxisdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          Jul 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uiaxishandler.h"
#include "statruncalc.h"

class DataClipper;
class uiGraphicsScene;

/*!
\brief Convenient base class to carry axis data:
  1) the AxisHandler which handles the behaviour and positioning of an axis
     in a 2D plot
  2) axis scaling parameters
  3) axis ranges
*/

mExpClass(uiTools) uiAxisData
{
public:

			uiAxisData(uiRect::Side);
			~uiAxisData();

    virtual void	stop();
    void		setRange( const Interval<float>& rg ) { rg_ = rg; }

    struct AutoScalePars
    {
			AutoScalePars();

	bool            doautoscale_;
	float           clipratio_;

	static float    defclipratio_;
	//!< 1) settings "AxisData.Clip Ratio"
	//!< 2) env "OD_DEFAULT_AXIS_CLIPRATIO"
	//!< 3) zero
    };

    uiAxisHandler*		axis_;
    AutoScalePars		autoscalepars_;
    Interval<float>		rg_;

    bool			needautoscale_;
    uiAxisHandler::Setup	defaxsu_;
    bool			isreset_;

    void			handleAutoScale(const Stats::RunCalc<float>&);
    void			handleAutoScale(const DataClipper&);
    void			newDevSize();
    void			renewAxis(const char*,uiGraphicsScene*,int w,
	    				  int h,const Interval<float>*);
};

#endif

