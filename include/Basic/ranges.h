#ifndef ranges_H
#define ranges_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Ranges
 RCS:		$Id: ranges.h,v 1.1.1.2 1999-09-16 09:19:08 arend Exp $
________________________________________________________________________

@$*/

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

    T		width() const			{ return stop - start; }
    int		includes( const T& t ) const	{ return t>=start && t<=stop; }
    int		getIndex( const T& t, const T& step ) const
		{ return (int)(( t  - start ) / step); }
    T		atIndex( int idx, const T& step ) const
		{ return start + step * idx; }
    virtual void sort( bool asc=YES )
		{
		    if ( (asc && stop<start) || (!asc && start<stop) )
			Swap(start,stop);
		}

    void	shift( const T& len )		{ start += len; stop += len; }
    void	widen( const T& len )		{ start -= len; stop += len; }
    void	include( const T& i )
		{ if ( start > i ) start = i; if ( stop < i ) stop = i; }
    void	include( const Interval<T>& i )
		{ include( i.start ); include( i.stop ); }

    T		start;
    T		stop;
};


typedef Interval<int> SampleGate;

class Gate: public Interval<double> {
public:
		Gate()				{}
		Gate( double t1, double t2 )
		: Interval<double>(t1,t2)	{}
    int		position( double x, double step, double& frac ) const
		{
		    int nrbefore = getIndex(x,step);
		    double beforex = atIndex(nrbefore,step);   
		    frac = ( x - beforex ) / step;
		    return frac > .5 ? nrbefore + 1 : nrbefore;
		}
};

typedef Gate TimeGate;
typedef Gate DepthGate;


template <class T>
class StepInterval : public Interval<T>
{
public:
		StepInterval()			{ step = 1; }
		StepInterval( const T& t1, const T& t2, const T& t3 )
		: Interval<T>( t1, t2 )		{ step = t3; }

    T		atIndex( int idx ) const
		{ return Interval<T>::atIndex(idx,step); }
    int		nrSteps() const
		{
		    if ( step == 0 ) return 0;
		    T cur = start > stop ? stop : start;
		    T last = start > stop ? start : stop;
		    T stp = step > 0 ? step : -step;
		    int nr = 0;
		    while ( cur-last < mEPSILON ) { cur += stp; nr++; }
		    return nr;
		}
    virtual void sort( bool asc=YES )
		{
		    Interval::sort(asc);
		    if ( (asc && step < 0) || (!asc && step > 0) )
			step = -step;
		}

     T		step;
};


#endif
