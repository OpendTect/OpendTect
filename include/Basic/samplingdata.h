#ifndef samplingdata_h
#define samplingdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		23-10-1996
 RCS:		$Id: samplingdata.h,v 1.20 2012-07-24 18:41:30 cvskris Exp $
________________________________________________________________________

-*/

#include "ranges.h"


/*!\brief holds the fundamental sampling info: start and interval. */

template <class T>
class SamplingData
{
public:

    inline				SamplingData(T sa=0,T se=1);
    inline				SamplingData(T x0,T y0,T x1,T y1);
    template <class FT>	inline		SamplingData(const SamplingData<FT>&);
    template <class FT>	inline		SamplingData(const StepInterval<FT>&);

    inline bool				operator==(const SamplingData&)const;
    inline bool				operator!=(const SamplingData&)const;

    template <class IT> inline StepInterval<T> interval(IT nrsamples) const;
    template <class FT> inline float	getfIndex(FT) const;
    template <class FT> inline int	nearestIndex(FT) const;
    template <class IT> inline T	atIndex(IT) const;
    template <class FT> inline T	snap(FT) const;

    template <class FT>	inline void	set(FT,FT);
    template <class FT>	inline void	set(const SamplingData<FT>&);
    template <class FT>	inline void	set(const StepInterval<FT>&);
    inline void				scale(T);

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
    step = (y1-y0) / (x1-x0);
    start = y0 - step*x0;
}


template <class T> 
template <class FT> inline
SamplingData<T>::SamplingData( const SamplingData<FT>& sd )
    : start( sd.start ), step( sd.step )
{}


template <class T>
template <class FT> inline
SamplingData<T>::SamplingData( const StepInterval<FT>& intv )
    : start(intv.start), step(intv.step)
{}


template <class T> inline
bool SamplingData<T>::operator==( const SamplingData& sd ) const
{ return start == sd.start && step == sd.step; }

template <> inline
bool SamplingData<float>::operator==( const SamplingData<float>& sd ) const
{
    float val = start - sd.start;
    if ( !mIsZero(val,1e-6f) ) return false;
    val = 1 - (step / sd.step);
    return val < 1e-6f && val > -1e-6f;
}

template <> inline
bool SamplingData<double>::operator==( const SamplingData<double>& sd ) const
{
    double val = start - sd.start;
    if ( !mIsZero(val,1e-10) ) return false;
    val = 1 - (step / sd.step);
    return val < 1e-10 && val > -1e-10;
}


template <class T> inline
bool SamplingData<T>::operator!=( const SamplingData& sd ) const
{ return ! (sd == *this); }


template <class T>
template <class IT> inline
StepInterval<T> SamplingData<T>::interval( IT nrsamp ) const
{
    return nrsamp ? StepInterval<T>( start, start+(nrsamp-1)*step, step )
		  : StepInterval<T>( start, start, step ); }


template <class T>
template <class FT> inline
float SamplingData<T>::getfIndex( FT val ) const
{ return (val-start) / ((float)step); }


template <class T>
template <class FT> inline
int SamplingData<T>::nearestIndex( FT x ) const
{ const float fidx = getfIndex(x); return mNINT32(fidx); }


template <class T>
template <class FT> inline
T SamplingData<T>::snap( FT val ) const
{ return start + step * nearestIndex(val); }


template <class T>
template <class IT> inline
T SamplingData<T>::atIndex( IT idx ) const
{ return start + step * (T)idx; }

template <class T> 
template <class FT> inline
void SamplingData<T>::set( FT sa, FT se )
{ start = sa; step = se; }

template <class T> 
template <class FT> inline
void SamplingData<T>::set( const StepInterval<FT>& intv )
{ start = intv.start; step = intv.step; }

template <class T> 
template <class FT> inline
void SamplingData<T>::set( const SamplingData<FT>& sd )
{ start = sd.start; step = sd.step; }

template <class T> inline
void SamplingData<T>::scale( T scl )
{ start *= scl; step *= scl; }


#endif
