#ifndef statruncalc_h
#define statruncalc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl (org) / Bert Bril (rev)
 Date:          10-12-1999 / Sep 2006
 RCS:           $Id: statruncalc.h,v 1.17 2009-07-22 16:01:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "convert.h"
#include "math2.h"
#include "stattype.h"
#include "sorting.h"
#include "sets.h"


#define mUndefReplacement 0

namespace Stats
{


/*!\brief setup for the Stats::RunCalc object */

mClass RunCalcSetup
{
public:
    			RunCalcSetup( bool weighted=false )
			    : weighted_(weighted)
			    , needextreme_(false)
			    , needsums_(false)
			    , needmed_(false)
			    , needmostfreq_(false)	{}

    RunCalcSetup&	require(Type);

    static bool		medianEvenAverage(); //!< Tied to OD_EVEN_MEDIAN_AVERAGE
    			//!< If medianing over an even number of points,
    			//!< either take the high mid, or avg the two middles
    			//!< default is false: no averaging

protected:

    template <class T>
    friend class	RunCalc;
    template <class T>
    friend class	WindowedCalc;

    bool		weighted_;
    bool		needextreme_;
    bool		needsums_;
    bool		needmed_;
    bool		needmostfreq_;

};


/*!\brief calculates mean, min, max, etc. as running values.

The idea is that you simply add values and ask for a stat whenever needed.
The clear() method resets the object and makes it able to work with new data.

Adding values can be doing with weight (addValue) or without (operator +=).
You can remove a value; for Min or Max this has no effect as this would
require buffering all data.

The mostFrequent assumes the data contains integer classes. Then the class
that is found most often will be the output. Weighting, again, assumes integer
values. Beware that if you pass data that is not really class-data, the
memory consumption can become large (and the result will be rather
uninteresting).

The variance won't take the decreasing degrees of freedom into consideration
when weights are provided.


The object is ready to use with int, float and double types. If other types
are needed, you may need to specialise an isZero function for each new type.

-*/


template <class T>
class RunCalc
{
public:

			RunCalc( const RunCalcSetup& s )
			    : setup_(s)		{ clear(); }
    inline void		clear();
    const RunCalcSetup&	setup() const		{ return setup_; }
    bool		isWeighted() const	{ return setup_.weighted_; }

    inline RunCalc<T>&	addValue(T data,T weight=1);
    inline RunCalc<T>&	addValues(int sz,const T* data,const T* weights=0);
    inline RunCalc<T>&	replaceValue(T olddata,T newdata,T wt=1);
    inline RunCalc<T>&	removeValue(T data,T weight=1);

    inline double	getValue(Type) const;
    inline int		getIndex(Type) const; //!< only for Median, Min and Max

    inline RunCalc<T>&	operator +=( T t )	{ return addValue(t); }
    inline bool		hasUndefs() const	{ return nrused_ != nradded_; }
    inline int		size( bool used=true ) const
    			{ return used ? nrused_ : nradded_; }
    inline bool		isEmpty() const		{ return size() == 0; }

    inline int		count() const		{ return nrused_; }
    inline double	average() const;
    inline double	variance() const; 
    inline double	normvariance() const;
    inline T		mostFreq() const;
    inline T		sum() const;
    inline T		min(int* index_of_min=0) const;
    inline T		max(int* index_of_max=0) const;
    inline T		median(int* index_of_median=0) const;
    inline T		sqSum() const;
    inline double	rms() const;
    inline double	stdDev() const;

    inline T		clipVal(float ratio,bool upper) const;
    			//!< require median; 0 <= ratio <= 1

    TypeSet<T>		vals_;

protected:

    RunCalcSetup	setup_;

    int			nradded_;
    int			nrused_;
    int			minidx_;
    int			maxidx_;
    T			minval_;
    T			maxval_;
    T			sum_x;
    T			sum_xx;
    T			sum_w;
    T			sum_wx;
    T			sum_wxx;
    TypeSet<int>	clss_;
    TypeSet<int>	occs_;

    inline bool		isZero( const T& t ) const	{ return !t; }

    mutable int		curmedidx_;
    inline bool		findMedIdx(T) const;
    inline bool		addExceptMed(T,T);
    inline bool		remExceptMed(T,T);

};

template <>
inline bool RunCalc<float>::isZero( const float& val ) const
{ return mIsZero(val,1e-6); }


template <>
inline bool RunCalc<double>::isZero( const double& val ) const
{ return mIsZero(val,1e-10); }


/*!\brief RunCalc manager which buffers a part of the data.
 
  Allows calculating running stats on a window only. Once the window is full,
  WindowedCalc will replace the first value added (fifo).
 
 */

template <class T>
class WindowedCalc
{
public:
			WindowedCalc( const RunCalcSetup& rcs, int sz )
			    : calc_(rcs)
			    , sz_(sz)
			    , wts_(calc_.isWeighted() ? new T [sz] : 0)
			    , vals_(new T [sz])	{ clear(); }
			~WindowedCalc()	
				{ delete [] vals_; delete [] wts_; }
    inline void		clear();

