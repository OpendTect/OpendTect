#ifndef statparallelcalc_h
#define statparallelcalc_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Kris and Bruno
Date:          Oct 2011
RCS:           $Id$
________________________________________________________________________

-*/

#include "math2.h"
#include "task.h"
#include "statruncalc.h"

/*!\brief Stats computation running in parallel. 

The difference with the running values (Stats::RunCalc) is that you have to 
pass the entire data array prior to the execution. 

It also works with optional weights.
-!*/


namespace Stats
{

template <class T>
mClass ParallelCalc : public ParallelTask, public BaseCalc<T>
{
public:
				ParallelCalc(const CalcSetup& s,const T* data, 
					     int sz,const T* weights = 0)
				    : BaseCalc<T>(s){setValues(data,sz,weights);}

				ParallelCalc( const CalcSetup& s )
				    : BaseCalc<T>(s) { setValues(0,0,0); }

    inline void 		setValues(const T* inp,int sz,const T* wght=0);

    const char*                 errMsg() const
				{ return errmsg_.isEmpty() ? 0 : errmsg_.buf();}

    virtual inline double 	variance() const;

    using BaseCalc<T>::medvals_;

protected:

    od_int64 			nrIterations() const    { return nradded_;}

    inline bool                 doPrepare(int);
    inline bool                 doWork(od_int64,od_int64,int);
    inline bool                 doFinish(bool);

    BufferString                errmsg_;

    mutable Threads::Barrier    barrier_;

    const T* 			data_;
    const T*                    weights_;

    T                           meanval_;
    T                           meanval_w_;
    T                           variance_;
    T                           variance_w_;

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
inline void ParallelCalc<T>::setValues( const T* data, int sz, const T* wght ) 
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
	{ errmsg_ = "No data given to compute statistics"; return false; }
    if ( nradded_ < 1 )
	{ errmsg_ = "Data array is empty"; return false; }

    variance_ = variance_w_ =0;

    barrier_.setNrThreads( nrthreads );

    return true;
}


template <class T> 
inline bool ParallelCalc<T>::doWork( od_int64 start, od_int64 stop, int thread )
{
    T sum_w = 0;
    T sum_wx = 0;
    T sum_wxx = 0;
    T sum_x = 0;
    T sum_xx = 0;
    int minidx = 0;
    int maxidx = 0;
    int nrused = 0;

    for ( ; start<=stop && mIsUdf(data_[start] ); start++ )
	/* just skip undefs at start */;

    int idx = start;
    const T* dataptr = data_ + start;
    const T* stopptr = dataptr + (stop-start+1);

    T val = *dataptr;
    T tmin = val;
    T tmax = val;

    while ( dataptr < stopptr )
    {
	val = *dataptr;
	dataptr++;
	idx ++;

	if ( mIsUdf( val ) )
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
	const T* weightptr = weights_ ? weights_ + start : 0;
	while ( dataptr < stopptr )
	{
	    const T weight = *weightptr;
	    val = *dataptr;

	    dataptr ++;
	    weightptr ++;

	    if ( mIsUdf( val ) )
		continue;

	    sum_w += weight;
	    sum_wx += weight * val;
	    sum_wxx += weight * val * val;
	}
    }

    barrier_.waitForAll( false );

    sum_x_ += sum_x;
    sum_xx_ += sum_xx;
    sum_w_ += sum_w;
    sum_wx_ += sum_wx;
    sum_wxx_ += sum_wxx;
    nrused_ += nrused;

    if ( ( mIsUdf(minval_) || minval_ > tmin ) &&  !mIsUdf(tmin ) )
	{ minval_ = tmin; minidx_ = minidx; }
    if ( ( mIsUdf(maxval_) || maxval_ < tmax ) && !mIsUdf(tmax) )
	{ maxval_ = tmax; maxidx_ = maxidx; }

    barrier_.mutex().unLock();

    barrier_.waitForAll( false );

    meanval_ = sum_x_ / nrused_;
    meanval_w_ = sum_wx_ / nrused_;

    barrier_.mutex().unLock();

    barrier_.waitForAll( true );


    //The 2nd pass of the 2 pass variance
    if ( setup_.needvariance_ )
    {
	T tvariance = 0;
	T tvariance_w = 0;
	const int nrthreads = barrier_.nrThreads();
	const int nrperthread = nradded_/nrthreads;
	const int startidx = thread*nrperthread;
	const int stopidx = mMIN( startidx + nrperthread, nradded_)-1;

	dataptr = data_ + startidx;
	stopptr = dataptr + ( stopidx-startidx + 1 );

	if ( setup_.weighted_ && weights_ )
	{
	    const T* weightptr = weights_ ? weights_ + startidx : 0;
	    while ( dataptr < stopptr )
	    {
		const T weight = *weightptr;
		val = *dataptr;

		dataptr ++;
		weightptr ++;

		if ( mIsUdf( val ) )
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
		val = *dataptr;

		dataptr++;

		if ( mIsUdf( val ) )
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

	for ( int idx=0; idx<nradded_; idx++ )
	{
	    const T val = data_[idx];
	    if ( mIsUdf( val ) )
		continue;

	    const T wt = weights_[idx];
	    int ival; Conv::set( ival, val );
	    int setidx = clss_.indexOf( ival );

	    if ( setidx < 0 )
		{ clss_[idx] = ival; clsswt_[idx] = wt; }
	    else
		clsswt_[setidx] = wt;
	}
    }

    if ( setup_.needmed_ )
    {
	for ( int idx=0; idx<nradded_; idx++ )
	{
	    const T val = data_[idx];
	    if ( !mIsUdf( val ) )
		medvals_ += val;
	}
    }

    return true;
}


template <class T> 
inline double ParallelCalc<T>::variance() const
{
    return setup_.weighted_ ? variance_w_ : variance_ ;
}


}
#endif
