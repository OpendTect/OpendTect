#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl (org) / Bert Bril (rev)
 Date:          10-12-1999 / Sep 2006
________________________________________________________________________

-*/

#include "algomod.h"
#include "convert.h"
#include "math2.h"
#include "simpnumer.h"
#include "sorting.h"
#include "stattype.h"
#include "typeset.h"

#define mUndefReplacement 0

/*!\brief Statistics*/

namespace Stats
{

/*!
\brief Setup for the Stats::RunCalc and Stats::ParallelCalc objects.

  medianEvenHandling() is tied to OD_EVEN_MEDIAN_AVERAGE, OD_EVEN_MEDIAN_LOWMID,
  and settings dTect.Even Median.Average and dTect.Even Median.LowMid.
  When medianing over an even number of points, either take the low mid (<0),
  hi mid (>0), or avg the two middles. By default, hi mid is used.
*/

mExpClass(Algo) CalcSetup
{
public:

    typedef od_int64	idx_type;
    typedef idx_type	size_type;

			CalcSetup( bool weighted=false )
			    : weighted_(weighted)
			    , needextreme_(false)
			    , needsums_(false)
			    , needmed_(false)
			    , needsorted_(false)
			    , needvariance_(false)
			    , needmostfreq_(false)	{}

    CalcSetup&	require(Type);
    void		setNeedSorted( bool yn=true )	{ needsorted_ = yn; }

    static int		medianEvenHandling();

    bool		isWeighted() const	{ return weighted_; }
    bool		needExtreme() const	{ return needextreme_; }
    bool		needSums() const	{ return needsums_; }
    bool		needMedian() const	{ return needmed_; }
    bool		needSorted() const	{ return needsorted_; }
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
    bool		needsorted_;
    bool		needmostfreq_;
    bool		needvariance_;

};


/*!
\brief Base class to calculate mean, min, max, etc.. can be used either as
running values (Stats::RunCalc) or in parallel (Stats::ParallelCalc).

  The mostFrequent assumes the data contains integer classes. Then the class
  that is found most often will be the output. Weighting, again, assumes integer
  values. Beware that if you pass data that is not really class-data, the
  memory consumption can become large (and the result will be rather
  uninteresting).

  The variance won't take the decreasing degrees of freedom into consideration
  when weights are provided.

  The object is ready to use with int, float and double types. If other types
  are needed, you may need to specialize an isZero function for each new type.

-*/

template <class T>
mClass(Algo) BaseCalc
{
public:

    mUseType(CalcSetup,idx_type);
    mUseType(CalcSetup,size_type);

    virtual		~BaseCalc()		{}

    inline void		clear();
    const CalcSetup&	setup() const		{ return setup_; }
    bool		isWeighted() const	{ return setup_.weighted_; }

    inline double	getValue(Type) const;
    inline idx_type	getIndex(Type) const; //!< only for Median, Min and Max

    inline bool		hasUndefs() const	{ return nrused_ != nradded_; }
    inline size_type	size( bool used=true ) const
			{ return used ? (size_type)nrused_ : nradded_; }

    inline bool		isEmpty() const		{ return size() == 0; }

    inline size_type	count() const		{ return nrused_; }
    inline double	average() const;
    inline T		mostFreq() const;
    inline T		sum() const;
    inline T		min(idx_type* index_of_min=0) const;
    inline T		max(idx_type* index_of_max=0) const;
    inline T		extreme(idx_type* index_of_extr=0) const;
    inline T		median(idx_type* index_of_median=0) const;
    inline T		sqSum() const;
    inline double	rms() const;
    inline double	stdDev() const;
    inline double	normvariance() const;
    virtual inline double variance() const;

    inline T		clipVal(float ratio,bool upper) const;
			//!< requires sort; 0 <= ratio <= 1

    const T*		medValsArr() const	{ return medvals_.arr(); }

protected:
			BaseCalc( const CalcSetup& s )
			    : setup_(s) { clear(); }

    CalcSetup		setup_;

    size_type		nradded_;
    Threads::Atomic<size_type>	nrused_;
    idx_type		minidx_;
    idx_type		maxidx_;
    T			minval_;
    T			maxval_;
    T			sum_x_;
    T			sum_xx_;
    T			sum_w_;
    T			sum_wx_;
    T			sum_wxx_;
    bool		issorted_ = false;

    LargeValVec<idx_type>	clss_;
    LargeValVec<T>	clsswt_;
    LargeValVec<T>	medvals_;
    LargeValVec<T>	medwts_;
    LargeValVec<idx_type>	medidxs_;

    inline bool		isZero(const T&) const;
    virtual const T*	sort(idx_type* index_of_median =nullptr);
    T			computeMedian(idx_type* index_of_median =nullptr) const;
    T			computeWeightedMedian(
				      idx_type* index_of_median =nullptr) const;
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


/*!
\brief Calculates mean, min, max etc., as running values.

  The idea is that you simply add values and ask for a stat whenever needed.
  The clear() method resets the object and makes it able to work with new data.

  Adding values can be doing with weight (addValue) or without (operator +=).
  You can remove a value; for Min or Max this has no effect as this would
  require buffering all data.
-*/

template <class T>
mClass(Algo) RunCalc : public BaseCalc<T>
{
public:

    mUseType(CalcSetup,idx_type);
    mUseType(CalcSetup,size_type);

			RunCalc( const CalcSetup& s )
			    : BaseCalc<T>(s) {}

    inline RunCalc<T>&	addValue(T data,T weight=1);
    RunCalc<T>&		addValues(size_type sz,const T* data,
				  const T* weights=0);
    inline RunCalc<T>&	replaceValue(T olddata,T newdata,T wt=1);
    inline RunCalc<T>&	removeValue(T data,T weight=1);

    inline RunCalc<T>&	operator +=( T t )	{ return addValue(t); }

protected:

    inline void		setMostFrequent(T val,T wt);

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
    using BaseCalc<T>::medvals_;
    using BaseCalc<T>::medwts_;
};



/*!
\brief RunCalc manager which buffers a part of the data.

  Allows calculating running stats on a window only. Once the window is full,
  WindowedCalc will replace the first value added (fifo).
*/

template <class T>
mClass(Algo) WindowedCalc
{
public:

    mUseType(CalcSetup,idx_type);
    mUseType(CalcSetup,size_type);

			WindowedCalc( const CalcSetup& rcs, size_type sz )
			    : calc_(rcs)
			    , sz_(sz)
			    , wts_(calc_.isWeighted() ? new T [sz] : 0)
			    , vals_(new T [sz])	{ clear(); }
			~WindowedCalc()
				{ delete [] vals_; delete [] wts_; }
    inline void		clear();

    inline WindowedCalc& addValue(T data,T weight=1);
    inline WindowedCalc& operator +=( T t )	{ return addValue(t); }

    inline idx_type	getIndex(Type) const;
			//!< Only use for Min, Max or Median
    inline double	getValue(Type) const;

    inline size_type	count() const	{ return full_ ? sz_ : posidx_; }

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

    inline T			min(idx_type* i=0) const;
    inline T			max(idx_type* i=0) const;
    inline T			extreme(idx_type* i=0) const;
    inline T			median(idx_type* i=0) const;

protected:

    RunCalc<T>		calc_; // has to be before wts_ (see constructor inits)
    const size_type	sz_;
    T*			wts_;
    T*			vals_;
    idx_type		posidx_;
    bool		empty_;
    bool		full_;
    bool		needcalc_;

    inline void	fillCalc(RunCalc<T>&) const;
};



template <class T> inline
void BaseCalc<T>::clear()
{
    sum_x_ = sum_w_ = sum_xx_ = sum_wx_ = sum_wxx_ = 0;
    nradded_ = nrused_ = minidx_ = maxidx_ = 0;
    minval_ = maxval_ = mUdf(T);
    clss_.setEmpty(); clsswt_.setEmpty();
    medvals_.setEmpty(); medwts_.setEmpty(); medidxs_.setEmpty();
    issorted_ = false;
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
CalcSetup::idx_type BaseCalc<T>::getIndex( Type t ) const
{
    idx_type ret = 0;
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

    return this->isZero(sum_w_) ? mUdf(double)
				: Math::Sqrt( ((double)sum_wxx_)/sum_w_ );
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


template <>
inline double BaseCalc<float_complex>::variance() const
{
    pErrMsg("Undefined operation for float_complex in template");
    return mUdf(double);
}


template <class T>
inline double BaseCalc<T>::normvariance() const
{
    if ( nrused_ < 2 ) return 0;

    double fact = 0.1;
    double avg = average();
    double var = variance();
    const double divisor = avg*avg + fact*var;
    if ( mIsZero(divisor,mDefEps) )
	return 0;

    return var / divisor;
}


template <class T>
inline T BaseCalc<T>::min( idx_type* index_of_min ) const
{
    if ( index_of_min ) *index_of_min = minidx_;
    mBaseCalc_ChkEmpty(T);
    return minval_;
}


template <class T>
inline T BaseCalc<T>::max( idx_type* index_of_max ) const
{
    if ( index_of_max ) *index_of_max = maxidx_;
    mBaseCalc_ChkEmpty(T);
    return maxval_;
}


template <class T>
inline T BaseCalc<T>::extreme( idx_type* index_of_extr ) const
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


template <class T> inline
const T* BaseCalc<T>::sort( idx_type* idx_of_med )
{
    T* valarr = medvals_.arr();
    const bool withidxs = idx_of_med || setup_.weighted_;
    if ( issorted_ &&
	 ( (withidxs && medidxs_.size()==nrused_) || (!withidxs) ) )
	return valarr;

    if ( withidxs )
    {
	if ( medidxs_.size() != nrused_ )
	{
	    medidxs_.setSize( nrused_, 0 );
	    for ( idx_type idx=0; idx<nrused_; idx++ )
		medidxs_[idx] = idx;
	}

	quickSort( valarr, medidxs_.arr(), nrused_ );
    }
    else if ( nrused_<=255 || !is8BitesData(valarr,nrused_,100) ||
	     !duplicate_sort(valarr,nrused_,256) )
	quickSort( valarr, nrused_ );

    return valarr;
}


template <> inline
const float_complex* BaseCalc<float_complex>::sort( idx_type* idx_of_med )
{
    pErrMsg("Undefined operation for float_complex in template");
    return nullptr;
}


template <class T>
inline T BaseCalc<T>::computeWeightedMedian( idx_type* idx_of_med ) const
{
    const T* valarr = medvals_.arr();
    const T* wts = medwts_.arr();
    const size_type sz = nrused_;
    T* wtcopy = new T[sz];
    OD::memCopy( wtcopy, wts, sz*sizeof(T) );
    double wsum = 0;
    const idx_type* idxs = medidxs_.arr();
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	const_cast<T&>(wts[idx]) = wtcopy[ idxs[idx] ];
	wsum += (double) wts[idx];
    }
    delete [] wtcopy;

    const_cast<LargeValVec<idx_type>&>( medidxs_ ).setEmpty();
    const double hwsum = wsum * 0.5;
    wsum = 0;
    idx_type medidx = 0;
    for ( size_type idx=0; idx<sz; idx++ )
    {
	wsum += (double) wts[idx];
	if ( wsum >= hwsum )
	    { medidx = idx; break; }
    }

    if ( idx_of_med ) *idx_of_med = medidx;

    return valarr[sz/2];
}


template <class T>
inline T BaseCalc<T>::computeMedian( idx_type* idx_of_med ) const
{
    const T* valarr = medvals_.arr();
    const size_type sz = nrused_;
    idx_type mididx = sz/2;
    if ( idx_of_med )
    {
	const idx_type* idxs = medidxs_.arr();
	*idx_of_med = idxs[ mididx ];
	const_cast<LargeValVec<idx_type>&>( medidxs_ ).setEmpty();
    }

    if ( sz%2 == 0 )
    {
	const int policy = setup_.medianEvenHandling();
	if ( policy == 0 )
	    return (valarr[mididx] + valarr[mididx-1]) / 2;
	else if ( policy == 1 )
	    mididx--;
    }

    return valarr[mididx];
}


template <class T>
inline T BaseCalc<T>::median( idx_type* index_of_median ) const
{
    mBaseCalc_ChkEmpty(T);
    const size_type sz = nrused_;
    if ( sz == 1 )
    {
	if ( index_of_median ) *index_of_median = 0;
	return medvals_[0];
    }

    const_cast<BaseCalc<T>&>( *this ).sort( index_of_median );
    return setup_.weighted_ ? computeWeightedMedian( index_of_median )
			    : computeMedian( index_of_median );
}


template <class T>
inline T BaseCalc<T>::clipVal( float ratio, bool upper ) const
{
    mBaseCalc_ChkEmpty(T);
    const T* vararr = const_cast<BaseCalc<T>&>( *this ).sort();

    const size_type lastidx = nrused_;
    const float fidx = ratio * lastidx;
    const idx_type idx = fidx <= 0 ? 0
				   : (fidx > lastidx ? lastidx
						     : (idx_type)fidx);
    return vararr[upper ? lastidx - idx : idx];
}


template <class T>
inline T BaseCalc<T>::mostFreq() const
{
    if ( clss_.isEmpty() )
	return mUdf(T);

    T maxwt = clsswt_[0]; idx_type ret = clss_[0];
    for ( idx_type idx=1; idx<clss_.size(); idx++ )
    {
	if ( clsswt_[idx] > maxwt )
	    { maxwt = clsswt_[idx]; ret = clss_[idx]; }
    }

    return (T)ret;
}


template<> inline
float_complex BaseCalc<float_complex>::mostFreq() const
{ return mUdf(float_complex); }



template <class T>
inline RunCalc<T>& RunCalc<T>::addValue( T val, T wt )
{
    nradded_++;
    if ( mIsUdf(val) )
	return *this;
    if ( setup_.weighted_ && (mIsUdf(wt) || this->isZero(wt)) )
	return *this;

    if ( setup_.needmed_ || setup_.needsorted_ )
    {
	BaseCalc<T>::issorted_ = false;
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
	setMostFrequent( val, wt );

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
void RunCalc<T>::setMostFrequent( T val, T wt )
{
    idx_type ival; Conv::set( ival, val );
    idx_type setidx = clss_.indexOf( ival );

    if ( setidx < 0 )
	{ clss_ += ival; clsswt_ += wt; }
    else
	clsswt_[setidx] += wt;
}


template <> inline
void RunCalc<float_complex>::setMostFrequent( float_complex val,
					      float_complex wt )
{ pErrMsg("Undefined operation for float_complex in template"); }


template <class T> inline
RunCalc<T>& RunCalc<T>::removeValue( T val, T wt )
{
    nradded_--;
    if ( mIsUdf(val) )
	return *this;
    if ( setup_.weighted_ && (mIsUdf(wt) || this->isZero(wt)) )
	return *this;

    if ( setup_.needmed_ || setup_.needsorted_ )
    {
	size_type idx = medvals_.size();
	while ( true )
	{
	    idx = medvals_.indexOf( val, false, idx-1 );
	    if ( idx < 0 ) break;
	    if ( setup_.weighted_ && medwts_[idx] == wt )
	    {
		medvals_.removeSingle( idx );
		medwts_.removeSingle( idx );
		break;
	    }
	}
    }

    if ( setup_.needmostfreq_ )
    {
	idx_type ival; Conv::set( ival, val );
	idx_type setidx = clss_.indexOf( ival );
	idx_type iwt = 1;
	if ( setup_.weighted_ )
	    Conv::set( iwt, wt );

	if ( setidx >= 0 )
	{
	    clsswt_[setidx] -= wt;
	    if ( clsswt_[setidx] <= 0 )
	    {
		clss_.removeSingle( setidx );
		clsswt_.removeSingle( setidx );
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
RunCalc<T>& RunCalc<T>::addValues( size_type sz, const T* data,
				   const T* weights )
{
    for ( idx_type idx=0; idx<sz; idx++ )
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

    const idx_type stopidx = full_ ? sz_ : posidx_;
    for ( idx_type idx=posidx_; idx<stopidx; idx++ )
	calc.addValue( vals_[idx], wts_ ? wts_[idx] : 1 );
    for ( idx_type idx=0; idx<posidx_; idx++ )
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
CalcSetup::idx_type WindowedCalc<T>::getIndex( Type t ) const
{
    idx_type ret = 0;
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
T WindowedCalc<T>::min( idx_type* index_of_min ) const
{
    RunCalc<T> calc( CalcSetup().require(Stats::Min) );
    fillCalc( calc );
    return calc.min( index_of_min );
}


template <class T> inline
T WindowedCalc<T>::max( idx_type* index_of_max ) const
{
    RunCalc<T> calc( CalcSetup().require(Stats::Max) );
    fillCalc( calc );
    return calc.max( index_of_max );
}


template <class T> inline
T WindowedCalc<T>::extreme( idx_type* index_of_extr ) const
{
    RunCalc<T> calc( CalcSetup().require(Stats::Extreme) );
    fillCalc( calc );
    return calc.extreme( index_of_extr );
}


template <class T> inline
T WindowedCalc<T>::median( idx_type* index_of_med ) const
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

} // namespace Stats

