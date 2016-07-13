#ifndef arrayndalgo_h
#define arrayndalgo_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________


@$*/

#include "algomod.h"
#include "arrayndimpl.h"
#include "coord.h"
#include "enums.h"
#include "mathfunc.h"
#include "periodicvalue.h"
#include "trckeysampling.h"
#include "uistrings.h"


namespace ArrayMath
{


/*!\brief Parallel task for computing the sum of all element of the array
	  Should not be used directly, instead call getSum(const ArrayND)
 */

template <class T>
class CumSumExec : public ParallelTask
{ mODTextTranslationClass(CumSumExec);
public:
		CumSumExec( const T* vals, od_int64 sz, bool noudf )
		    : vals_(vals)
		    , sz_(sz)
		    , noudf_(noudf)
		    , cumsum_(mUdf(T))
		{}

    uiString	uiNrDoneText() const	{ return ParallelTask::sPosFinished(); }
    uiString	uiMessage() const	{ return tr("Cumulative sum executor");}

    T		getSum() const		{ return cumsum_; }

protected:

    od_int64	nrIterations() const	{ return sz_; }

private:

    bool	doPrepare( int nrthreads )
		{
		    return sumvals_.setSize( nrthreads );
		}

    bool	doWork( od_int64 start, od_int64 stop, int threadidx )
		{
		    T sumval = 0;
		    od_int64 count = 0;
		    for ( int idx=mCast(int,start); idx<=stop; idx++ )
		    {
			const T value = vals_[idx];
			if ( !noudf_ && mIsUdf(value) )
			    continue;

			sumval += value;
			count++;
		    }

		    sumvals_[threadidx] = count==0 ? mUdf(T) : sumval;

		    return true;
		}

    bool	doFinish( bool success )
		{
		    if ( !success )
			return false;

		    int count = 0;
		    cumsum_ = 0;
		    for ( int idx=0; idx<sumvals_.size(); idx++ )
		    {
			if ( mIsUdf(sumvals_[idx]) )
			    continue;

			cumsum_ += sumvals_[idx];
			count++;
		    }

		    if ( count == 0 )
			cumsum_ = mUdf(T);

		    return true;
		}

    od_int64	sz_;
    bool	noudf_;

