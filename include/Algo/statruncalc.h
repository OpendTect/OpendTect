#ifndef statruncalc_h
#define statruncalc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl (org) / Bert Bril (rev)
 Date:          10-12-1999 / Sep 2006
 RCS:           $Id: statruncalc.h,v 1.31 2012-08-13 09:36:56 cvsaneesh Exp $
________________________________________________________________________

-*/

#include "algomod.h"
#include "convert.h"
#include "math2.h"
#include "stattype.h"
#include "sorting.h"
#include "sets.h"

#define mUndefReplacement 0

namespace Stats
{


/*!\brief setup for the Stats::RunCalc and Stats::ParallelCalc objects

  medianEvenHandling() is tied to OD_EVEN_MEDIAN_AVERAGE, OD_EVEN_MEDIAN_LOWMID,
  and settings dTect.Even Median.Average and dTect.Even Median.LowMid.
  When medianing over an even number of points, either take the low mid (<0),
  hi mid (>0), or avg the two middles. By default, hi mid is used.
 
 */


mClass(Algo) CalcSetup
{ 
public:
    			CalcSetup( bool weighted=false )
			    : weighted_(weighted)
			    , needextreme_(false)
			    , needsums_(false)
			    , needmed_(false) 
			    , needvariance_(false) 
			    , needmostfreq_(false)	{}

    CalcSetup&       	require(Type);

    static int		medianEvenHandling();

    bool		isWeighted() const	{ return weighted_; }
    bool		needExtreme() const	{ return needextreme_; }
    bool		needSums() const	{ return needsums_; }
    bool		needMedian() const	{ return needmed_; }
    bool		needMostFreq() const	{ return needmostfreq_; }
    bool		needVariance() const	{ return needvariance_; }

protected:

    template <class T>
    friend class	BaseCalc;
    template <class T>
    friend class	RunCalc;
    template <class T>
    friend class	WindowedCalc;
    template <class T>
    friend class	ParallelCalc;

    bool		weighted_;
    bool		needextreme_;
    bool		needsums_;
    bool		needmed_;
    bool		needmostfreq_;
    bool		needvariance_;

};


/*!\brief base class to calculate mean, min, max, etc.. can be used either 
as running values (Stats::RunCalc) or in parallel (Stats::ParallelCalc).

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
class BaseCalc
{
public:

    virtual		~BaseCalc()		{}

    inline void		clear();
    const CalcSetup&	setup() const		{ return setup_; }
    bool		isWeighted() const	{ return setup_.weighted_; }

    inline double	getValue(Type) const;
    inline int		getIndex(Type) const; //!< only for Median, Min and Max

    inline bool		hasUndefs() const	{ return nrused_ != nradded_; }
    inline int		size( bool used=true ) const
    			{ return used ? nrused_ : nradded_; }

    inline bool		isEmpty() const		{ return size() == 0; }

    inline int		count() const		{ return nrused_; }
    inline double	average() const;
    inline T		mostFreq() const;
    inline T		sum() const;
    inline T		min(int* index_of_min=0) const;
    inline T		max(int* index_of_max=0) const;
    inline T		extreme(int* index_of_extr=0) const;
    inline T		median(int* index_of_median=0) const;
    inline T		sqSum() const;
    inline double	rms() const;
    inline double	stdDev() const;
    inline double	normvariance() const;
    virtual inline double variance() const; 

    inline T		clipVal(float ratio,bool upper) const;
    			//!< require median; 0 <= ratio <= 1

    TypeSet<T>		medvals_;

protected:
			BaseCalc( const CalcSetup& s )
			    : setup_(s) { clear(); }

    CalcSetup		setup_;

    int			nradded_; 
    int			nrused_;
    int			minidx_;
    int			maxidx_;
    T			minval_;
    T			maxval_;
    T			sum_x_;
    T			sum_xx_;
    T			sum_w_;
    T			sum_wx_;
    T			sum_wxx_;

    TypeSet<int>	clss_;
    TypeSet<T>		clsswt_;
    TypeSet<T>		medwts_;

    inline bool		isZero( const T& t ) const;
};


template <> inline
bool BaseCalc<float>::isZero( const float& val ) const
{ return mIsZero(val,1e-6); }


template <> inline
bool BaseCalc<double>::isZero( const double& val ) const
{ return mIsZero(val,1e-10); }


template <class T> inline
bool BaseCalc<T>::isZero( const T& val ) const
{ return !val; }


/*!\brief calculates mean, min, max, etc. as running values.

The idea is that you simply add values and ask for a stat whenever needed.
The clear() method resets the object and makes it able to work with new data.

Adding values can be doing with weight (addValue) or without (operator +=).
You can remove a value; for Min or Max this has no effect as this would
require buffering all data.

-*/


template <class T>
class RunCalc : public BaseCalc<T>
{
public:
    			RunCalc( const CalcSetup& s )
			    : BaseCalc<T>(s) {}

