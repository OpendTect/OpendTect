#ifndef ranges_H
#define ranges_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Ranges
 RCS:		$Id: ranges.h,v 1.12 2001-07-02 12:42:03 bert Exp $
________________________________________________________________________

-*/

#include <general.h>


/*!\brief interval of values.

Note that start does not need to be lower than stop. That's why there is a
sort() method.

*/

template <class T>
class Interval
{
public:
		Interval()			{ start = 0; stop = 0; }
		Interval( const T& t1, const T& t2 )
						{ start = t1; stop = t2; }

    inline int	operator==( const Interval<T>& i ) const
		{ return start == i.start && stop == i.stop; }
    inline int	operator!=( const Interval<T>& i ) const
		{ return ! (i == *this); }

    inline T	width( bool allowrev=true ) const
		{ return allowrev && isRev() ? start - stop : stop - start; }
    inline void	shift( const T& len )
		{ start += len; stop += len; }
    inline void	widen( const T& len, bool allowrev=true )
		{
		    if ( allowrev && isRev() )
			{ start += len; stop -= len; }
		    else
			{ start -= len; stop += len; }
		}

    inline int	includes( const T& t, bool allowrev=true ) const
		{
		    return allowrev && isRev()
			? t>=stop && start>=t
			: t>=start && stop>=t;
		}
    inline void	include( const T& i, bool allowrev=true )
		{
		    if ( allowrev && isRev() )
			{ if ( stop>i ) stop=i; if ( start<i ) start=i; }
		    else
			{ if ( start>i ) start=i; if ( stop<i ) stop=i; }
		}
    inline void	include( const Interval<T>& i, bool allowrev=true )
		{ include( i.start, allowrev ); include( i.stop, allowrev ); }

    inline T	atIndex( int idx, const T& step ) const
		{ return start + step * idx; }
    inline int	getIndex( const T& t, const T& step ) const
		{ return (int)(( t  - start ) / step); }
    int         nearestIndex( const T& x, const T& step ) const
		{
		    int nr = getIndex(x,step);
		    return step < 0
			 ? ( (x-atIndex(nr,step)) < step*.5 ? nr - 1 : nr )
			 : ( (x-atIndex(nr,step)) > step*.5 ? nr + 1 : nr );
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


/*!\brief Interval with step. */

template <class T>
class StepInterval : public Interval<T>
{
public:
		StepInterval()			{ step = 1; }
		StepInterval( const T& t1, const T& t2, const T& t3 )
		: Interval<T>(t1,t2)		{ step = t3; }

    inline T	atIndex( int idx ) const
		{ return Interval<T>::atIndex(idx,step); }
    inline int	getIndex( const T& t ) const
		{ return getIndex( t, step ); }
    inline int	nearestIndex( const T& x ) const
		{ return Interval<T>::nearestIndex( x, step ); }

    inline int	nrSteps() const
		{
		    if ( !step ) return 0;
		    double ns = ( (start > stop ? start : stop)
				- (start > stop ? stop : start) )
			      / (step > 0 ? step : -step);
		    return (int)(ns * (1. + 1e-10));
		}
    virtual void sort( bool asc=true )
		{
		    Interval<T>::sort(asc);
		    if ( (asc && step < 0) || (!asc && step > 0) )
			step = -step;
		}

     T		step;

};


template <class T1,class T2>
inline void assign( Interval<T1>& i1, const Interval<T2>& i2 )
{ i1.start = (T1)i2.start; i1.stop = (T1)i2.stop; }


template <class T1,class T2>
inline void assign( StepInterval<T1>& i1, const StepInterval<T2>& i2 )
{ i1.start = (T1)i2.start; i1.stop = (T1)i2.stop; i1.step = (T1)i2.step; }

#define mDefIntNrSteps(typ) \
inline int StepInterval<typ>::nrSteps() const \
{ \
    int ret = ((int)start - stop) / step; \
    return ret < 0 ? -ret : ret; \
}

mDefIntNrSteps(int)
mDefIntNrSteps(long)
mDefIntNrSteps(short)
mDefIntNrSteps(char)
mDefIntNrSteps(unsigned int)
mDefIntNrSteps(unsigned long)
mDefIntNrSteps(unsigned short)
mDefIntNrSteps(unsigned char)


#endif