    TypeSet<T>	sumvals_;
    const T*	vals_;
    T		cumsum_;
};


/*!\brief returns the sum of all defined values in the Array.
   Returns UDF if empty or only udfs encountered. */

template <class T>
inline T getSum( const ArrayND<T>& in, bool noudf, bool parallel )
{
    const T* inpvals = in.getData();
    if ( inpvals )
    {
	CumSumExec<T> applycumsum( inpvals, in.info().getTotalSz(), noudf );
	if ( !applycumsum.executeParallel(parallel) )
	    return mUdf(T);

	return applycumsum.getSum();
    }

    T sum = 0; od_uint64 count = 0;
    ArrayNDIter iter( in.info() );
    do
    {
	const T value = in.getND( iter.getPos() );
	if ( !noudf && mIsUdf(value) )
	    continue;

	sum += value;
	count++;
    } while ( iter.next() );

    return count == 0 ? mUdf(T) : sum;
}


/*!\brief returns the average amplitude of the array */

template <class T>
inline T getAverage( const ArrayND<T>& in, bool noudf, bool parallel )
{
    const od_uint64 sz = in.info().getTotalSz();
    const T sumvals = getSum( in, noudf, parallel );
    return mIsUdf(sumvals) ? mUdf(T) : sumvals / mCast(T,sz);
}


//!Specialization for complex numbers.
template <>
inline float_complex getAverage<float_complex>(const ArrayND<float_complex>& in,
					       bool noudf, bool parallel )
{
    const od_uint64 sz = in.info().getTotalSz();
    const float_complex sumvals = getSum( in, noudf, parallel );
    return mIsUdf(sumvals) ? mUdf(float_complex) : sumvals / mCast(float,sz);
}



template <class T>
mDefParallelCalc5Pars(ScalingExec,
		  od_static_tr("ScalingExec","Array scaler executor"),
		  const T*,arrin,T*,arrout,T,fact,T,shift,bool,noudf)
mDefParallelCalcBody(,const T inpval = arrin_[idx]; \
		      if ( !noudf_ && ( mIsUdf(inpval) ) ) \
			  { arrout_[idx] = mUdf(T); continue; } \
		      arrout_[idx] = fact_ * inpval + shift_; , )

/*!\brief returns a scaled array */

template <class T>
inline void getScaled( const ArrayND<T>& in, ArrayND<T>* out_, T fact, T shift,
		       bool noudf, bool parallel )
{
    ArrayND<T>& out = out_ ? *out_ : const_cast<ArrayND<T>&>( in );
    const od_uint64 sz = in.info().getTotalSz();
    const T* inpvals = in.getData();
    T* outvals = out.getData();
    if ( inpvals && outvals )
    {
	ScalingExec<T> applyscaler( sz, inpvals, outvals, fact, shift, noudf );
	applyscaler.setReport( false );
	applyscaler.executeParallel( parallel );
	return;
    }

    ArrayNDIter iter( in.info() );
    do
    {
	const int* pos = iter.getPos();
	const T value = in.getND( pos );
	if ( !noudf && mIsUdf(value) )
	    continue;

	out.setND( pos, fact * value + shift );
    } while ( iter.next() );
}


template <class T>
mDefParallelCalc6Pars(SumExec,
		      od_static_tr("SumExec","Array addition executor"),
		  const T*,arr1,const T*,arr2,T*,out,T,fact1,T,fact2,bool,noudf)
mDefParallelCalcBody(,const T val1 = arr1_[idx]; const T val2 = arr2_[idx]; \
		      if ( !noudf_ && ( mIsUdf(val1) || mIsUdf(val2) ) ) \
			  { out_[idx] = mUdf(T); continue; } \
		      out_[idx] = fact1_ * val1 + fact2_ * val2; , )

/*!\brief computes the sum array between two arrays with scaling */

template <class T>
inline void getSum( const ArrayND<T>& in1, const ArrayND<T>& in2,
		    ArrayND<T>& out, T fact1, T fact2, bool noudf,
		    bool parallel )
{
    const od_uint64 sz = in1.info().getTotalSz();
    if ( in2.info().getTotalSz() != sz )
	return;

    const T* vals1 = in1.getData();
    const T* vals2 = in2.getData();
    T* outvals = out.getData();
    if ( vals1 && vals2 && outvals )
    {
	SumExec<T> applysum( sz, vals1, vals2, outvals, fact1, fact2,noudf);
	applysum.setReport( false );
	applysum.executeParallel( parallel );
	return;
    }

    ArrayNDIter iter( in1.info() );
    do
    {
	const int* pos = iter.getPos();
	const T val1 = in1.getND( pos );
	const T val2 = in2.getND( pos );
	if ( !noudf && ( mIsUdf(val1) || mIsUdf(val2) ) )
	    { out.setND( pos, mUdf(T) ); continue; }

	out.setND( pos, fact1 * val1 + fact2 * val2 );
    } while ( iter.next() );
}


template <class T>
mDefParallelCalc4Pars(ProdExec,
		      od_static_tr("ProdExec","Array product executor"),
		      const T*,arr1,const T*,arr2,T*,out,bool,noudf)
mDefParallelCalcBody(,const T val1 = arr1_[idx]; const T val2 = arr2_[idx]; \
		      if ( !noudf_ && ( mIsUdf(val1) || mIsUdf(val2) ) ) \
			  { out_[idx] = mUdf(T); continue; } \
		      out_[idx] = val1*val2; , )


/*!\brief computes the product array between two arrays */

template <class T>
inline void getProduct( const ArrayND<T>& in1, const ArrayND<T>& in2,
			ArrayND<T>& out, bool noudf, bool parallel )
{
    const od_uint64 sz = in1.info().getTotalSz();
    if ( in2.info().getTotalSz() != sz )
	return;

    const T* vals1 = in1.getData();
    const T* vals2 = in2.getData();
    T* outvals = out.getData();
    if ( vals1 && vals2 && outvals )
    {
	ProdExec<T> applyprod( sz, vals1, vals2, outvals, noudf );
	applyprod.setReport( false );
	applyprod.executeParallel( parallel );
	return;
    }

    ArrayNDIter iter( in1.info() );
    do
    {
	const int* pos = iter.getPos();
	const T val1 = in1.getND( pos );
	const T val2 = in2.getND( pos );
	if ( !noudf && ( mIsUdf(val1) || mIsUdf(val2) ) )
	    { out.setND( pos, mUdf(T) ); continue; }

	out.setND( pos, val1 * val2 );
    } while ( iter.next() );
}


/*!\brief computes the sum array between two arrays */

template <class T>
inline void getSum( const ArrayND<T>& in1, const ArrayND<T>& in2,
		    ArrayND<T>& out, bool noudf, bool parallel )
{ getSum( in1, in2, out, (T)1, (T)1, noudf, parallel ); }


/*!\brief returns the sum of product amplitudes between two vectors */

template <class T>
inline T getSumProduct( const ArrayND<T>& in1, const ArrayND<T>& in2,
			bool noudf, bool parallel )
{
    const od_uint64 sz = in1.info().getTotalSz();
    if ( in2.info().getTotalSz() != sz )
	return mUdf(T);

    Array1DImpl<T> prodvec( mCast(int,sz) );
    if ( !prodvec.isOK() )
	return mUdf(T);

    getProduct( in1, in2, prodvec, noudf, parallel );
    return getSum( prodvec, noudf, parallel );
}


/*!\brief returns the sum of squarred amplitudes of the array */

template <class T>
inline T getSumSq( const ArrayND<T>& in, bool noudf, bool parallel )
{ return getSumProduct( in, in, noudf, parallel ); }


/*!\brief return the Norm-2 of the array */

template <class T>
inline T getNorm2( const ArrayND<T>& in, bool noudf, bool parallel )
{
    const T sumsqvals = getSumSq( in, noudf, parallel );
    return mIsUdf(sumsqvals) ? mUdf(T) : Math::Sqrt( sumsqvals );
}


/*!\brief return the RMS of the array */

template <class T>
inline T getRMS( const ArrayND<T>& in, bool noudf, bool parallel )
{
    const od_uint64 sz = in.info().getTotalSz();
    const T sumsqvals = getSumSq( in, noudf, parallel );
    return mIsUdf(sumsqvals) ? mUdf(T) : Math::Sqrt( sumsqvals/mCast(T,sz) );
}


/*!\brief returns the residual differences of two arrays */

template <class T>
inline T getResidual( const ArrayND<T>& in1, const ArrayND<T>& in2, bool noudf,
		      bool parallel )
{
    const od_uint64 sz = in1.info().getTotalSz();
    if ( in2.info().getTotalSz() != sz )
	return mUdf(T);

    Array1DImpl<T> diffvec( mCast(int,sz) );
    if ( !diffvec.isOK() )
	return mUdf(T);

    getSum( in1, in2, diffvec, (T)1, (T)-1, noudf, parallel );
    T* diffdata = diffvec.getData();
    if ( diffdata )
    {
	for ( od_uint64 idx=0; idx<sz; idx++ )
	    diffdata[idx] = Math::Abs( diffdata[idx] );
    }
    else
    {
	ArrayNDIter iter( diffvec.info() );
	do
	{
	    const int* pos = iter.getPos();
	    diffvec.setND( pos, Math::Abs( diffvec.getND(pos) ) );
	} while ( iter.next() );
    }

    return getAverage( diffvec, noudf, parallel );
}


/*!\brief returns the sum of squarred differences of two arrays */

template <class T>
inline T getSumXMY2( const ArrayND<T>& in1, const ArrayND<T>& in2, bool noudf,
		     bool parallel )
{
    const od_uint64 sz = in1.info().getTotalSz();
    if ( in2.info().getTotalSz() != sz )
	return mUdf(T);

    Array1DImpl<T> sumvec( mCast(int,sz) );
    if ( !sumvec.isOK() )
	return mUdf(T);

    getSum( in1, in2, sumvec, (T)1, (T)-1, noudf, parallel );
    return getSumSq( sumvec, noudf, parallel );
}


/*!\brief returns the sum of summed squarred amplitudes of two arrays */

template <class T>
inline T getSumX2PY2( const ArrayND<T>& in1, const ArrayND<T>& in2, bool noudf,
		      bool parallel )
{
    const od_uint64 sz = in1.info().getTotalSz();
    if ( in2.info().getTotalSz() != sz )
	return mUdf(T);

    Array1DImpl<T> sqvec1( mCast(int,sz) ), sqvec2( mCast(int,sz) );
    if ( !sqvec1.isOK() || !sqvec2.isOK() )
	return mUdf(T);

    getProduct( in1, in1, sqvec1, noudf, parallel );
    getProduct( in2, in2, sqvec2, noudf, parallel );
    Array1DImpl<T> sumvec( mCast(int,sz) );
    if ( !sumvec.isOK() )
	return mUdf(T);

    getSum( sqvec1, sqvec2, sumvec, noudf, parallel );
    return getSum( sumvec, noudf, parallel );
}


/*!\brief returns the sum of subtracted squarred amplitudes of two arrays */

template <class T>
inline T getSumX2MY2( const ArrayND<T>& in1, const ArrayND<T>& in2, bool noudf,
		      bool parallel )
{
    const od_uint64 sz = in1.info().getTotalSz();
    if ( in2.info().getTotalSz() != sz )
	return mUdf(T);

    Array1DImpl<T> sqvec1( mCast(int,sz) ), sqvec2( mCast(int,sz) );
    if ( !sqvec1.isOK() || !sqvec2.isOK() )
	return mUdf(T);

    getProduct( in1, in1, sqvec1, noudf, parallel );
    getProduct( in2, in2, sqvec2, noudf, parallel );
    Array1DImpl<T> sumvec( mCast(int,sz) );
    if ( !sumvec.isOK() )
	return mUdf(T);

    getSum( sqvec1, sqvec2, sumvec, (T)1, (T)-1, noudf, parallel );
    return getSum( sumvec, noudf, parallel );
}


/*!\brief Fills an ArrayND with an unbiased version of another. */

template <class T, class fT>
inline bool removeBias( const ArrayND<T>& in, ArrayND<T>& out )
{
    const T averagevalue = getAverage( in, false, true );
    if ( mIsUdf(averagevalue) )
	return false;

    getScaled( in, &out, (T)1, -averagevalue, false, true );
    return true;
}


/*!\brief Removes the bias ( 0 order trend = average ) from an ArrayND. */

template <class T, class fT>
inline bool removeBias( ArrayND<T>& inout )
{
    const ArrayND<T>& inconst = const_cast<const ArrayND<T>&>( inout );
    return removeBias<T,fT>( inconst, inout );
}


/*!\brief returns the intercept and gradient of two arrays */

template <class T, class fT>
inline bool getInterceptGradient( const ArrayND<T>& iny, const ArrayND<T>* inx_,
				  T& intercept, T& gradient, bool parallel )
{
    const od_uint64 sz = iny.info().getTotalSz();
    T avgyvals = getAverage( iny, false, parallel );
    if ( mIsUdf(avgyvals) )
	return false;

    const bool hasxvals = inx_;
    const ArrayND<T>* inx = hasxvals ? inx_ : 0;
    if ( !hasxvals )
    {
	Array1DImpl<T>* inxtmp = new Array1DImpl<T>( mCast(int,sz) );
	if ( !inxtmp->isOK() )
	    { delete inxtmp; return false; }

	T* inxvals = inxtmp->getData();
	if ( inxvals )
	{
	    for ( od_uint64 idx=0; idx<sz; idx++ )
		inxvals[idx] = mCast(fT,idx);
	}
	else
	{
	    ArrayNDIter iter( inxtmp->info() );
	    od_uint64 idx = 0;
	    do
	    {
		inxtmp->setND( iter.getPos(), mCast(fT,idx) );
		idx++;
	    } while ( iter.next() );
	}

	inx = inxtmp;
    }

    T avgxvals = getAverage( *inx, false, parallel );
    if ( mIsUdf(avgxvals) )
	{ if ( !hasxvals) delete inx; return false; }

    ArrayND<T>& inyed = const_cast<ArrayND<T>&>( iny );
    ArrayND<T>& inxed = const_cast<ArrayND<T>&>( *inx );
    removeBias<T,fT>( inyed );
    removeBias<T,fT>( inxed );

    Array1DImpl<T> crossprodxy( mCast(int,sz) );
    if ( !crossprodxy.isOK() )
	{ if ( !hasxvals ) delete inx; return false; }

    getProduct( *inx, iny, crossprodxy, false, parallel );

    gradient = getSumProduct( *inx, iny, false, parallel ) /
	       getSumSq( *inx, false, parallel );
    intercept = avgyvals - gradient * avgxvals;
    getScaled( iny, &inyed, (T)1, avgyvals, false, parallel );

    if ( !hasxvals )
	delete inx;
    else
	getScaled( *inx, &inxed, (T)1, avgxvals, false, parallel );

    return true;
}


/*!\brief Fills an ArrayND with a de-trended version of another. */

template <class T, class fT>
inline bool removeTrend( const ArrayND<T>& in, ArrayND<T>& out )
{
    T intercept, gradient;
    if ( !getInterceptGradient<T,fT>(in,0,intercept,gradient,true) )
	return false;

    const od_uint64 sz = in.info().getTotalSz();
    Array1DImpl<T> trend( mCast(int,sz) );
    if ( !trend.isOK() )
	return false;

    T* trendvals = trend.getData();
    if ( trendvals )
    {
	for ( od_uint64 idx=0; idx<sz; idx++ )
	    trendvals[idx] = -( mCast(fT,idx)*gradient + intercept );
    }
    else
    {
	ArrayNDIter iter( trend.info() );
	od_uint64 idx = 0;
	do
	{
	    trend.setND( iter.getPos(), -( mCast(fT,idx)*gradient+intercept ) );
	    idx++;
	} while( iter.next() );
    }

    getSum( in, trend, out, false, true );

    return true;
}


/*!\brief Removes a 1st order (linear) trend from an ArrayND. */

template <class T, class fT>
inline bool removeTrend( ArrayND<T>& inout )
{
    const ArrayND<T>& inconst = const_cast<const ArrayND<T>&>( inout );
    return removeTrend<T,fT>( inconst, inout );
}

} // namespace ArrayMath


