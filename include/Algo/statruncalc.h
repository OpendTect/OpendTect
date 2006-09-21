#ifndef statruncalc_h
#define statruncalc_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl (org) / Bert Bril (rev)
 Date:          10-12-1999 / Sep 2006
 RCS:           $Id: statruncalc.h,v 1.4 2006-09-21 12:02:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "stattype.h"
#include "sorting.h"
#include "sets.h"
#include "convert.h"

#include <math.h>

#define mUndefReplacement 0

namespace Stats
{


/*!\brief setup for the Stats::RunCalc object */

class RunCalcSetup
{
public:
    			RunCalcSetup( bool weighted=false )
			    : weighted_(weighted)
			    , needextreme_(false)
			    , needsums_(false)
			    , needmed_(false)
			    , needmostfreq_(false)	{}

    RunCalcSetup&	require(Type);

protected:

    template <class T>
    friend class	RunCalc;

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
    bool		isWeighted() const	{ return setup_.weighted_; }

    inline RunCalc<T>&	addValue(T data,T weight=1);
    inline RunCalc<T>&	replaceValue(T olddata,T newdata,T wt=1);
    inline RunCalc<T>&	removeValue(T data,T weight=1);

    inline double	getValue(Type) const;
    inline int		getIndex(Type) const; //!< only for Median, Min and Max

    inline RunCalc<T>&	operator +=( T t )	{ return addValue(t); }
    inline bool		hasUndefs() const	{ return nrused_ != nradded_; }
    inline int		size( bool used=true ) const
    			{ return used ? nrused_ : nradded_; }

    inline double	mean() const;
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
    TypeSet<T>		vals_;

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

    const RunCalc<T>&	runCalc() const		{ return calc_; }

protected:

    RunCalc<T>	calc_; // has to be before wts_ (see constructor inits)
    const int	sz_;
    T*		wts_;
    T*		vals_;
    int		posidx_;
    bool	full_;

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
    if ( !addExceptMed( newval, wt ) || !setup_.needmed_ || vals_.size() < 1 )
	return *this;

    int iwt = 1;
    if ( setup_.weighted_ )
	Conv::set( iwt, wt );

    for ( int idx=0; idx<iwt; idx++ )
    {
	if ( !findMedIdx(oldval) )
	    break;

	vals_[curmedidx_] = newval;
	curmedidx_++;
    }

    return *this;
}


template <class T> inline
RunCalc<T>& RunCalc<T>::removeValue( T val, T wt )
{
    if ( !remExceptMed(val,wt) || !setup_.needmed_ || vals_.size() < 1 )
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
	case Average:		return mean();
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
    return v > 0 ? sqrt( v ) : 0;
}


template <class T>
inline double RunCalc<T>::mean() const
{
    mChkEmpty(double);

    if ( !setup_.weighted_ )
	return ((double)sum_x) / nrused_;

    return isZero(sum_w) ? mUdf(double) : ((double)sum_wx) / sum_w;
}


template <class T>
inline T RunCalc<T>::mostFreq() const
{
    if ( clss_.size() < 1 )
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
	int* idxs = new int [sz];
	for ( int idx=0; idx<sz; idx++ ) idxs[idx] = idx;
	quickSort( valarr, idxs, sz );
	*idx_of_med = idxs[ mididx ];
	delete [] idxs;
    }
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
	return sqrt( ((double)sum_xx) / nrused_ );

    return isZero(sum_w) ? mUdf(double) : sqrt( ((double)sum_wxx) / sum_w );
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
    double avg = mean();
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


template <class T> inline
void WindowedCalc<T>::clear()
{
    posidx_ = 0; full_ = false;
    calc_.clear();
}


template <class T>
inline WindowedCalc<T>&	WindowedCalc<T>::addValue( T val, T wt )
{
    if ( !full_ )
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