    inline RunCalc<T>&	addValue(T data,T weight=1);
    inline RunCalc<T>&	addValues(int sz,const T* data,const T* weights=0);
    inline RunCalc<T>&	replaceValue(T olddata,T newdata,T wt=1);
    inline RunCalc<T>&	removeValue(T data,T weight=1);

    inline RunCalc<T>&	operator +=( T t )	{ return addValue(t); }

    using BaseCalc<T>::medvals_;

protected:

    using BaseCalc<T>::setup_;
    using BaseCalc<T>::nradded_;
    using BaseCalc<T>::nrused_;
    using BaseCalc<T>::minidx_;
    using BaseCalc<T>::maxidx_;
    using BaseCalc<T>::maxval_;
    using BaseCalc<T>::minval_;
    using BaseCalc<T>::sum_x_;
    using BaseCalc<T>::sum_xx_;
    using BaseCalc<T>::sum_w_;
    using BaseCalc<T>::sum_wx_;
    using BaseCalc<T>::sum_wxx_;
    using BaseCalc<T>::clss_;
    using BaseCalc<T>::clsswt_;
    using BaseCalc<T>::medwts_;
};



/*!\brief RunCalc manager which buffers a part of the data.
 
  Allows calculating running stats on a window only. Once the window is full,
  WindowedCalc will replace the first value added (fifo).
 
 */


template <class T>
class WindowedCalc
{
public:
			WindowedCalc( const CalcSetup& rcs, int sz )
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

    inline T		count() const	{ return full_ ? sz_ : posidx_; }

#   define			mRunCalc_DefEqFn(ret,fn) \
    inline ret			fn() const	{ return calc_.fn(); }
    mRunCalc_DefEqFn(double,	average)
    mRunCalc_DefEqFn(double,	variance)
    mRunCalc_DefEqFn(double,	normvariance)
    mRunCalc_DefEqFn(T,		sum)
    mRunCalc_DefEqFn(T,		sqSum)
    mRunCalc_DefEqFn(double,	rms)
    mRunCalc_DefEqFn(double,	stdDev)
    mRunCalc_DefEqFn(T,		mostFreq)
#   undef			mRunCalc_DefEqFn

    inline T			min(int* i=0) const;
    inline T			max(int* i=0) const;
    inline T			extreme(int* i=0) const;
    inline T			median(int* i=0) const;

protected:

    RunCalc<T>	calc_; // has to be before wts_ (see constructor inits)
    const int	sz_;
    T*		wts_;
    T*		vals_;
    int		posidx_;
    bool	empty_;
    bool	full_;
    bool	needcalc_;