/*!\brief Returns whether there are undefs in the Array.  */

template <class fT>
inline bool hasUndefs( const ArrayND<fT>& in )
{
    const fT* vals = in.getData();
    if ( vals )
    {
	for ( od_uint64 idx=0; idx<in.info().getTotalSz(); idx++ )
	{
	    if ( mIsUdf(vals[idx]) )
		return true;
	}

	return false;
    }

    ArrayNDIter iter( in.info() );
    do
    {
	if ( mIsUdf(in.getND(iter.getPos())) )
	    return true;

    } while( iter.next() );

    return false;
}


/*!
   The function interpUdf fills all the undefined values in a Array1D
   by using an inter- or extrapolation from the defined values.
   It uses the BendPointBasedMathFunction for this.
   Note that even if there is only one defined value, this function will fill
   the entire array by this value.

   Returns whether any substitution was made.
*/

template <class fT>
inline bool interpUdf( Array1D<fT>& in,
	typename BendPointBasedMathFunction<fT,fT>::InterpolType ipoltyp=
			BendPointBasedMathFunction<fT,fT>::Poly )
{
    if ( !hasUndefs(in) )
	return false;

    BendPointBasedMathFunction<fT,fT> data( ipoltyp );
    const int sz = in.info().getSize(0);
    for ( int idx=0; idx<sz; idx++ )
    {
	const fT val = in.get( idx );
	if ( !mIsUdf(val) )
	    data.add( mCast(fT,idx), val );
    }

    for ( int idx=0; idx<sz; idx++ )
    {
	const fT val = in.get( idx );
	if ( mIsUdf(val) )
	    in.set( idx, data.getValue( mCast(fT,idx) ) );
    }

    return true;
}


