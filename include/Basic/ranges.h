#ifndef ranges_H
#define ranges_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Ranges
 RCS:		$Id: ranges.h,v 1.5 2000-08-07 14:33:14 bert Exp $
________________________________________________________________________

-*/

#include <gendefs.h>


template <class T>
class Interval
{
public:
		Interval()			{ start = 0; stop = 0; }
		Interval( const T& t1, const T& t2 )
						{ start = t1; stop = t2; }

    int		operator==( const Interval<T>& i ) const
		{ return start == i.start && stop == i.stop; }
    int		operator!=( const Interval<T>& i ) const
		{ return ! (i == *this); }

    T		width() const
		{ return isRev() ? start - stop : stop - start; }
    void	shift( const T& len )
		{ start += len; stop += len; }
    void	widen( const T& len )
		{
		    if ( isRev() )
			{ start += len; stop -= len; }
		    else
			{ start -= len; stop += len; }
		}

    int		includes( const T& t ) const
		{ return isRev ? t>=stop && start>=t : t>=start && stop>=t; }
    void	include( const T& i )
		{
		    if ( isRev() )
			{ if ( stop>i ) stop=i; if ( start<i ) start=i; }
		    else
			{ if ( start>i ) start=i; if ( stop<i ) stop=i; }
		}
    void	include( const Interval<T>& i )
		{ include( i.start ); include( i.stop ); }

    T		atIndex( int idx, const T& step ) const
		{ return start + step * idx; }
    int		getIndex( const T& t, const T& step ) const
		{ return (int)(( t  - start ) / step); }
    int		nearestIndex( const T& x, const T& step ) const
		{
		    int nrbefore = getIndex(x,step);
		    T frac = ( x - atIndex(nrbefore,step) ) / step;
		    return frac > .5 ? nrbefore + 1 : nrbefore;
		}

    virtual void sort( bool asc=true )
		{
		    if ( (asc && stop<start) || (!asc && start<stop) )
			Swap(start,stop);
		}


    T		start;
    T		stop;

protected:

    inline bool	isRev() const		{ return start > stop; }

};


typedef Interval<int> SampleGate;
typedef Interval<double> TimeGate;
typedef Interval<double> DepthGate;


template <class T>
class StepInterval : public Interval<T>
{
public:
		StepInterval()			{ step = 1; }
		StepInterval( const T& t1, const T& t2, const T& t3 )
		: Interval<T>(t1,t2)		{ step = t3; }

    T		atIndex( int idx ) const
		{ return Interval<T>::atIndex(idx,step); }
    int		getIndex( const T& t ) const
		{ return getIndex( t, step ); }
    int		nearestIndex( const T& x ) const
		{ return nearestIndex( x, step ); }

    int		nrSteps() const
		{
		    if ( !step ) return 0;
		    T tnr = ( (start > stop ? start : stop)
			    - (start > stop ? stop : start) )
			    / (step > 0 ? step : -step);
		    return (int)(tnr + mEPSILON) + 1;
		}
    virtual void sort( bool asc=true )
		{
		    Interval<T>::sort(asc);
		    if ( (asc && step < 0) || (!asc && step > 0) )
			step = -step;
		}

     T		step;
};


#endif