    inline void	fillCalc(RunCalc<T>&) const;
};



template <class T> inline
void BaseCalc<T>::clear()
{
    sum_x_ = sum_w_ = sum_xx_ = sum_wx_ = sum_wxx_ = 0;
    nradded_ = nrused_ = minidx_ = maxidx_ = 0;
    minval_ = maxval_ = mUdf(T);
    clss_.erase(); clsswt_.erase(); medvals_.erase(); medwts_.erase();
}


template <class T> inline
double BaseCalc<T>::getValue( Stats::Type t ) const
{
    switch ( t )
    {
	case Count:		return count();
	case Average:		return average();
	case Median:		return median();
	case RMS:		return rms();
	case StdDev:		return stdDev();
	case Variance:		return variance();
	case NormVariance:	return normvariance();			
	case Min:		return min();
	case Max:		return max();
	case Extreme:		return extreme();
	case Sum:		return sum();
	case SqSum:		return sqSum();
	case MostFreq:		return mostFreq();
    }

    return 0;
}


template <class T> inline
int BaseCalc<T>::getIndex( Type t ) const
{
    int ret;
    switch ( t )
    {
	case Min:		(void)min( &ret );	break;
	case Max:		(void)max( &ret );	break;
	case Extreme:		(void)extreme( &ret );	break;
	case Median:		(void)median( &ret );	break;
	default:		ret = 0;	break;
    }
    return ret;
}



#undef mBaseCalc_ChkEmpty
#define mBaseCalc_ChkEmpty(typ) \
    if ( nrused_ < 1 ) return mUdf(typ);

template <class T>
inline double BaseCalc<T>::stdDev() const
{
    mBaseCalc_ChkEmpty(double);

    double v = variance();
    return v > 0 ? Math::Sqrt( v ) : 0;
}


template <class T>
inline double BaseCalc<T>::average() const
{
    mBaseCalc_ChkEmpty(double);

    if ( !setup_.weighted_ )
	return ((double)sum_x_) / nrused_;

    return this->isZero(sum_w_) ? mUdf(double) : ((double)sum_wx_) / sum_w_;
}


template <class T>
inline T BaseCalc<T>::sum() const
{
    mBaseCalc_ChkEmpty(T);
    return sum_x_;
}


template <class T>
inline T BaseCalc<T>::sqSum() const
{
    mBaseCalc_ChkEmpty(T);
    return sum_xx_;
}


template <class T>
inline double BaseCalc<T>::rms() const
{
    mBaseCalc_ChkEmpty(double);

    if ( !setup_.weighted_ )
	return Math::Sqrt( ((double)sum_xx_) / nrused_ );

    return this->isZero(sum_w_) ? mUdf(double) : Math::Sqrt( ((double)sum_wxx_)/sum_w_ );
}


template <class T>
inline double BaseCalc<T>::variance() const 
{
    if ( nrused_ < 2 ) return 0;

    if ( !setup_.weighted_ )
	return ( sum_xx_ - (sum_x_ * ((double)sum_x_) / nrused_) )
	     / (nrused_ - 1);

    return (sum_wxx_ - (sum_wx_ * ((double)sum_wx_) / sum_w_) )
	 / ( (nrused_-1) * ((double)sum_w_) / nrused_ );
}


template <class T>
inline double BaseCalc<T>::normvariance() const
{
    if ( nrused_ < 2 ) return 0;

    double fact = 0.1;
    double avg = average();
    double var = variance();
    return var / (avg*avg + fact*var);
}


template <class T>
inline T BaseCalc<T>::min( int* index_of_min ) const
{
    if ( index_of_min ) *index_of_min = minidx_;
    mBaseCalc_ChkEmpty(T);
    return minval_;
}


template <class T>
inline T BaseCalc<T>::max( int* index_of_max ) const
{
    if ( index_of_max ) *index_of_max = maxidx_;
    mBaseCalc_ChkEmpty(T);
    return maxval_;
}


template <class T>
inline T BaseCalc<T>::extreme( int* index_of_extr ) const
{
    if ( index_of_extr ) *index_of_extr = 0;
    mBaseCalc_ChkEmpty(T);

    const T maxcmp = maxval_ < 0 ? -maxval_ : maxval_;
    const T mincmp = minval_ < 0 ? -minval_ : minval_;
    if ( maxcmp < mincmp )
    {
	if ( index_of_extr ) *index_of_extr = minidx_;
	return minval_;
    }
    else
    {
	if ( index_of_extr ) *index_of_extr = maxidx_;
	return maxval_;
    }
}


template <class T>
inline T BaseCalc<T>::clipVal( float ratio, bool upper ) const
{
    mBaseCalc_ChkEmpty(T);
    (void)median();
    const int lastidx = medvals_.size();
    const float fidx = ratio * lastidx;
    const int idx = fidx <= 0 ? 0 : (fidx > lastidx ? lastidx : (int)fidx);
    return medvals_[upper ? lastidx - idx : idx];
}


template <class T>
inline T BaseCalc<T>::mostFreq() const
{
    if ( clss_.isEmpty() )
	return mUdf(T);

    T maxwt = clsswt_[0]; int ret = clss_[0];
    for ( int idx=1; idx<clss_.size(); idx++ )
    {
	if ( clsswt_[idx] > maxwt )
	    { maxwt = clsswt_[idx]; ret = clss_[idx]; }
    }

    return ret;
}


template <class T> inline
T computeMedian( const T* data, int sz, int pol, int* idx_of_med ) 
{
    if ( idx_of_med ) *idx_of_med = 0;
    if ( sz < 2 )
	return sz < 1 ? mUdf(T) : data[0];

    int mididx = sz / 2;
    T* valarr = const_cast<T*>( data );
    if ( !idx_of_med )
	quickSort( valarr, sz );
    else
    {
	mGetIdxArr(int,idxs,sz)
	quickSort( valarr, idxs, sz );
	*idx_of_med = idxs[ mididx ];
	delete [] idxs;
    }

    if ( sz%2 == 0 )
    {
	if ( pol == 0 )
	    return (data[mididx] + data[mididx-1]) / 2;
	else if ( pol == 1 )
	   mididx--;
    }

    return data[ mididx ];
}


template <class T> inline
T computeWeightedMedian( const T* data, const T* wts, int sz, 
				int* idx_of_med ) 
{
    if ( idx_of_med ) *idx_of_med = 0;
    if ( sz < 2 )
	return sz < 1 ? mUdf(T) : data[0];

    T* valarr = const_cast<T*>( data );
    mGetIdxArr(int,idxs,sz)
    quickSort( valarr, idxs, sz );
    T* wtcopy = new T[sz];
    memcpy( wtcopy, wts, sz*sizeof(T) );
    float wsum = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	const_cast<T&>(wts[idx]) = wtcopy[ idxs[idx] ];
	wsum += (float) wts[idx];
    }
    delete [] idxs;