/*!
\brief Tapers the N-dimentional ArrayND with a windowFunction.

  Usage is straightforward- construct and use. If apply()'s second argument is
  omitted, the result will be placed in the input array. apply() will return
  false if input-, output- and window-size are not equal.
  The only requirement on the windowfunction is that it should give full taper
  at x=+-1 and no taper when x=0. Feel free to implement more functions!!
*/

mExpClass(Algo) ArrayNDWindow
{
public:
    enum WindowType	{ Box, Hamming, Hanning, Blackman, Bartlett,
			  CosTaper5, CosTaper10, CosTaper20 };
			mDeclareEnumUtils(WindowType);

			ArrayNDWindow(const ArrayNDInfo&,bool rectangular,
				      WindowType=Hamming);
			ArrayNDWindow(const ArrayNDInfo&,bool rectangular,
				      const char* winnm,
				      float paramval=mUdf(float));
			~ArrayNDWindow();

    bool		isOK() const	{ return window_; }

    float		getParamVal() const { return paramval_; }
    float*		getValues() const { return window_; }

    void		setValue(int idx,float val) { window_[idx]=val; }
    bool		setType(WindowType);
    bool		setType(const char*,float paramval=mUdf(float));

    bool		resize(const ArrayNDInfo&);

    template <class Type> bool	apply(  ArrayND<Type>* in,
					ArrayND<Type>* out_=0 ) const
    {
	ArrayND<Type>* out = out_ ? out_ : in;

	if ( out_ && in->info() != out_->info() ) return false;
	if ( in->info() != size_ ) return false;

	const od_int64 totalsz = size_.getTotalSz();

	Type* indata = in->getData();
	Type* outdata = out->getData();
	if ( indata && outdata )
	{
	    for ( unsigned long idx = 0; idx<totalsz; idx++ )
	    {
		Type inval = indata[idx];
		outdata[idx] = mIsUdf( inval ) ? inval : inval * window_[idx];
	    }
	}
	else
	{
	    const ValueSeries<Type>* instorage = in->getStorage();
	    ValueSeries<Type>* outstorage = out->getStorage();

	    if ( instorage && outstorage )
	    {
		for ( unsigned long idx = 0; idx < totalsz; idx++ )
		{
		    Type inval = instorage->value(idx);
		    outstorage->setValue(idx,
			    mIsUdf( inval ) ? inval : inval * window_[idx] );
		}
	    }
	    else
	    {
		ArrayNDIter iter( size_ );
		int idx = 0;
		do
		{
		    Type inval = in->getND(iter.getPos());
		    out->setND( iter.getPos(),
			     mIsUdf( inval ) ? inval : inval * window_[idx] );
		    idx++;
		} while ( iter.next() );
	    }
	}

	return true;
    }

protected:

    float*			window_;
    ArrayNDInfoImpl		size_;
    bool			rectangular_;

    BufferString		windowtypename_;
    float			paramval_;

    bool			buildWindow(const char* winnm,float pval);
};


