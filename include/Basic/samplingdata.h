#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"

/*!
\brief Holds the fundamental sampling info: start and interval.
*/

template <class T>
mClass(Basic) SamplingData
{
public:

    inline				SamplingData(T sa=0,T se=1);
    inline				SamplingData(T x0,T y0,T x1,T y1);
    template <class FT>	inline		SamplingData(const SamplingData<FT>&);
    template <class FT>	inline		SamplingData(const StepInterval<FT>&);

    inline SamplingData<T>&		operator=(const SamplingData<T>&);
    inline bool				operator==(const SamplingData&)const;
    inline bool				operator!=(const SamplingData&)const;

    template <class IT> inline StepInterval<T> interval(IT nrsamples) const;
    template <class FT> inline float	getfIndex(FT) const;
    template <class FT> inline int	nrSteps(FT) const;
    template <class FT> inline int	nearestIndex(FT) const;
    template <class FT> inline int	indexOnOrAfter(FT x,
						    float eps=mDefEps ) const;
					/*!<
					\param x get index for value x
					\param eps is in number of samples. */
    template <class IT> inline T	atIndex(IT) const;
    template <class FT> inline T	snap(FT) const;

    template <class FT>	inline void	set(FT,FT);
    template <class FT>	inline void	set(const SamplingData<FT>&);
    template <class FT>	inline void	set(const StepInterval<FT>&);
    inline void				scale(T);
    inline bool				isUdf() const;

    T					start_;
    T					step_;
    mDeprecated("Use with underscore")
    T&					start;
    mDeprecated("Use with underscore")
    T&					step;
};


mStartAllowDeprecatedSection

template <class T> inline
SamplingData<T>::SamplingData( T sa, T se )
    : start_(sa)
    , step_(se)
    , start(start_)
    , step(step_)
{}


template <class T> inline
SamplingData<T>::SamplingData( T x0, T y0, T x1, T y1 )
    : start(start_)
    , step(step_)
{
    step_ = (y1-y0) / (x1-x0);
    start_ = y0 - step_*x0;
}


template <class T>
template <class FT> inline
SamplingData<T>::SamplingData( const SamplingData<FT>& sd )
    : start_( mCast(T,sd.start_) )
    , step_( mCast(T,sd.step_) )
    , start(start_)
    , step(step_)
{}


#include "ranges.h"

template <class T>
template <class FT> inline
SamplingData<T>::SamplingData( const StepInterval<FT>& intv )
    : start_(mCast(T,intv.start_))
    , step_(mCast(T,intv.step_))
    , start(start_)
    , step(step_)
{}

mStopAllowDeprecatedSection


template <class T> inline
SamplingData<T>& SamplingData<T>::operator=( const SamplingData<T>& sd )
{
    start_ = sd.start_;
    step_ = sd.step_;
    return *this;
}


template <class T> inline
bool SamplingData<T>::operator==( const SamplingData& sd ) const
{
    return start_ == sd.start_ && step_ == sd.step_;
}


template <> inline
bool SamplingData<float>::operator==( const SamplingData<float>& sd ) const
{
    float val = start_ - sd.start_;
    if ( !mIsZero(val,1e-6f) ) return false;
    val = 1 - (step_ / sd.step_);
    return val < 1e-6f && val > -1e-6f;
}


template <> inline
bool SamplingData<double>::operator==( const SamplingData<double>& sd ) const
{
    double val = start_ - sd.start_;
    if ( !mIsZero(val,1e-10) ) return false;
    val = 1 - (step_ / sd.step_);
    return val < 1e-10 && val > -1e-10;
}


template <class T> inline
bool SamplingData<T>::operator!=( const SamplingData& sd ) const
{ return ! (sd == *this); }


template <class T>
template <class IT> inline
StepInterval<T> SamplingData<T>::interval( IT nrsamp ) const
{
    return nrsamp ? StepInterval<T>( start_, start_+(nrsamp-1)*step_, step_ )
		  : StepInterval<T>( start_, start_, step_ );
}


template <>
template <> inline
float SamplingData<float>::getfIndex( float val ) const
{ return mIsZero(step_,mDefEps) ? 0.f : val/step_ - start_/step_; }


template <class T>
template <class FT> inline
float SamplingData<T>::getfIndex( FT val ) const
{
    return mIsZero(step_,mDefEps) ? 0.f
				  : sCast(float,(double(val) - start_)/step_ );
}


template <class T>
template <class FT> inline
int SamplingData<T>::nrSteps( FT x ) const
{ const float fidx = getfIndex(x); return (int)(fidx + 1e-6f); }


template <class T>
template <class FT> inline
int SamplingData<T>::nearestIndex( FT x ) const
{ const float fidx = getfIndex(x); return mNINT32(fidx); }


template <class T>
template <class FT> inline
int SamplingData<T>::indexOnOrAfter( FT x, float eps ) const
{
    float fres = getfIndex( x );
    int res = (int) getfIndex(x);
    const float diff = fres-res;
    if ( diff>eps )
	res++;

    return res;
}


template <class T>
template <class FT> inline
T SamplingData<T>::snap( FT val ) const
{ return start_ + step_ * nearestIndex(val); }


template <class T>
template <class IT> inline
T SamplingData<T>::atIndex( IT idx ) const
{ return start_ + step_ * (T)idx; }


template <class T>
template <class FT> inline
void SamplingData<T>::set( FT sa, FT se )
{ start_ = (T)sa; step_ = (T)se; }


template <class T>
template <class FT> inline
void SamplingData<T>::set( const StepInterval<FT>& intv )
{ start_ = (T)intv.start_; step_ = (T)intv.step_; }


template <class T>
template <class FT> inline
void SamplingData<T>::set( const SamplingData<FT>& sd )
{ start_ = (T)sd.start_; step_ = (T)sd.step_; }


template <class T> inline
void SamplingData<T>::scale( T scl )
{ start_ *= scl; step_ *= scl; }


template <class T> inline
bool SamplingData<T>::isUdf() const
{ return mIsUdf(start_) || mIsUdf(step_); }
