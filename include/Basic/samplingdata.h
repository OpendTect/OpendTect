#ifndef samplingdata_h
#define samplingdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 RCS:		$Id: samplingdata.h,v 1.3 2000-08-07 15:11:23 bert Exp $
________________________________________________________________________

-*/

#include <ranges.h>


class SamplingData
{
public:
		SamplingData( double sa=0, double se=1 )
		: start(sa), step(se)			{}
		SamplingData( const StepInterval<double>& intv )
		: start(intv.start), step(intv.step)	{}

    void	operator+=( const SamplingData& sd )
		{ start += sd.start; step += sd.step; }
    int		operator==( const SamplingData& sd ) const
		{ return start == sd.start && step == sd.step; }
    int		operator!=( const SamplingData& sd ) const
		{ return ! (sd == *this); }

    StepInterval<double> interval( int nrsamp ) const
		{ return StepInterval<double>(
			 start, start+(nrsamp-1)*step, step ); }
    int		getIndex( double val ) const
		{ return (int)((val-start) / step); }
    double	atIndex( int idx ) const
		{ return start + step * idx; }
    int		nearestIndex( double x ) const
		{ return interval(1).nearestIndex(x); }

    double	start;
    double	step;

};


#endif