template<class T>
inline T Array3DInterpolate( const Array3D<T>& array,
			     float p0, float p1, float p2,
			     bool posperiodic = false )
{
    const Array3DInfo& size = array.info();
    int intpos0 = mNINT32( p0 );
    float dist0 = p0 - intpos0;
    int prevpos0 = intpos0;
    if ( dist0 < 0 )
    {
	prevpos0--;
	dist0++;
    }
    if ( posperiodic ) prevpos0 = dePeriodize( prevpos0, size.getSize(0) );

    int intpos1 = mNINT32( p1 );
    float dist1 = p1 - intpos1;
    int prevpos1 = intpos1;
    if ( dist1 < 0 )
    {
	prevpos1--;
	dist1++;
    }
    if ( posperiodic ) prevpos1 = dePeriodize( prevpos1, size.getSize(1) );

    int intpos2 = mNINT32( p2 );
    float dist2 = p2 - intpos2;
    int prevpos2 = intpos2;
    if ( dist2 < 0 )
    {
	prevpos2--;
	dist2++;
    }

    if ( posperiodic ) prevpos2 = dePeriodize( prevpos2, size.getSize(2) );

    if ( !posperiodic && ( prevpos0 < 0 || prevpos0 > size.getSize(0) -2 ||
			 prevpos1 < 0 || prevpos1 > size.getSize(1) -2 ||
			 prevpos2 < 0 || prevpos2 > size.getSize(2) -2 ))
	return mUdf(T);

    if ( !posperiodic && ( !prevpos0 || prevpos0 > size.getSize(0) -3 ||
			 !prevpos1 || prevpos1 > size.getSize(1) -3 ||
			 !prevpos2 || prevpos2 > size.getSize(2) -3 ))
    {
	return linearInterpolate3D(
            array.get( prevpos0  , prevpos1  , prevpos2  ),
	    array.get( prevpos0  , prevpos1  , prevpos2+1),
	    array.get( prevpos0  , prevpos1+1, prevpos2  ),
            array.get( prevpos0  , prevpos1+1, prevpos2+1),
            array.get( prevpos0+1, prevpos1  , prevpos2  ),
            array.get( prevpos0+1, prevpos1  , prevpos2+1),
            array.get( prevpos0+1, prevpos1+1, prevpos2  ),
            array.get( prevpos0+1, prevpos1+1, prevpos2+1),
	    dist0, dist1, dist2 );
    }

    int firstpos0 = prevpos0 - 1;
    int nextpos0 = prevpos0 + 1;
    int lastpos0 = prevpos0 + 2;

    if ( posperiodic ) firstpos0 = dePeriodize( firstpos0, size.getSize(0) );
    if ( posperiodic ) nextpos0 = dePeriodize( nextpos0, size.getSize(0) );
    if ( posperiodic ) lastpos0 = dePeriodize( lastpos0, size.getSize(0) );

    int firstpos1 = prevpos1 - 1;
    int nextpos1 = prevpos1 + 1;
    int lastpos1 = prevpos1 + 2;

    if ( posperiodic ) firstpos1 = dePeriodize( firstpos1, size.getSize(1) );
    if ( posperiodic ) nextpos1 = dePeriodize( nextpos1, size.getSize(1) );
    if ( posperiodic ) lastpos1 = dePeriodize( lastpos1, size.getSize(1) );

    int firstpos2 = prevpos2 - 1;
    int nextpos2 = prevpos2 + 1;
    int lastpos2 = prevpos2 + 2;

    if ( posperiodic ) firstpos2 = dePeriodize( firstpos2, size.getSize(2) );
    if ( posperiodic ) nextpos2 = dePeriodize( nextpos2, size.getSize(2) );
    if ( posperiodic ) lastpos2 = dePeriodize( lastpos2, size.getSize(2) );

    return polyInterpolate3D (
            array.get( firstpos0  , firstpos1  , firstpos2 ),
            array.get( firstpos0  , firstpos1  , prevpos2  ),
            array.get( firstpos0  , firstpos1  , nextpos2  ),
            array.get( firstpos0  , firstpos1  , lastpos2  ),

            array.get( firstpos0  , prevpos1   , firstpos2 ),
            array.get( firstpos0  , prevpos1   , prevpos2  ),
            array.get( firstpos0  , prevpos1   , nextpos2  ),
            array.get( firstpos0  , prevpos1   , lastpos2  ),

            array.get( firstpos0  , nextpos1   , firstpos2 ),
            array.get( firstpos0  , nextpos1   , prevpos2  ),
            array.get( firstpos0  , nextpos1   , nextpos2  ),
            array.get( firstpos0  , nextpos1   , lastpos2  ),

            array.get( firstpos0  , lastpos1   , firstpos2 ),
            array.get( firstpos0  , lastpos1   , prevpos2  ),
            array.get( firstpos0  , lastpos1   , nextpos2  ),
            array.get( firstpos0  , lastpos1   , lastpos2  ),


            array.get( prevpos0  , firstpos1  , firstpos2 ),
            array.get( prevpos0  , firstpos1  , prevpos2  ),
            array.get( prevpos0  , firstpos1  , nextpos2  ),
            array.get( prevpos0  , firstpos1  , lastpos2  ),

            array.get( prevpos0  , prevpos1   , firstpos2 ),
            array.get( prevpos0  , prevpos1   , prevpos2  ),
            array.get( prevpos0  , prevpos1   , nextpos2  ),
            array.get( prevpos0  , prevpos1   , lastpos2  ),

            array.get( prevpos0  , nextpos1   , firstpos2 ),
            array.get( prevpos0  , nextpos1   , prevpos2  ),
            array.get( prevpos0  , nextpos1   , nextpos2  ),
            array.get( prevpos0  , nextpos1   , lastpos2  ),

            array.get( prevpos0  , lastpos1   , firstpos2 ),
            array.get( prevpos0  , lastpos1   , prevpos2  ),
            array.get( prevpos0  , lastpos1   , nextpos2  ),
            array.get( prevpos0  , lastpos1   , lastpos2  ),


            array.get( nextpos0  , firstpos1  , firstpos2 ),
            array.get( nextpos0  , firstpos1  , prevpos2  ),
            array.get( nextpos0  , firstpos1  , nextpos2  ),
            array.get( nextpos0  , firstpos1  , lastpos2  ),

            array.get( nextpos0  , prevpos1   , firstpos2 ),
            array.get( nextpos0  , prevpos1   , prevpos2  ),
            array.get( nextpos0  , prevpos1   , nextpos2  ),
            array.get( nextpos0  , prevpos1   , lastpos2  ),

            array.get( nextpos0  , nextpos1   , firstpos2 ),
            array.get( nextpos0  , nextpos1   , prevpos2  ),
            array.get( nextpos0  , nextpos1   , nextpos2  ),
            array.get( nextpos0  , nextpos1   , lastpos2  ),

            array.get( nextpos0  , lastpos1   , firstpos2 ),
            array.get( nextpos0  , lastpos1   , prevpos2  ),
            array.get( nextpos0  , lastpos1   , nextpos2  ),
            array.get( nextpos0  , lastpos1   , lastpos2  ),


            array.get( lastpos0  , firstpos1  , firstpos2 ),
            array.get( lastpos0  , firstpos1  , prevpos2  ),
            array.get( lastpos0  , firstpos1  , nextpos2  ),
            array.get( lastpos0  , firstpos1  , lastpos2  ),

            array.get( lastpos0  , prevpos1   , firstpos2 ),
            array.get( lastpos0  , prevpos1   , prevpos2  ),
            array.get( lastpos0  , prevpos1   , nextpos2  ),
            array.get( lastpos0  , prevpos1   , lastpos2  ),

            array.get( lastpos0  , nextpos1   , firstpos2 ),
            array.get( lastpos0  , nextpos1   , prevpos2  ),
            array.get( lastpos0  , nextpos1   , nextpos2  ),
            array.get( lastpos0  , nextpos1   , lastpos2  ),

            array.get( lastpos0  , lastpos1   , firstpos2 ),
            array.get( lastpos0  , lastpos1   , prevpos2  ),
            array.get( lastpos0  , lastpos1   , nextpos2  ),
            array.get( lastpos0  , lastpos1   , lastpos2  ),
	    dist0, dist1, dist2 );
}


