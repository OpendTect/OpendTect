#ifndef axislayout_h
#define axislayout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jan 2005 / Dec 2009
 RCS:		$Id: axislayout.h,v 1.1 2009-12-22 14:48:10 cvsbert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "samplingdata.h"


/*!\brief helps making nice axes for graphs */

mClass AxisLayout
{
public:
    				// Have layout calculated
				AxisLayout( const Interval<float>& dr )
				    { setDataRange(dr); }
    void			setDataRange(const Interval<float>&);

    SamplingData<float>		sd;
    float			stop;

    				// Init with explicit layout
				AxisLayout( float start=0, float st_op=1,
					    float step=1 )
				    : sd(start,step), stop(st_op)	{}
				AxisLayout( const StepInterval<float>& rg )
				    : sd(rg.start,rg.step)
				    , stop(rg.stop)			{}

    				// Returns 'modulo' end with this sd and stop
    float			findEnd(float datastop) const;

};


#endif
