#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "ranges.h"

/*!
\brief Fits values into a pre-defined range.
 
  The Squeezer has 2 ranges:
  * The limits
  * The 'untouched' limits

  No data will go outside the limits. If you only want a one-sided squeeze,
  pass an undefined value (not a very large number!).
  For good squeezing, you have to give the squeezer some 'work space'. You
  can do this by defining an 'untouched' range. In that range, output will be
  the same as input. In the zone outside that range, the Squeezer will return
  a value between the limit value and the input value, in such a way that the
  output will be nicely continuous - up to first derivative.

  For example:
  DataSqueezer<float> sq( Interval<float>(mUdf(float),10) );
  sq.setUntouchedRange( Interval<float>(mUdf(float),8) );
  will map values in range [8,Inf] on [8,10].

  By default, the Squeezer will behave like a 'Clipper'. You have to use
  setUntouchedRange() to get proper squeezing behavior.
*/

template <class T>
mClass(Algo) DataSqueezer
{
public:

    inline	DataSqueezer( const Interval<T>& l )
			: urg_(mUdf(T),mUdf(T))		{ setRange(l,true); }
    void	setUntouchedRange( const Interval<T>& r ) { setRange(r,false); }

    inline	DataSqueezer( T l0, T l1 ) : urg_(mUdf(T),mUdf(T))	
    					{ setRange(Interval<T>(l0,l1),true); }
    void	setUntouchedRange( T u0, T u1 )
    					{ setRange(Interval<T>(u0,u1),false); }

    T		value(T) const;

    const Interval<T>&	range( bool lim ) const		{ return lim?rg_:urg_; }
    void	setRange(const Interval<T>&,bool lim);

protected:

    Interval<T>	rg_;
    Interval<T>	urg_;
    bool	udfstart_, udfstop_;
    bool	udfustart_, udfustop_;

};


template <class T> 
inline void DataSqueezer<T>::setRange( const Interval<T>& inprg, bool lim )
{
    Interval<T>& rg = lim ? rg_ : urg_;
    rg = inprg;
    if ( !mIsUdf(rg.start_) && !mIsUdf(rg.stop_) )
	rg.sort(true);

    udfstart_ = mIsUdf(rg_.start_); udfstop_ = mIsUdf(rg_.stop_);
    udfustart_ = mIsUdf(urg_.start_); udfustop_ = mIsUdf(urg_.stop_);
    if ( !udfstart_ && !udfustart_ && urg_.start_ <= rg_.start_ )
    { urg_.start_ = mUdf(T); udfustart_ = true; }
    if ( !udfstop_ && !udfustop_ && urg_.stop_ >= rg_.stop_ )
    { urg_.stop_ = mUdf(T); udfustop_ = true; }
}


template <class T> 
inline T DataSqueezer<T>::value( T v ) const
{
    if ( mIsUdf(v) ) return v;

    if ( !udfstart_  )
    {
	if ( udfustart_ )
	{
	    if ( v < rg_.start_ )
		v = rg_.start_;
	}
	else if ( v < urg_.start_ )
	{
	    const T w = rg_.start_ - urg_.start_;
	    v = rg_.start_ - ((w*w) / (v + w - urg_.start_));
	}
    }
    if ( !udfstop_  )
    {
	if ( udfustop_ )
	{
	    if ( v > rg_.stop_ )
		v = rg_.stop_;
	}
	else if ( v > urg_.stop_ )
	{
	    const T w = rg_.stop_ - urg_.stop_;
	    v = rg_.stop_ - ((w*w) / (v + w - urg_.stop_));
	}
    }

    return v;
}
