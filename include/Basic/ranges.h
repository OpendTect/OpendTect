#ifndef ranges_H
#define ranges_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Ranges
 RCS:		$Id: ranges.h,v 1.26 2003-11-07 12:21:50 bert Exp $
________________________________________________________________________

-*/

#include "general.h"


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

    virtual Interval<T>* clone() const		
		{ return new Interval<T>( *this ); }

    inline int	operator==( const Interval<T>& i ) const
		{ return start == i.start && stop == i.stop; }
    inline int	operator!=( const Interval<T>& i ) const
		{ return ! (i == *this); }

    inline T	width( bool allowrev=true ) const
		{ return allowrev && isRev() ? start - stop : stop - start; }
    inline T	center() const
		{ return (start+stop)/2; }
    inline void	shift( const T& len )
		{ start += len; stop += len; }
    inline void	widen( const T& len, bool allowrev=true )
		{
		    if ( allowrev && isRev() )
			{ start += len; stop -= len; }
		    else
			{ start -= len; stop += len; }
		}

    template <class TT>
    inline bool	includes( const TT& t, bool allowrev=true ) const
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

    template <class X>
    inline int	getIndex( const X& t, const T& step ) const
		{ return (int)(( t  - start ) / step); }

    template <class X>
    inline X	limitValue( const X& t ) const
    		{
		    const bool isrev = isRev();
		    if ( (!isrev&&t>stop) || (isrev&&t<stop) ) return stop;
		    if ( (!isrev&&t<start) || (isrev&&t>start) ) return start;
		    return t;
		}
		

    template <class X>
    inline int	nearestIndex( const X& x, const T& step ) const
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


#define cloneTp	mPolyRet( Interval<T>, StepInterval<T> )

/*!\brief Interval with step. */

template <class T>
class StepInterval : public Interval<T>
{
public:
		StepInterval()			{ step = 1; }
		StepInterval( const T& t1, const T& t2, const T& t3 )
		: Interval<T>(t1,t2)		{ step = t3; }

    virtual cloneTp* clone() const	
		{ return new StepInterval<T>( *this ); }

    inline T	atIndex( int idx ) const
		{ return Interval<T>::atIndex(idx,step); }

    template <class X>
    inline int	getIndex( const X& t ) const
		{ return Interval<T>::getIndex( t, step ); }

    template <class X>
    inline int	nearestIndex( const X& x ) const
		{ return Interval<T>::nearestIndex( x, step ); }

    template <class X>
    inline T	snap( const X& t ) const
		{ return atIndex( nearestIndex( t ) ); }

    inline int	nrSteps() const;
    virtual void sort( bool asc=true )
		{
		    Interval<T>::sort(asc);
		    if ( (asc && step < 0) || (!asc && step > 0) )
			step = -step;
		}

    inline bool	isCompatible( const StepInterval<T>& b, T eps=mEPSILON) const;
    		//!< epsilon refers to the steps, i.e eps=0.1 allows b
    		//!< to be 0.1 steps apart.

     T		step;

};


#undef cloneTp


template <class T1,class T2>
inline void assign( Interval<T1>& i1, const Interval<T2>& i2 )
{ i1.start = (T1)i2.start; i1.stop = (T1)i2.stop; }

template <class T1,class T2>
inline void assign( StepInterval<T1>& i1, const StepInterval<T2>& i2 )
{ i1.start = (T1)i2.start; i1.stop = (T1)i2.stop; i1.step = (T1)i2.step; }


#define mDefIntNrSteps(typ) \
inline int StepInterval<typ>::nrSteps() const \
{ \
    if ( !step ) return 0; \
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

#define mDefFNrSteps(typ,eps) \
inline int StepInterval<typ>::nrSteps() const \
{ \
    if ( !step ) return 0; \
    typ ns = ( (start > stop ? start : stop) \
	    - (start > stop ? stop : start) ) \
	      / (step > 0 ? step : -step); \
    return (int)(ns * (1. + eps)); \
}

mDefFNrSteps(float,1e-4)
mDefFNrSteps(double,1e-8)

#define mDefIntisCompat(typ) \
inline bool StepInterval<typ>::isCompatible( const StepInterval<typ>& b, \
	typ ) const \
{ \
    if ( step!=b.step ) return false; \
\
    const typ diff = start - b.start; \
    return diff % step;	\
}

mDefIntisCompat(int)
mDefIntisCompat(long)
mDefIntisCompat(short)
mDefIntisCompat(char)
mDefIntisCompat(unsigned int)
mDefIntisCompat(unsigned long)
mDefIntisCompat(unsigned short)
mDefIntisCompat(unsigned char)

#define mDefFltisCompat(typ) \
inline bool StepInterval<typ>::isCompatible( const StepInterval<typ>& b, \
			typ eps ) const \
{ \
    if ( !mIS_ZERO(step-b.step) ) return false; \
 \
    typ nrsteps = (start - b.start) / step; \
    int nrstepsi = mNINT( nrsteps ); \
    typ diff = nrsteps - nrstepsi; \
    return ( (diff) < (eps) && (diff) > (-eps) ); \
}

mDefFltisCompat(float)
mDefFltisCompat(double)


#endif