template <class T>
inline bool ArrayNDCopy( ArrayND<T>& dest, const ArrayND<T>& src,
			 const TypeSet<int>& copypos, bool srcperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int ndim = destsz.getNDim();
    if ( ndim != srcsz.getNDim() || ndim != copypos.size() ) return false;

    for ( int idx=0; idx<ndim; idx++ )
    {
	if ( !srcperiodic &&
	     copypos[idx] + destsz.getSize(idx) > srcsz.getSize(idx) )
	    return false;
    }

    ArrayNDIter destposition( destsz );
    TypeSet<int> srcposition( ndim, 0 );

    do
    {
	for ( int idx=0; idx<ndim; idx++ )
	{
	    srcposition[idx] = copypos[idx] + destposition[idx];
	    if ( srcperiodic )
		srcposition[idx] =
		    dePeriodize( srcposition[idx], srcsz.getSize(idx) );
	}

	dest.setND( destposition.getPos(), src.get( &srcposition[0] ));


    } while ( destposition.next() );

    return true;
}


template <class T>
inline bool Array3DCopy( Array3D<T>& dest, const Array3D<T>& src,
			 int p0, int p1, int p2, bool srcperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int destsz0 = destsz.getSize(0);
    const int destsz1 = destsz.getSize(1);
    const int destsz2 = destsz.getSize(2);

    const int srcsz0 = srcsz.getSize(0);
    const int srcsz1 = srcsz.getSize(1);
    const int srcsz2 = srcsz.getSize(2);

    if ( !srcperiodic )
    {
	if ( p0 + destsz0 > srcsz0 ||
	     p1 + destsz1 > srcsz1 ||
	     p2 + destsz2 > srcsz2  )
	     return false;
    }

    int idx = 0;
    T* ptr = dest.getData();

    for ( int id0=0; id0<destsz0; id0++ )
    {
	for ( int id1=0; id1<destsz1; id1++ )
	{
	    for ( int id2=0; id2<destsz2; id2++ )
	    {
		ptr[idx++] = src.get( dePeriodize(id0 + p0, srcsz0),
					 dePeriodize(id1 + p1, srcsz1),
					 dePeriodize(id2 + p2, srcsz2));

	    }
	}
    }

    return true;
}


