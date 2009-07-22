#ifndef samplingdata_h
#define samplingdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		23-10-1996
 RCS:		$Id: samplingdata.h,v 1.15 2009-07-22 16:01:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "ranges.h"


/*!\brief holds the fundamental sampling info: start and interval. */

template <class T>
class SamplingData
{
public:
    inline				SamplingData(T sa=0,T se=1);
    inline				SamplingData( T x0, T y0, T x1, T y1 );
    template <class FT>	inline		SamplingData(const SamplingData<FT>&);
    template <class FT>	inline		SamplingData(const StepInterval<FT>&);

    inline void				operator+=(const SamplingData& sd);
    inline int				operator==(const SamplingData& sd)const;
    inline int				operator!=(const SamplingData& sd)const;

    inline StepInterval<T>		interval(int nrsamp) const;
    template <class IT> inline float	getIndex(IT val) const;
    template <class IT> inline int	nearestIndex(IT x) const;
    template <class IT> inline T	atIndex(IT idx) const;
    template <class IT> inline T	snap(IT idx) const;

    T					start;
    T					step;
};


template <class T> inline
SamplingData<T>::SamplingData( T sa, T se )
    : start(sa), step(se)
{}


template <class T> inline
SamplingData<T>::SamplingData( T x0, T y0, T x1, T y1 )
{
    step = (y1-y0)/(x1-x0);
    start = y0-step*x0;
}


template <class T> 
template <class FT> inline
SamplingData<T>::SamplingData( const SamplingData<FT>& templ )
    : start( templ.start ), step( templ.step )
{}


template <class T>
template <class FT> inline
SamplingData<T>::SamplingData( const StepInterval<FT>& intv )
    : start(intv.start), step(intv.step)
{}


template <class T> inline
void SamplingData<T>::operator+=( const SamplingData& sd )
{ start += sd.start; step += sd.step; }


template <class T> inline
int SamplingData<T>::operator==( const SamplingData& sd ) const
{ return start == sd.start && step == sd.step; }


template <class T> inline
int SamplingData<T>::operator!=( const SamplingData& sd ) const
{ return ! (sd == *this); }


template <class T> inline
StepInterval<T> SamplingData<T>::interval( int nrsamp ) const
{ return StepInterval<T>( start, nrsamp ? start+(nrsamp-1)*step : 0, step); }


template <class T>
template <class IT> inline
float SamplingData<T>::getIndex( IT val ) const
{ return (val-start) / (float) step; }


template <class T>
template <class IT> inline
int SamplingData<T>::nearestIndex( IT x ) const
{ float fidx = getIndex(x); return mNINT(fidx); }


template <class T>
template <class IT> inline
T SamplingData<T>::snap( IT val ) const
{ return start + step * ((T) nearestIndex(val) ); }


template <class T>
template <class IT> inline
T SamplingData<T>::atIndex( IT idx ) const
{ return start + step * (T)idx; }


#endif
