#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "math2.h"
#include "paralleltask.h"
#include "statruncalc.h"
#include "uistrings.h"

namespace Stats
{

/*!
\brief Stats computation running in parallel.

  The difference with the running values (Stats::RunCalc) is that you have to
  pass the entire data array prior to the execution.

  It also works with optional weights.
*/

template <class T>
mClass(Algo) ParallelCalc : public ParallelTask, public BaseCalc<T>
{ mODTextTranslationClass(ParallelCalc);
public:

    mUseType(CalcSetup,idx_type);
    mUseType(CalcSetup,size_type);

				ParallelCalc(const CalcSetup& s,const T* data,
					     size_type sz,const T* weights = 0)
				    : BaseCalc<T>(s)
						{ setValues(data,sz,weights); }
				ParallelCalc( const CalcSetup& s )
				    : BaseCalc<T>(s) { setValues(0,0,0); }

    inline void			setValues(const T* inp,size_type sz,
					  const T* wght=0);
    inline void			setEmpty();

    const uiString		errMsg() const		{ return errmsg_; }

    inline double		variance() const override;

    using BaseCalc<T>::medvals_;

protected:

    od_int64		nrIterations() const override
			{ return nradded_; }

    inline bool		doPrepare(int) override;
    inline bool		doWork(od_int64,od_int64,int) override;
    inline bool		doFinish(bool) override;

    const T*		sort(idx_type* index_of_median =nullptr) override;

    uiString		errmsg_;

    mutable Threads::Barrier barrier_;

    const T*		data_ = nullptr;
    const T*		weights_ = nullptr;
    bool*		udfarr_ = nullptr;

    T			meanval_;
    T			meanval_w_;
    T			variance_;
    T			variance_w_;

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


template <class T>
inline void ParallelCalc<T>::setEmpty()
{
    this->clear();
    nradded_ = 0; data_ = nullptr; weights_ = nullptr;
    deleteAndZeroPtr( udfarr_ );
}


template <class T>
inline void ParallelCalc<T>::setValues( const T* data, size_type sz,
					const T* wght )
{
    this->clear();
    nradded_ = sz;
    data_ = data;
    weights_ = wght;
}


template <class T>
inline bool ParallelCalc<T>::doPrepare( int nrthreads )
{
    if ( !data_ )
    {
	errmsg_ = tr("No data given to compute statistics");
	return false;
    }
    if ( nradded_ < 1 )
    {
	errmsg_ = tr("Data array is empty");
	return false;
    }

    const size_type nradded = nradded_;
    BaseCalc<T>::clear();
    nradded_ = nradded;
    meanval_ = meanval_w_ = 0.;
    variance_ = variance_w_ = 0.;
    delete udfarr_;
    mTryAlloc( udfarr_, bool[nradded] );
    if ( !udfarr_ )
    {
	errmsg_ = uiStrings::phrCannotAllocateMemory( nradded );
	return false;
    }

    barrier_.setNrThreads( nrthreads );

    return true;
}


template <class T>
inline bool ParallelCalc<T>::doWork( od_int64 start,
				     od_int64 stop, int thread )
{
    double sum_w = 0;
    double sum_wx = 0;
    double sum_wxx = 0;
    double sum_x = 0;
    double sum_xx = 0;
    idx_type minidx = 0;
    idx_type maxidx = 0;
    idx_type nrused = 0;

    bool* udfptr = udfarr_ + start;
    for ( ; start<=stop && mIsUdf(data_[start] ); start++ )
	*udfptr++ = true;

    idx_type idx = start;
    const T* dataptr = data_ + start;
    const T* stopptr = dataptr + (stop-start+1);

    T val = *dataptr;
    T tmin = val;
    T tmax = val;

    while ( dataptr < stopptr )
    {
	val = *dataptr++;
	idx ++;
	*udfptr = mIsUdf(val);
	if ( *udfptr++ )
	    continue;

	sum_x += val;
	sum_xx += val*val;
	if ( val < tmin )
	    { tmin = val; minidx = idx; }
	if ( val > tmax )
	    { tmax = val; maxidx = idx; }

	nrused ++;
    }

    if ( setup_.weighted_ && weights_ )
    {
	dataptr = data_ + start;
	udfptr = udfarr_ + start;
	const T* weightptr = weights_ ? weights_ + start : 0;
	while ( dataptr < stopptr )
	{
	    const T weight = *weightptr;
	    val = *dataptr;

	    dataptr ++;
	    weightptr ++;

	    if ( *udfptr++ )
		continue;

	    sum_w += weight;
	    sum_wx += weight * val;
	    sum_wxx += weight * val * val;
	}
    }

    barrier_.waitForAll( false );

    nrused_ += nrused;
    sum_x_ += (T)sum_x;
    sum_xx_ += (T)sum_xx;
    if ( setup_.weighted_ )
    {
	sum_w_ += (T)sum_w;
	sum_wx_ += (T)sum_wx;
	sum_wxx_ += (T)sum_wxx;
    }

    if ( ( mIsUdf(minval_) || minval_ > tmin ) &&  !mIsUdf(tmin ) )
	{ minval_ = tmin; minidx_ = minidx; }
    if ( ( mIsUdf(maxval_) || maxval_ < tmax ) && !mIsUdf(tmax) )
	{ maxval_ = tmax; maxidx_ = maxidx; }

    barrier_.mutex().unLock();

    barrier_.waitForAll( false );

    if ( nrused_ != 0 )
    {
	meanval_ = sum_x_ / nrused_;
	meanval_w_ = sum_wx_ / nrused_;
    }

    barrier_.mutex().unLock();

    barrier_.waitForAll( true );

    //The 2nd pass of the 2 pass variance
    if ( setup_.needvariance_ )
    {
	T tvariance = 0;
	T tvariance_w = 0;
	const int nrthreads = barrier_.nrThreads();
	const int nrperthread = nradded_/nrthreads;
	const idx_type startidx = thread*nrperthread;
	const idx_type stopidx = mMIN( startidx + nrperthread, nradded_)-1;

	dataptr = data_ + startidx;
	stopptr = dataptr + ( stopidx-startidx + 1 );
	udfptr = udfarr_ + startidx;

	if ( setup_.weighted_ && weights_ )
	{
	    const T* weightptr = weights_ ? weights_ + startidx : 0;
	    while ( dataptr < stopptr )
	    {
		const T weight = *weightptr++;
		val = *dataptr++;
		if ( *udfptr++ )
		    continue;

		T varval = val*weight - meanval_w_;
		varval *= varval;
		tvariance_w += varval;
	    }
	}
	else
	{
	    while ( dataptr < stopptr )
	    {
		val = *dataptr++;
		if ( *udfptr++ )
		    continue;

		T varval = val - meanval_;
		varval *= varval;
		tvariance += varval;
	    }
	}
	barrier_.waitForAll( false );

	variance_ += tvariance / (nrused_-1);
	variance_w_ += tvariance_w / (nrused_-1);

	barrier_.mutex().unLock();
    }

    return true;
}


template <>
inline bool ParallelCalc<float_complex>::doWork( od_int64, od_int64, int )
{
    pErrMsg("Undefined operation for float_complex in template");
    return false;
}


template <class T>
inline bool ParallelCalc<T>::doFinish( bool success )
{
    if ( !success )
	return false;

    if ( setup_.needmostfreq_ )
    {
	clss_.setSize( nrused_ );
	clsswt_.setSize( nrused_ );
	clsswt_.setAll( 1 );

	const bool* udfptr = udfarr_;
	for ( idx_type idx=0; idx<nradded_; idx++ )
	{
	    if ( *udfptr++ )
		continue;

	    const T val = data_[idx];
	    idx_type ival;
	    Conv::set( ival, val );
	    idx_type setidx = clss_.indexOf( ival );

	    if ( setup_.weighted_ && weights_ )
	    {
		const T wt = weights_[idx];
		if ( setidx < 0 )
		{
		    clss_[idx] = ival;
		    clsswt_[idx] = wt;
		}
		else
		    clsswt_[setidx] = wt;
	    }
	    else
	    {
		if ( setidx < 0 )
		    clss_[idx] = ival;
	    }
	}
    }

    if ( setup_.needmed_ || setup_.needsorted_ )
    {
	medvals_.setSize( nrused_, mUdf(float) );
	T* medvalsarr = medvals_.arr();
	const bool* udfptr = udfarr_;
	for ( idx_type idx=0; idx<nradded_; idx++ )
	{
	    if ( *udfptr++ )
		continue;

	    *medvalsarr++ = data_[idx];
	}
    }

    return true;
}


template <>
inline bool ParallelCalc<float_complex>::doFinish( bool )
{
    pErrMsg("Undefined operation for float_complex in template");
    return false;
}


template <class T> inline
const T* ParallelCalc<T>::sort( idx_type* idx_of_med )
{
    return BaseCalc<T>::sort( idx_of_med );

/*  TODO: uncomment when the ParallelSorter works
    T* valarr = medvals_.arr();
    const size_type sz = nrused_;
    LargeValVec<idx_type>& medidxs = BaseCalc<T>::medidxs_;
    const bool withidxs = idx_of_med || setup_.weighted_;
    if ( BaseCalc<T>::issorted_ &&
         ( (withidxs && medidxs.size()==sz) || (!withidxs) ) )
        return valarr;

    ParallelSorter<T>* sorter = nullptr;
    if ( withidxs )
    {
	if ( medidxs.size() != sz )
	{
	    medidxs.setSize( sz, 0 );
	    for ( idx_type idx=0; idx<sz; idx++ )
		medidxs[idx] = idx;
	}

	sorter = new ParallelSorter<T>( valarr, medidxs.arr(), sz );
    }
    else
	sorter = new ParallelSorter<T>( valarr, sz );

    sorter->execute();
    delete sorter;

    return valarr; */
}


template <> inline
const float_complex* ParallelCalc<float_complex>::sort( idx_type* idx_of_med )
{
    pErrMsg("Undefined operation for float_complex in template");
    return nullptr;
}


template <class T>
inline double ParallelCalc<T>::variance() const
{
    return setup_.weighted_ ? variance_w_ : variance_ ;
}


template <>
inline double ParallelCalc<float_complex>::variance() const
{
    pErrMsg("Undefined operation for float_complex in template");
    return mUdf(double);
}

} // namespace Stats