template <class T>
inline bool ArrayNDPaste( ArrayND<T>& dest, const ArrayND<T>& src,
			  const TypeSet<int>& pastepos,
			  bool destperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int ndim = destsz.getNDim();
    if ( ndim != srcsz.getNDim() || ndim != pastepos.size() ) return false;

    for ( int idx=0; idx<ndim; idx++ )
    {
	if ( !destperiodic &&
	     pastepos[idx] + srcsz.getSize(idx) > destsz.getSize(idx) )
	    return false;
    }

    ArrayNDIter srcposition( srcsz );
    TypeSet<int> destposition( ndim, 0 );

    int ptrpos = 0;
    T* ptr = src.getData();

    do
    {
	for ( int idx=0; idx<ndim; idx++ )
	{
	    destposition[idx] = pastepos[idx] + srcposition[idx];
	    if ( destperiodic )
		destposition[idx] =
		    dePeriodize( destposition[idx], destsz.getSize(idx) );
	}

	dest( destposition ) =  ptr[ptrpos++];

    } while ( srcposition.next() );

    return true;
}


template <class T>
inline bool Array2DPaste( Array2D<T>& dest, const Array2D<T>& src,
			  int p0, int p1, bool destperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int srcsz0 = srcsz.getSize(0);
    const int srcsz1 = srcsz.getSize(1);

    const int destsz0 = destsz.getSize(0);
    const int destsz1 = destsz.getSize(1);

    if ( !destperiodic )
    {
	if ( p0 + srcsz0 > destsz0 ||
	     p1 + srcsz1 > destsz1  )
	     return false;
    }


    int idx = 0;
    const T* ptr = src.getData();

    for ( int id0=0; id0<srcsz0; id0++ )
    {
	for ( int id1=0; id1<srcsz1; id1++ )
	{
	    dest.set( dePeriodize( id0 + p0, destsz0),
		  dePeriodize( id1 + p1, destsz1),
		  ptr[idx++]);
	}
    }

    return true;
}


template <class T>
inline bool Array3DPaste( Array3D<T>& dest, const Array3D<T>& src,
			  int p0, int p1, int p2,
			  bool destperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int srcsz0 = srcsz.getSize(0);
    const int srcsz1 = srcsz.getSize(1);
    const int srcsz2 = srcsz.getSize(2);

    const int destsz0 = destsz.getSize(0);
    const int destsz1 = destsz.getSize(1);
    const int destsz2 = destsz.getSize(2);

    if ( !destperiodic )
    {
	if ( p0 + srcsz0 > destsz0 ||
	     p1 + srcsz1 > destsz1 ||
	     p2 + srcsz2 > destsz2  )
	     return false;
    }


    int idx = 0;
    const T* ptr = src.getData();

    for ( int id0=0; id0<srcsz0; id0++ )
    {
	for ( int id1=0; id1<srcsz1; id1++ )
	{
	    for ( int id2=0; id2<srcsz2; id2++ )
	    {
		dest.set( dePeriodize( id0 + p0, destsz0),
		      dePeriodize( id1 + p1, destsz1),
		      dePeriodize( id2 + p2, destsz2), ptr[idx++]);
	    }
	}
    }

    return true;
}


/*!\brief Transfers the common samples from one 2D array to another */