    inline WindowedCalc& addValue(T data,T weight=1);
    inline WindowedCalc& operator +=( T t )	{ return addValue(t); }

    inline int		getIndex(Type) const;
    			//!< Only use for Min, Max or Median
    inline double	getValue(Type) const;

#   define			mRunCalcDefEqFn(ret,fn) \
    inline ret			fn() const	{ return calc_.fn(); }
    mRunCalcDefEqFn(int,	count)
    mRunCalcDefEqFn(double,	average)
    mRunCalcDefEqFn(double,	variance)
    mRunCalcDefEqFn(double,	normvariance)
    mRunCalcDefEqFn(T,		mostFreq)
    mRunCalcDefEqFn(T,		sum)
    mRunCalcDefEqFn(T,		sqSum)
    mRunCalcDefEqFn(double,	rms)
    mRunCalcDefEqFn(double,	stdDev)
#   undef			mRunCalcDefEqFn
    inline T		median( int* i=0 ) const { return calc_.median(i); }
    inline T		min(int* i=0) const;
    inline T		max(int* i=0) const;

protected:

    RunCalc<T>	calc_; // has to be before wts_ (see constructor inits)
    const int	sz_;
    T*		wts_;
    T*		vals_;
    int		posidx_;
    bool	full_;

    inline void	fillCalc(RunCalc<T>&) const;
};



template <class T> inline
void RunCalc<T>::clear()
{
    sum_x = sum_w = sum_xx = sum_wx = sum_wxx = 0;
    nradded_ = nrused_ = minidx_ = maxidx_ = curmedidx_ = 0;
    clss_.erase(); occs_.erase(); vals_.erase();
}


template <class T> inline
bool RunCalc<T>::addExceptMed( T val, T wt )
{
    nradded_++;
    if ( mIsUdf(val) )
	return false;
    if ( setup_.weighted_ && (mIsUdf(wt) || isZero(wt)) )
	return false;

    if ( setup_.needextreme_ )
    {
	if ( nrused_ == 0 )
	    minval_ = maxval_ = val;
	else
	{
	    if ( val < minval_ ) { minval_ = val; minidx_ = nradded_ - 1; }
	    if ( val > maxval_ ) { maxval_ = val; maxidx_ = nradded_ - 1; }
	}
    }

    if ( setup_.needmostfreq_ )
    {
	int ival; Conv::set( ival, val );
	int setidx = clss_.indexOf( ival );
	int iwt = 1;
	if ( setup_.weighted_ )
	    Conv::set( iwt, wt );

	if ( setidx < 0 )
	    { clss_ += ival; occs_ += iwt; }
	else
	    occs_[setidx] += iwt;
    }

    if ( setup_.needsums_ )
    {
	sum_x += val;
	sum_xx += val * val;
	if ( setup_.weighted_ )
	{
	    sum_w += wt;
	    sum_wx += wt * val;
	    sum_wxx += wt * val * val;
	}
    }

    nrused_++;
    return true;
}


template <class T> inline
bool RunCalc<T>::remExceptMed( T val, T wt )
{
    nradded_--;
    if ( mIsUdf(val) )
	return false;
    if ( setup_.weighted_ && (mIsUdf(wt) || isZero(wt)) )
	return false;

    if ( setup_.needmostfreq_ )
    {
	int ival; Conv::set( ival, val );
	int setidx = clss_.indexOf( ival );
	int iwt = 1;
	if ( setup_.weighted_ )
	    Conv::set( iwt, wt );

	if ( setidx >= 0 )
	{
	    occs_[setidx] -= iwt;
	    if ( occs_[setidx] <= 0 )
	    {
		clss_.remove( setidx );
		occs_.remove( setidx );
	    }
	}
    }

    if ( setup_.needsums_ )
    {
	sum_x -= val;
	sum_xx -= val * val;
	if ( setup_.weighted_ )
	{
	    sum_w -= wt;
	    sum_wx -= wt * val;
	    sum_wxx -= wt * val * val;
	}
    }

    nrused_--;
    return true;
}


template <class T> inline
RunCalc<T>& RunCalc<T>::addValue( T val, T wt )
{
    if ( !addExceptMed(val,wt) || !setup_.needmed_ )
	return *this;

    int iwt = 1;
    if ( setup_.weighted_ )
	Conv::set( iwt, wt );
    for ( int idx=0; idx<iwt; idx++ )
	vals_ += val;

    return *this;
}


template <class T> inline
RunCalc<T>& RunCalc<T>::addValues( int sz, const T* data, const T* weights )
{
    for ( int idx=0; idx<sz; idx++ )
	addValue( data[idx], weights ? weights[idx] : 1 );
    return *this;
}


template <class T> inline
bool RunCalc<T>::findMedIdx( T val ) const
{
    if ( curmedidx_ >= vals_.size() )
	curmedidx_ = 0;

    if ( vals_[curmedidx_] != val ) // oh well, need to search anyway
    {
	curmedidx_ = vals_.indexOf( val );
	if ( curmedidx_ < 0 )
	    { curmedidx_ = 0; return false; }
    }

    return true;
}


template <class T> inline
RunCalc<T>& RunCalc<T>::replaceValue( T oldval, T newval, T wt )
{
    remExceptMed( oldval, wt );
    addExceptMed( newval, wt );
    if ( !setup_.needmed_ || vals_.isEmpty() )
	return *this;

    int iwt = 1;
    if ( setup_.weighted_ )
	Conv::set( iwt, wt );

    const bool newisudf = mIsUdf(newval);
    for ( int idx=0; idx<iwt; idx++ )
    {
	if ( !findMedIdx(oldval) )
	    break;

	if ( newisudf )
	    vals_.remove( curmedidx_ );
	else
	    vals_[curmedidx_] = newval;
	curmedidx_++;
    }

    return *this;
}


template <class T> inline
RunCalc<T>& RunCalc<T>::removeValue( T val, T wt )
{
    remExceptMed( val, wt );
    if ( !setup_.needmed_ || vals_.isEmpty() )
	return *this;

    int iwt = 1;
    if ( setup_.weighted_ )
	Conv::set( iwt, wt );

    for ( int idx=0; idx<iwt; idx++ )
    {
	if ( findMedIdx(val) )
	    vals_.remove( curmedidx_ );
    }

    return *this;
}


template <class T> inline
double RunCalc<T>::getValue( Stats::Type t ) const
{
    switch ( t )
    {
	case Count:		return count();
	case Average:		return average();
	case StdDev:		return stdDev();
	case Variance:		return variance();
	case NormVariance:	return normvariance();			
	case Min:		return min();
	case Max:		return max();
	case MostFreq:		return mostFreq();
	case Sum:		return sum();
	case SqSum:		return sqSum();
	case RMS:		return rms();
	case Median:		return median();
    }

    return 0;
}


template <class T> inline
int RunCalc<T>::getIndex( Type t ) const
{
    int ret;
    switch ( t )
    {
	case Min:		min( &ret );	break;
	case Max:		max( &ret );	break;
	case Median:		median( &ret );	break;
	default:		ret = 0;	break;
    }
    return ret;
}


#undef mChkEmpty
#define mChkEmpty(typ) \
    if ( nrused_ < 1 ) return mUdf(typ);

template <class T>
inline double RunCalc<T>::stdDev() const
{
    mChkEmpty(double);

    double v = variance();
    return v > 0 ? Math::Sqrt( v ) : 0;
}


template <class T>
inline double RunCalc<T>::average() const
{
    mChkEmpty(double);

    if ( !setup_.weighted_ )
	return ((double)sum_x) / nrused_;

    return isZero(sum_w) ? mUdf(double) : ((double)sum_wx) / sum_w;
}


template <class T>
inline T RunCalc<T>::mostFreq() const
{
    if ( clss_.isEmpty() )
	return mUdf(T);

    int maxocc = occs_[0]; int ret = clss_[0];
    for ( int idx=1; idx<clss_.size(); idx++ )
    {
	if ( occs_[idx] > maxocc )
	    { maxocc = occs_[idx]; ret = clss_[idx]; }
    }

    return ret;
}


template <class T>
inline T RunCalc<T>::median( int* idx_of_med ) const
{
    if ( idx_of_med ) *idx_of_med = 0;
    const int sz = vals_.size();
    if ( sz < 3 )
	return sz < 1 ? mUdf(T) : vals_[0];

    const int mididx = sz / 2;
    T* valarr = const_cast<T*>( vals_.arr() );
    if ( !idx_of_med )
	quickSort( valarr, sz );
    else
    {
	mGetIdxArr(int,idxs,sz)
	quickSort( valarr, idxs, sz );
	*idx_of_med = idxs[ mididx ];
	delete [] idxs;
    }

    if ( !(sz%2) && setup_.medianEvenAverage() )
	return (vals_[mididx] + vals_[mididx-1]) / 2;

    return vals_[ mididx ];
}


template <class T>
inline T RunCalc<T>::sum() const
{
    mChkEmpty(T);
    return sum_x;
}



template <class T>
inline T RunCalc<T>::sqSum() const
{
    mChkEmpty(T);
    return sum_xx;
}


template <class T>
inline double RunCalc<T>::rms() const
{
    mChkEmpty(double);

    if ( !setup_.weighted_ )
	return Math::Sqrt( ((double)sum_xx) / nrused_ );

    return isZero(sum_w) ? mUdf(double) : Math::Sqrt( ((double)sum_wxx)/sum_w );
}


template <class T>
inline double RunCalc<T>::variance() const 
{
    if ( nrused_ < 2 ) return 0;

    if ( !setup_.weighted_ )
	return ( sum_xx - (sum_x * ((double)sum_x) / nrused_) )
	     / (nrused_ - 1);

    return (sum_wxx - (sum_wx * ((double)sum_wx) / sum_w) )
	 / ( (nrused_-1) * ((double)sum_w) / nrused_ );
}


template <class T>
inline double RunCalc<T>::normvariance() const
{
    if ( nrused_ < 2 ) return 0;

    double fact = 0.1;
    double avg = average();
    double var = variance();
    return var / (avg*avg + fact*var);
}


template <class T>
inline T RunCalc<T>::min( int* index_of_min ) const
{
    if ( index_of_min ) *index_of_min = minidx_;

    mChkEmpty(T);

    return minval_;
}


template <class T>
inline T RunCalc<T>::max( int* index_of_max ) const
{
    if ( index_of_max ) *index_of_max = maxidx_;

    mChkEmpty(T);

    return maxval_;
}


template <class T>
inline T RunCalc<T>::clipVal( float ratio, bool upper ) const
{
    mChkEmpty(T);
    (void)median();
    const int lastidx = vals_.size();
    const float fidx = ratio * lastidx;
    const int idx = fidx <= 0 ? 0 : (fidx > lastidx ? lastidx : (int)fidx);
    return vals_[upper ? lastidx - idx : idx];
}


template <class T> inline
void WindowedCalc<T>::clear()
{
    posidx_ = 0; full_ = false;
    calc_.clear();
}


template <class T> inline
void WindowedCalc<T>::fillCalc( RunCalc<T>& calc ) const
{
    const int lastidx = full_ ? sz_ - 1 : posidx_;
    for ( int idx=posidx_+1; idx<=lastidx; idx++ )
	calc.addValue( vals_[idx], wts_ ? wts_[idx] : 1 );
    for ( int idx=0; idx<=posidx_; idx++ )
	calc.addValue( vals_[idx], wts_ ? wts_[idx] : 1 );
}


template <class T> inline
double WindowedCalc<T>::getValue( Type t ) const
{
    if ( t == Min )
	return min();
    else if ( t == Max )
	return max();

    return calc_.getValue( t );
}


template <class T> inline
int WindowedCalc<T>::getIndex( Type t ) const
{
    if ( t == Min )
	{ int ret; min( &ret ); return ret; }
    else if ( t == Max )
	{ int ret; max( &ret ); return ret; }

    return calc_.getIndex( t );
}


template <class T> inline
T WindowedCalc<T>::min( int* index_of_min ) const
{
    RunCalc<T> calc( RunCalcSetup().require(Stats::Min) );
    fillCalc( calc );
    return calc.min( index_of_min );
}


template <class T> inline
T WindowedCalc<T>::max( int* index_of_max ) const
{
    RunCalc<T> calc( RunCalcSetup().require(Stats::Max) );
    fillCalc( calc );
    return calc.max( index_of_max );
}


template <class T>
inline WindowedCalc<T>&	WindowedCalc<T>::addValue( T val, T wt )
{
    if ( !full_ || (calc_.vals_.isEmpty() && calc_.setup().needmed_ )
	    	|| (!mIsUdf(val) && mIsUdf(vals_[posidx_])) )
	calc_.addValue( val, wt );
    else
    {
	if ( !wts_ || wt == wts_[posidx_] )
	    calc_.replaceValue( vals_[posidx_], val, wt );
	else
	{
	    calc_.removeValue( vals_[posidx_], wts_[posidx_] );
	    calc_.addValue( val, wt );
	}
    }

    vals_[posidx_] = val;
    if ( wts_ ) wts_[posidx_] = wt;

    posidx_++;
    if ( posidx_ >= sz_ )
    {
	full_ = true;
	posidx_ = 0;
    }

    return *this;
}

#undef mChkEmpty

}; // namespace Stats

#endif