    const float hwsum = wsum * 0.5f;
    wsum = 0;
    int medidx = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	wsum += (float) wts[idx];
	if ( wsum >= hwsum )
	    { medidx = idx; break; }
    }
    if ( idx_of_med ) *idx_of_med = medidx;
    return valarr[ medidx ];
}


template <class T>
inline T BaseCalc<T>::median( int* idx_of_med ) const
{
    const int policy = setup_.medianEvenHandling();
    const int sz = medvals_.size();
    const T* vals = medvals_.arr();
    return setup_.weighted_ ? 
	  computeWeightedMedian( vals, medwts_.arr(), sz, idx_of_med ) 
	: computeMedian( vals, sz, policy, idx_of_med );
}




template <class T> inline
RunCalc<T>& RunCalc<T>::addValue( T val, T wt )
{
    nradded_++;
    if ( mIsUdf(val) )
	return *this;
    if ( setup_.weighted_ && (mIsUdf(wt) || this->isZero(wt)) )
	return *this;

    if ( setup_.needmed_ )
    {
	medvals_ += val;
	if ( setup_.weighted_ )
	    medwts_ += wt;
    }

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

	if ( setidx < 0 )
	    { clss_ += ival; clsswt_ += wt; }
	else
	    clsswt_[setidx] += wt;
    }

    if ( setup_.needsums_ )
    {
	sum_x_ += val;
	sum_xx_ += val * val;
	if ( setup_.weighted_ )
	{
	    sum_w_ += wt;
	    sum_wx_ += wt * val;
	    sum_wxx_ += wt * val * val;
	}
    }

    nrused_++;
    return *this;
}


