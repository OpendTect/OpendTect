#ifndef squeezing_h
#define squeezing_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2010
 RCS:		$Id$
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
  setUntouchedRange() to get proper squeezing behaviour.
*/

template <class T>
class DataSqueezer
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
    if ( !mIsUdf(rg.start) && !mIsUdf(rg.stop) )
	rg.sort(true);

    udfstart_ = mIsUdf(rg_.start); udfstop_ = mIsUdf(rg_.stop);
    udfustart_ = mIsUdf(urg_.start); udfustop_ = mIsUdf(urg_.stop);
    if ( !udfstart_ && !udfustart_ && urg_.start <= rg_.start )
	{ urg_.start = mUdf(T); udfustart_ = true; }
    if ( !udfstop_ && !udfustop_ && urg_.stop >= rg_.stop )
	{ urg_.stop = mUdf(T); udfustop_ = true; }
}


template <class T> 
inline T DataSqueezer<T>::value( T v ) const
{
    if ( mIsUdf(v) ) return v;

    if ( !udfstart_  )
    {
	if ( udfustart_ )
	{
	    if ( v < rg_.start )
		v = rg_.start;
	}
	else if ( v < urg_.start )
	{
	    const T w = rg_.start - urg_.start;
	    v = rg_.start - ((w*w) / (v + w - urg_.start));
	}
    }
    if ( !udfstop_  )
    {
	if ( udfustop_ )
	{
	    if ( v > rg_.stop )
		v = rg_.stop;
	}
	else if ( v > urg_.stop )
	{
	    const T w = rg_.stop - urg_.stop;
	    v = rg_.stop - ((w*w) / (v + w - urg_.stop));
	}
    }

    return v;
}



#endif