template <class T>
mClass(Algo) Array2DCopier : public ParallelTask
{ mODTextTranslationClass(Array2DCopier)
public:
		Array2DCopier( const Array2D<T>& in,
			       const TrcKeySampling& tksin,
			       const TrcKeySampling& tksout,
			       Array2D<T>& out )
		    : in_(in)
		    , tksin_(tksin)
		    , tksout_(tksout)
		    , out_(out)
		{
		    if ( canCopyAll() )
		    {
			doPrepare(0);
			return;
		    }

		    tksin.getInterSection( tksout, commontks_ );
		}

    uiString	uiNrDoneText() const
		{
		    return uiStrings::phrJoinStrings(
						uiStrings::sInline(mPlural),
						uiStrings::sDone().toLower() );
		}
    uiString	uiMessage() const	{ return tr("Transferring grid data");}

protected:

    od_int64	nrIterations() const
		{
		    return canCopyAll() ? 0 : commontks_.nrLines();
		}

private:

    bool	canCopyAll() const
		{
		    return tksout_ == tksin_ && in_.getData() &&
			   ( out_.getData() || out_.getStorage() );
		}

    bool	doPrepare( int )
		{
		    if ( in_.info().getSize(0) != tksin_.nrLines() ||
			 in_.info().getSize(1) != tksin_.nrTrcs() )
		    {
			return false;
		    }

		    if ( out_.info().getSize(0) != tksout_.nrLines() ||
			 out_.info().getSize(1) != tksout_.nrTrcs() )
		    {
			mDynamicCastGet(Array2DImpl<T>*,outimpl,&out_)
			if ( !outimpl || !outimpl->setSize( tksout_.nrLines(),
							    tksout_.nrTrcs() ) )
			{
			    return false;
			}

			out_.setAll( mUdf(T) );
		    }

		    if ( canCopyAll() )
		    {
			if ( out_.getData() )
			    in_.getAll( out_.getData() );
			else if ( out_.getStorage() )
			    in_.getAll( *out_.getStorage() );
		    }

		    return true;
		}

    bool	doWork( od_int64 start, od_int64 stop, int )
		{
		    const TrcKeySampling tksin( tksin_ );
		    const TrcKeySampling tksout( tksout_ );
		    const TrcKeySampling tks( commontks_ );

		    const bool usearrayptrs = in_.getData() && out_.getData() &&
					      in_.getStorage() &&
					      out_.getStorage();
		    OffsetValueSeries<T>* invals = !usearrayptrs ? 0 :
			    new OffsetValueSeries<T>( *in_.getStorage(), 0 );
		    OffsetValueSeries<T>* outvals = !usearrayptrs ? 0 :
			    new OffsetValueSeries<T>( *out_.getStorage(), 0 );
		    const int nrcrl = tks.nrTrcs();
		    const od_int64 nrbytes = nrcrl * sizeof(T);

		    const int startcrl = tks.start_.crl();
		    const int startcrlidyin = tksin.trcIdx( startcrl );
		    const int startcrlidyout = tksout.trcIdx( startcrl );
		    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++)
		    {
			const int inl = tks.lineRange().atIndex( idx );
			const int inlidxin = tksin.lineIdx( inl );
			const int inlidxout = tksout.lineIdx( inl );
			if ( usearrayptrs )
			{
			    invals->setOffset(
			      in_.info().getOffset(inlidxin,startcrlidyin) );
			    outvals->setOffset(
			      out_.info().getOffset(inlidxout,startcrlidyout) );
			    OD::memCopy(outvals->arr(),invals->arr(),nrbytes);
			    continue;
			}
			else
			{
			    for ( int idy=0; idy<nrcrl; idy++ )
			    {
				const float val =
					in_.get( inlidxin, startcrlidyin+idy );
				out_.set( inlidxout, startcrlidyout+idy, val );
			    }
			}
		    }

		    delete invals;
		    delete outvals;

		    return true;
		}

    const Array2D<T>&	in_;
    const TrcKeySampling&	tksin_;
    const TrcKeySampling&	tksout_;
    TrcKeySampling	commontks_;
    Array2D<T>&		out_;
};


/*!
  \brief Polynomial trend with order 0 (mean), 1 (linear) or 2 (parabolic)
  The trend is derived from a set of values with positions
  and can be applied thereafter on any other position
*/

mExpClass(Algo) PolyTrend
{
public:
				PolyTrend();

    bool			operator==(const PolyTrend&) const;

				enum Order	{ None, Order0, Order1, Order2};
				mDeclareEnumUtils(Order)

    static const char*		sKeyOrder()	{ return "Polynomial Order"; }

    void			setOrder( PolyTrend::Order t )	{ order_ = t; }
    template <class IDXABLE> bool	set(const TypeSet<Coord>&,
					    const IDXABLE& valuelistj);
				/*!< Use after the order is set!
				     Sets the trend from a list of values
				     tied to a list of coordinates */

    Order			getOrder() const	{ return order_; }

    template <class T> void	apply(const Coord& pos,bool dir,T&) const;
				/*!<Applies the trend to a value tied to
				    a position */
				/*!<\param dir: true for detrend,
						false for restore */

protected:

    Order			order_;
    double			f0_;
    double			f1_;
    double			f2_;
    double			f11_;
    double			f12_;
    double			f22_;
    Coord			posc_;

    void			initOrder0(const TypeSet<double>&);
    void			initOrder1(const TypeSet<Coord>&,
					   const TypeSet<double>&);
    void			initOrder2(const TypeSet<Coord>&,
					   const TypeSet<double>&);
    void			initCenter(const TypeSet<Coord>&);

};



template <class IDXABLE> inline
bool PolyTrend::set( const TypeSet<Coord>& poslist, const IDXABLE& vals )
{
    int sz = poslist.size();
    if ( order_ == PolyTrend::None )
	return false;

    f0_ = f1_ = f2_ = f11_ = f12_ = f22_ = posc_.x = posc_.y = 0.;
    TypeSet<Coord> posnoudf;
    TypeSet<double> valsnoudf;
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !poslist[idx].isDefined() || mIsUdf(vals[idx]) )
	    continue;

	posnoudf += poslist[idx];
	valsnoudf += (double) vals[idx];
    }

    sz = valsnoudf.size();
    if ( order_ == Order2 && sz > 5 )
	initOrder2( posnoudf, valsnoudf );
    else if ( order_ == Order1 && sz > 2 )
	initOrder1( posnoudf, valsnoudf );
    else
	initOrder0( valsnoudf );

    return true;
}


template <class T> inline
void PolyTrend::apply( const Coord& pos, bool dir, T& val ) const
{
    if ( order_ == None || !pos.isDefined() || mIsUdf(val) )
	return;

    const double fact = dir ? -1. : 1;
    double inp = (double) val;
    inp += fact * f0_;
    if ( order_ == Order0 )
    {
	val = (T)inp;
	return;
    }

    const double dx = pos.x - posc_.x;
    const double dy = pos.y - posc_.y;
    inp += fact * ( f1_ * dx + f2_ * dy );
    if ( order_ == Order1 )
    {
	val = (T)inp;
	return;
    }

    const double dx2 = dx * dx;
    const double dxy = dx * dy;
    const double dyy = dy * dy;
    inp += fact * ( f11_ * dx2 + f12_ * dxy + f22_ * dyy );
    val = (T)inp;
}

#endif