template <class T> inline
RunCalc<T>& RunCalc<T>::removeValue( T val, T wt )
{
    nradded_--;
    if ( mIsUdf(val) )
	return *this;
    if ( setup_.weighted_ && (mIsUdf(wt) || this->isZero(wt)) )
	return *this;

    if ( setup_.needmed_ )
    {
	int idx = medvals_.size();
	while ( true )
	{
	    idx = medvals_.indexOf( val, false, idx-1 );
	    if ( idx < 0 ) break;
	    if ( medwts_[idx] == wt )
	    {
		medvals_.remove( idx );
		medwts_.remove( idx );
		break;
	    }
	}
    }

    if ( setup_.needmostfreq_ )
    {
	int ival; Conv::set( ival, val );
	int setidx = clss_.indexOf( ival );
	int iwt = 1;
	if ( setup_.weighted_ )
	    Conv::set( iwt, wt );

	if ( setidx >= 0 )
	{
	    clsswt_[setidx] -= wt;
	    if ( clsswt_[setidx] <= 0 )
	    {
		clss_.remove( setidx );
		clsswt_.remove( setidx );
	    }
	}
    }

    if ( setup_.needsums_ )
    {
	sum_x_ -= val;
	sum_xx_ -= val * val;
	if ( setup_.weighted_ )
	{
	    sum_w_ -= wt;
	    sum_wx_ -= wt * val;
	    sum_wxx_ -= wt * val * val;
	}
    }

    nrused_--;
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
RunCalc<T>& RunCalc<T>::replaceValue( T oldval, T newval, T wt )
{
    removeValue( oldval, wt );
    return addValue( newval, wt );
}




template <class T> inline
void WindowedCalc<T>::clear()
{
    posidx_ = 0; empty_ = true; full_ = false;
    needcalc_ = calc_.setup().needSums() || calc_.setup().needMostFreq();
    calc_.clear();
}


template <class T> inline
void WindowedCalc<T>::fillCalc( RunCalc<T>& calc ) const
{
    if ( empty_ ) return;

    const int stopidx = full_ ? sz_ : posidx_;
    for ( int idx=posidx_; idx<stopidx; idx++ )
	calc.addValue( vals_[idx], wts_ ? wts_[idx] : 1 );
    for ( int idx=0; idx<posidx_; idx++ )
	calc.addValue( vals_[idx], wts_ ? wts_[idx] : 1 );
}


template <class T> inline
double WindowedCalc<T>::getValue( Type t ) const
{
    switch ( t )
    {
    case Min:		return min();
    case Max:		return max();
    case Extreme:	return extreme();
    case Median:	return median();
    default:		break;
    }
    return calc_.getValue( t );
}


template <class T> inline
int WindowedCalc<T>::getIndex( Type t ) const
{
    int ret;
    switch ( t )
    {
	case Min:		(void)min( &ret );	break;
	case Max:		(void)max( &ret );	break;
	case Extreme:		(void)extreme( &ret );	break;
	case Median:		(void)median( &ret );	break;
	default:		ret = 0;	break;
    }
    return ret;
}


template <class T> inline
T WindowedCalc<T>::min( int* index_of_min ) const
{
    RunCalc<T> calc( CalcSetup().require(Stats::Min) );
    fillCalc( calc );
    return calc.min( index_of_min );
}


template <class T> inline
T WindowedCalc<T>::max( int* index_of_max ) const
{
    RunCalc<T> calc( CalcSetup().require(Stats::Max) );
    fillCalc( calc );
    return calc.max( index_of_max );
}


template <class T> inline
T WindowedCalc<T>::extreme( int* index_of_extr ) const
{
    RunCalc<T> calc( CalcSetup().require(Stats::Extreme) );
    fillCalc( calc );
    return calc.extreme( index_of_extr );
}


template <class T> inline
T WindowedCalc<T>::median( int* index_of_med ) const
{
    CalcSetup rcs( calc_.setup().weighted_ );
    RunCalc<T> calc( rcs.require(Stats::Median) );
    fillCalc( calc );
    return calc.median( index_of_med );
}


template <class T>
inline WindowedCalc<T>&	WindowedCalc<T>::addValue( T val, T wt )
{
    if ( needcalc_ )
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
    }

    vals_[posidx_] = val;
    if ( wts_ ) wts_[posidx_] = wt;

    posidx_++;
    if ( posidx_ >= sz_ )
	{ full_ = true; posidx_ = 0; }

    empty_ = false;
    return *this;
}

#undef mRunCalc_ChkEmpty

}; // namespace Stats

#endif

