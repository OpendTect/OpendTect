#ifndef samplingdata_h
#define samplingdata_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 RCS:		$Id: samplingdata.h,v 1.2 2000-03-23 12:54:20 bert Exp $
________________________________________________________________________

@$*/

#include <gendefs.h>
#include <ranges.h>


class SamplingData {
public:
		SamplingData()
		{ start = 0; step = 0; }
		SamplingData( double sa, double se )
		{ start = sa; step = se; }
		SamplingData( StepInterval<double> intv )
		{ start = intv.start; step = intv.step; }

    void	operator+=( const SamplingData& sd )
		{ start += sd.start; step += sd.step; }
    int		operator==( const SamplingData& sd ) const
		{ return start == sd.start && step == sd.step; }
    int		operator!=( const SamplingData& sd ) const
		{ return ! (sd == *this); }

		operator StepInterval<double>() const
		{ return StepInterval<double>(start,mUndefValue,step); }
    Gate	gate( int nrsamp ) const
		{ return Gate( start, start+(nrsamp-1)*step ); }

    int		getIndex( double val ) const
		{ return (int)(( val  - start ) / step); }
    double	atIndex( int idx ) const
		{ return start + step * idx; }
    int		position( double x, double& frac ) const
		{
		    int nrbefore = getIndex(x);
		    double beforex = atIndex(nrbefore);   
		    frac = ( x - beforex ) / step;
		    return frac > .5 ? nrbefore + 1 : nrbefore;
		}

    double	start;
    double	step;
 
};


/*$-*/
#endif
