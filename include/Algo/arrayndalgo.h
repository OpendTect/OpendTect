#pragma once

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
#include "cubedata.h"
#include "cubesubsel.h"
#include "enums.h"
#include "horsubsel.h"
#include "linesdata.h"
#include "linesubsel.h"
#include "mathfunc.h"
#include "periodicvalue.h"
#include "posinfo.h"
#include "survsubsel.h"
#include "uistrings.h"
#include "valseries.h"


namespace ArrayMath
{

mClass(Algo) ArrayOperExecSetup
{
public:
		ArrayOperExecSetup(bool doadd=true,bool dosqinp=false,
				   bool dosqout=false,bool doabs=false,
				   bool donormalizesum=false,
				   bool dosqrtsum=false)
		    : doadd_(doadd)
		    , dosqinp_(dosqinp)
		    , dosqout_(dosqout)
		    , doabs_(doabs)
		    , donormalizesum_(donormalizesum)
		    , dosqrtsum_(dosqrtsum)
		{}

    bool	doadd_;
    bool	dosqinp_;
    bool	dosqout_;
    bool	doabs_;
    bool	donormalizesum_;
    bool	dosqrtsum_;
};


/*!\brief Parallel task for computing the sum of element wise operations of
	  one array and optionally a second input array.
	  Should not be used directly, instead call getSum(const ArrayND&)
	  Template parameter SumType should be double (real or complex) for
	  all float types. Should be od_int64 for all integer types.
	  Template parameter OperType should be double (real) for all float
	  types. Should be od_int64 for all integer types.
 */

template <class ArrType,class SumType,class OperType,class RetType>
class CumArrOperExec : public ParallelTask
{ mODTextTranslationClass(CumArrOperExec);
public:
		CumArrOperExec( const ArrayND<ArrType>& xvals, bool noudf,
				const ArrayOperExecSetup& setup )
		    : xarr_(xvals)
		    , yarr_(0)
		    , sz_(xvals.totalSize())
		    , noudf_(noudf)
		    , xshift_(mUdf(SumType))
		    , yshift_(mUdf(SumType))
		    , xfact_(mUdf(OperType))
		    , yfact_(mUdf(OperType))
		    , cumsum_(mUdf(SumType))
		    , count_(0)
		    , setup_(setup)
		{}

    uiString	nrDoneText() const	{ return sPositionsDone(); }
    uiString	message() const	{ return tr("Cumulative sum executor");}

    void	setYVals( const ArrayND<ArrType>& yvals ) { yarr_ = &yvals; }
		/*!< Apply a shift on the input values after exponentiation
		     but before scaling (if applicable) */
    void	setShift( SumType shift, bool forx=true )
		{
		    if ( forx )
			xshift_ = shift;
		    else
			yshift_ = shift;
		}
		/*!< Apply a scaling on the input values after exponentiation
		     and shift (if applicable) */
    void	setScaler( OperType scaler, bool forx=true )
		{
		   if ( forx )
		      xfact_ = scaler;
		   else
		      yfact_ = scaler;
		}

    RetType	getSum() const		{ return mCast(RetType,cumsum_); }

protected:

    od_int64	nrIterations() const	{ return sz_; }

private:

    bool	doPrepare( int nrthreads )
		{
		    if ( yarr_ && yarr_->info().totalSize() != sz_ )
			return false;

		    cumsum_ = (SumType)0;
		    count_ = 0;

		    return true;
		}

    bool	doWork( od_int64 start, od_int64 stop, int threadidx )
    {
	SumType sumval = 0, comp = 0;
	od_int64 count = 0;
	const ArrType* xvals = xarr_.getData();
	const ArrType* yvals = yarr_ ? yarr_->getData() : 0;
	const ValueSeries<ArrType>* xstor = xarr_.getStorage();
	const ValueSeries<ArrType>* ystor = yarr_ ? yarr_->getStorage() : 0;
	ArrayNDIter* xiter = xvals || xstor
			   ? 0 : new ArrayNDIter( xarr_.info() );
	ArrayNDIter* yiter = ( yarr_ && ( yvals || ystor ) ) || !yarr_
			   ? 0 : new ArrayNDIter( yarr_->info() );
	if ( xiter ) xiter->setGlobalPos( start );
	if ( yiter ) yiter->setGlobalPos( start );
	const bool doshiftxvals = !mIsUdf(xshift_);
	const bool doscalexvals = !mIsUdf(xfact_);
	const bool hasyvals = yarr_;
	const bool doshiftyvals = !mIsUdf(yshift_);
	const bool doscaleyvals = !mIsUdf(yfact_);
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    SumType xvalue = xvals ? xvals[idx]
				   : xstor ? xstor->value(idx)
					   : xarr_.getND( xiter->getPos() );
	    if ( !noudf_ && mIsUdf(xvalue) )
	    {
		if ( xiter ) xiter->next();
		if ( yiter ) yiter->next();
		continue;
	    }

	    if ( setup_.dosqinp_ ) xvalue *= xvalue;
	    if ( doshiftxvals ) xvalue += xshift_;
	    if ( doscalexvals ) xvalue *= xfact_;
	    if ( hasyvals )
	    {
		SumType yvalue = yvals ? yvals[idx]
				       : ystor ? ystor->value(idx)
					       : yarr_->getND( yiter->getPos());
		if ( !noudf_ && mIsUdf(yvalue) )
		{
		    if ( xiter ) xiter->next();
		    if ( yiter ) yiter->next();
		    continue;
		}

		if ( setup_.dosqinp_ ) yvalue *= yvalue;
		if ( doshiftyvals ) yvalue += yshift_;
		if ( doscaleyvals ) yvalue *= yfact_;
		if ( setup_.doadd_ )
		    xvalue += yvalue;
		else
		    xvalue *= yvalue;
	    }

	    if ( setup_.doabs_ )
		xvalue = Math::Abs( xvalue );
	    else if ( setup_.dosqout_ )
		xvalue *= xvalue;

	    xvalue -= comp;
	    const SumType t = sumval + xvalue;
	    comp = ( t - sumval ) - xvalue;
	    sumval = t;
	    count++;
	    if ( xiter ) xiter->next();
	    if ( yiter ) yiter->next();
	}

	delete xiter; delete yiter;
	if ( count < 1 )
	    return true;

	Threads::Locker locker( writelock_ );
	cumsum_ += sumval;
	count_ += count;

	return true;
    }

    bool	doFinish( bool success )
		{
		    if ( !success || count_ == 0 )
		    {
			cumsum_ = mUdf(SumType);
			return false;
		    }

		    if ( setup_.donormalizesum_ )
			cumsum_ /= mCast(OperType,count_);

		    if ( setup_.dosqrtsum_ )
			cumsum_ = Math::Sqrt( cumsum_ );

		    return true;
		}

private:

    const ArrayOperExecSetup&	setup_;
    ArrayNDInfo::total_size_type sz_;
    bool			noudf_;

    Threads::Lock		writelock_;
    const ArrayND<ArrType>&	xarr_;
    const ArrayND<ArrType>*	yarr_;
    SumType		xshift_;
    SumType		yshift_;
    OperType		xfact_;
    OperType		yfact_;
    SumType		cumsum_;
    od_int64		count_;

};


#define mSetResAndContinue(res) \
{ \
    if ( outvals ) \
	outvals[idx] = res; \
    else if ( outstor ) \
	outstor->setValue( idx, res );\
    else \
	outarr_.setND( outiter->getPos(), res ); \
    \
    if ( xiter ) xiter->next(); \
    if ( yiter ) yiter->next(); \
    if ( outiter ) outiter->next(); \
}

/*!\brief Parallel task for computing the element wise operations of
	  one array and optionally a second input array.
	  Should not be used directly, instead call getSum(const ArrayND)
 */

template <class ArrType,class SumType,class OperType>
class ArrOperExec : public ParallelTask
{ mODTextTranslationClass(ArrOperExec);
public:
		ArrOperExec( const ArrayND<ArrType>& xvals,
			     const ArrayND<ArrType>* yvals, bool noudf,
			     const ArrayOperExecSetup& setup,
			     ArrayND<ArrType>& outvals )
		    : xarr_(xvals)
		    , yarr_(yvals)
		    , outarr_(outvals)
		    , sz_(xvals.info().totalSize())
		    , noudf_(noudf)
		    , xfact_(mUdf(OperType))
		    , yfact_(mUdf(OperType))
		    , shift_(mUdf(OperType))
		    , setup_(setup)
		{}

    uiString	nrDoneText() const	{ return sPositionsDone(); }
    uiString	message() const	{ return tr("Cumulative sum executor");}

    void	setYVals( const ArrayND<ArrType>& yvals ) { yarr_ = &yvals; }
    void	setScaler( OperType scaler, bool forx=true )
		{
		   if ( forx )
		      xfact_ = scaler;
		   else
		      yfact_ = scaler;
		}
    void	setShift( SumType shift )	{ shift_ = shift; }

protected:

    od_int64	nrIterations() const	{ return sz_; }

private:

    bool	doPrepare( int )
		{
		    if ( outarr_.info().totalSize() != sz_ )
			return false;

		    if ( yarr_ && yarr_->info().totalSize() != sz_ )
			return false;

		    return true;
		}

    bool	doWork( od_int64 start, od_int64 stop, int )
    {
	const ArrType* xvals = xarr_.getData();
	const ArrType* yvals = yarr_ ? yarr_->getData() : 0;
	ArrType* outvals = outarr_.getData();
	const ValueSeries<ArrType>* xstor = xarr_.getStorage();
	const ValueSeries<ArrType>* ystor = yarr_ ? yarr_->getStorage() : 0;
	ValueSeries<ArrType>* outstor = outarr_.getStorage();
	ArrayNDIter* xiter = xvals || xstor
			   ? 0 : new ArrayNDIter( xarr_.info() );
	ArrayNDIter* yiter = ( yarr_ && ( yvals || ystor ) ) || !yarr_
			   ? 0 : new ArrayNDIter( yarr_->info() );
	ArrayNDIter* outiter = outvals || outstor
			     ? 0 : new ArrayNDIter( outarr_.info() );
	if ( xiter ) xiter->setGlobalPos( start );
	if ( yiter ) yiter->setGlobalPos( start );
	if ( outiter ) outiter->setGlobalPos( start );
	const bool doscalexvals = !mIsUdf(xfact_);
	const bool hasyvals = yarr_;
	const bool doscaleyvals = !mIsUdf(yfact_);
	const bool doshiftoutvals = !mIsUdf(shift_);
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    SumType xvalue = xvals ? xvals[idx]
				   : xstor ? xstor->value(idx)
					   : xarr_.getND( xiter->getPos() );
	    if ( !noudf_ && mIsUdf(xvalue) )
		{ mSetResAndContinue( mUdf(ArrType) ) continue;	}

	    if ( doscalexvals ) xvalue *= xfact_;
	    if ( hasyvals )
	    {
		SumType yvalue = yvals ? yvals[idx]
				       : ystor ? ystor->value(idx)
					       : yarr_->getND( yiter->getPos());
		if ( !noudf_ && mIsUdf(yvalue) )
		    { mSetResAndContinue( mUdf(ArrType) ) continue; }

		if ( doscaleyvals ) yvalue *= yfact_;
		if ( setup_.doadd_ )
		    xvalue += yvalue;
		else
		    xvalue *= yvalue;
	    }

	    if ( doshiftoutvals )
		xvalue += shift_;

	    mSetResAndContinue( mCast(ArrType,xvalue) )
	}

	delete xiter; delete yiter; delete outiter;

	return true;
    }

private:

    const ArrayOperExecSetup&	setup_;
    od_int64		sz_;
    bool		noudf_;

    const ArrayND<ArrType>&	xarr_;
    const ArrayND<ArrType>*	yarr_;
    ArrayND<ArrType>&	outarr_;
    OperType		xfact_;
    OperType		yfact_;
    SumType		shift_;
};


/*!\brief returns the sum of all defined values in the Array.
   Returns UDF if empty or only udfs encountered. */

template <class ArrType,class SumType,class OperType,class RetType>
inline RetType getSum( const ArrayND<ArrType>& in, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    CumArrOperExec<ArrType,SumType,OperType,RetType> sumexec( in, noudf, setup);
    if ( !sumexec.executeParallel(parallel) )
	return mUdf(RetType);

    return sumexec.getSum();
}


/*!\brief returns the average amplitude of the array */

template <class ArrType,class SumType,class OperType,class RetType>
inline RetType getAverage( const ArrayND<ArrType>& in, bool noudf,
			   bool parallel )
{
    ArrayOperExecSetup setup;
    setup.donormalizesum_ = true;
    CumArrOperExec<ArrType,SumType,OperType,RetType> avgexec( in, noudf, setup);
    if ( !avgexec.executeParallel(parallel) )
	return mUdf(ArrType);

    return avgexec.getSum();
}


/*!\brief returns a scaled array */

template <class ArrType,class SumType,class OperType>
inline void getScaled( const ArrayND<ArrType>& in, ArrayND<ArrType>* out_,
		       OperType fact, SumType shift, bool noudf, bool parallel )
{
    ArrayND<ArrType>& out = out_ ? *out_ : const_cast<ArrayND<ArrType>&>( in );
    ArrayOperExecSetup setup;
    ArrOperExec<ArrType,SumType,OperType> scalinngexec( in, 0, noudf, setup,
							out );
    scalinngexec.setScaler( fact );
    scalinngexec.setShift( shift );
    scalinngexec.executeParallel( parallel );
}


/*!\brief computes the sum array between two arrays */

template <class ArrType,class SumType,class OperType>
inline void getSum( const ArrayND<ArrType>& in1, const ArrayND<ArrType>& in2,
		    ArrayND<ArrType>& out, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    ArrOperExec<ArrType,SumType,OperType> sumexec( in1, &in2, noudf, setup,
						   out );
    sumexec.executeParallel( parallel );
}


/*!\brief computes the sum array between two arrays with scaling */

template <class ArrType,class SumType,class OperType>
inline void getSum( const ArrayND<ArrType>& in1, const ArrayND<ArrType>& in2,
		    ArrayND<ArrType>& out, OperType fact1, OperType fact2,
		    bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    ArrOperExec<ArrType,SumType,OperType> sumexec( in1, &in2, noudf, setup,
						   out );
    sumexec.setScaler( fact1 );
    sumexec.setScaler( fact2, false );
    sumexec.executeParallel( parallel );
}


/*!\brief computes the product array between two arrays */

template <class ArrType,class SumType,class OperType>
inline void getProduct( const ArrayND<ArrType>& in1,
			const ArrayND<ArrType>& in2, ArrayND<ArrType>& out,
			bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.doadd_ = false;
    ArrOperExec<ArrType,SumType,OperType> prodexec( in1, &in2, noudf, setup,
						    out );
    prodexec.executeParallel( parallel );
}


/*!\brief returns the sum of product amplitudes between two vectors */

template <class ArrType,class SumType,class OperType,class RetType>
inline RetType getSumProduct( const ArrayND<ArrType>& in1,
			      const ArrayND<ArrType>& in2,
			      bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.doadd_ = false;
    CumArrOperExec<ArrType,SumType,OperType,RetType> sumprodexec( in1, noudf,
								  setup );
    sumprodexec.setYVals( in2 );
    if ( !sumprodexec.executeParallel(parallel) )
	return mUdf(RetType);

    return sumprodexec.getSum();
}


/*!\brief returns the sum of squarred amplitudes of the array */

template <class ArrType,class SumType,class OperType,class RetType>
inline RetType getSumSq( const ArrayND<ArrType>& in, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    CumArrOperExec<ArrType,SumType,OperType,RetType> sumsqexec( in, noudf,
								setup );
    if ( !sumsqexec.executeParallel(parallel) )
	return mUdf(RetType);

    return sumsqexec.getSum();
}


/*!\brief return the Norm-2 of the array */

template <class ArrType,class SumType,class OperType,class RetType>
inline RetType getNorm2( const ArrayND<ArrType>& in, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    setup.dosqrtsum_ = true;
    CumArrOperExec<ArrType,SumType,OperType,RetType> norm2exec( in, noudf,
								setup );
    if ( !norm2exec.executeParallel(parallel) )
	return mUdf(RetType);

    return norm2exec.getSum();
}


/*!\brief return the RMS of the array */

template <class ArrType,class SumType,class OperType,class RetType>
inline RetType getRMS( const ArrayND<ArrType>& in, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    setup.donormalizesum_ = true;
    setup.dosqrtsum_ = true;
    CumArrOperExec<ArrType,SumType,OperType,RetType> rmsexec( in, noudf, setup);
    if ( !rmsexec.executeParallel(parallel) )
	return mUdf(RetType);

    return rmsexec.getSum();
}


/*!\brief returns the residual differences of two arrays */

template <class ArrType,class SumType,class OperType,class RetType>
inline RetType getResidual( const ArrayND<ArrType>& in1,
			    const ArrayND<ArrType>& in2, bool noudf,
			    bool parallel )
{
    ArrayOperExecSetup setup;
    setup.doabs_ = true;
    setup.donormalizesum_ = true;
    CumArrOperExec<ArrType,SumType,OperType,RetType> residualexec( in1, noudf,
								   setup );
    residualexec.setYVals( in2 );
    residualexec.setScaler( (OperType)-1, false );
    if ( !residualexec.executeParallel(parallel) )
	return mUdf(RetType);

    return residualexec.getSum();
}


/*!\brief returns the sum of squarred differences of two arrays */

template <class ArrType,class SumType,class OperType,class RetType>
inline RetType getSumXMY2( const ArrayND<ArrType>& in1,
			   const ArrayND<ArrType>& in2, bool noudf,
			   bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqout_ = true;
    CumArrOperExec<ArrType,SumType,OperType,RetType> sumxmy2exec( in1, noudf,
								  setup );
    sumxmy2exec.setYVals( in2 );
    sumxmy2exec.setScaler( (SumType)-1, false );
    if ( !sumxmy2exec.executeParallel(parallel) )
	return mUdf(RetType);

    return sumxmy2exec.getSum();
}


/*!\brief returns the sum of summed squarred amplitudes of two arrays */

template <class ArrType,class SumType,class OperType,class RetType>
inline RetType getSumX2PY2( const ArrayND<ArrType>& in1,
			    const ArrayND<ArrType>& in2, bool noudf,
			    bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    CumArrOperExec<ArrType,SumType,OperType,RetType> sumx2py2exec( in1, noudf,
								   setup );
    sumx2py2exec.setYVals( in2 );
    if ( !sumx2py2exec.executeParallel(parallel) )
	return mUdf(RetType);

    return sumx2py2exec.getSum();
}


/*!\brief returns the sum of subtracted squarred amplitudes of two arrays */

template <class ArrType,class SumType,class OperType,class RetType>
inline RetType getSumX2MY2( const ArrayND<ArrType>& in1,
			    const ArrayND<ArrType>& in2, bool noudf,
			    bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    CumArrOperExec<ArrType,SumType,OperType,RetType> sumx2my2exec( in1, noudf,
								   setup );
    sumx2my2exec.setYVals( in2 );
    sumx2my2exec.setScaler( (SumType)-1, false );
    if ( !sumx2my2exec.executeParallel(parallel) )
	return mUdf(RetType);

    return sumx2my2exec.getSum();
}


/*!\brief Fills an ArrayND with an unbiased version of another. */

template <class ArrType,class SumType,class OperType>
inline bool removeBias( const ArrayND<ArrType>& in, ArrayND<ArrType>& out,
			bool noudf, bool parallel )
{
    const SumType averagevalue =
	    getAverage<ArrType,SumType,OperType,SumType>( in, noudf, parallel );
    if ( mIsUdf(averagevalue) )
	return false;

    getScaled<ArrType,SumType,OperType>( in, &out, (OperType)1, -averagevalue,
					 noudf, parallel );
    return true;
}


/*!\brief Removes the bias ( 0 order trend = average ) from an ArrayND. */

template <class ArrType,class SumType,class OperType>
inline bool removeBias( ArrayND<ArrType>& inout, bool noudf, bool parallel )
{
    const ArrayND<ArrType>& inconst =
			    const_cast<const ArrayND<ArrType>&>( inout );
    return removeBias<ArrType,SumType,OperType>( inconst, inout, noudf,
						 parallel );
}


/*!\brief returns the intercept and gradient of two arrays */

template <class ArrType,class OperType>
inline bool getInterceptGradient( const ArrayND<ArrType>& iny,
				  const ArrayND<ArrType>& inx,
				  OperType& intercept, OperType& gradient,
				  bool parallel )
{
    const OperType avgyvals = getAverage<ArrType,OperType,OperType,OperType>(
					 iny, false, parallel );
    if ( mIsUdf(avgyvals) )
	return false;

    const OperType avgxvals = getAverage<ArrType,OperType,OperType,OperType>(
					 inx, false, parallel );
    if ( mIsUdf(avgxvals) )
	return false;

    ArrayOperExecSetup numsetup, denomsetup;
    numsetup.doadd_ = false;
    denomsetup.dosqout_ = true;
    CumArrOperExec<ArrType,OperType,OperType,OperType> numgradexec( inx, false,
								    numsetup );
    numgradexec.setYVals( iny );
    numgradexec.setShift( -avgxvals );
    numgradexec.setShift( -avgyvals, false );
    CumArrOperExec<ArrType,OperType,OperType,OperType> denomgradexec( inx,false,
								    denomsetup);
    denomgradexec.setShift( -avgxvals );
    if ( !numgradexec.executeParallel(parallel) ||
	 !denomgradexec.executeParallel(parallel) )
	return false;

    gradient = numgradexec.getSum() / denomgradexec.getSum();
    intercept = avgyvals - gradient * avgxvals;

    return true;
}


/*!\brief Fills an ArrayND with a de-trended version of another. */

template <class ArrType,class OperType>
inline bool removeTrend( const ArrayND<ArrType>& in, ArrayND<ArrType>& out )
{
    if ( in.isEmpty() )
	return true;

    ArrayND<ArrType>* trendx = ArrayNDImpl<ArrType>::create( in.info() );
    if ( !trendx->isOK() )
	{ delete trendx; return false; }

    ArrType* startptr = trendx->getData();
    ArrType* endptr = startptr + trendx->totalSize();
    for ( ArrType* ptrval=startptr; ptrval!=endptr; ptrval++ )
	*ptrval = (ArrType)(ptrval-startptr);

    OperType intercept=0, gradient=0;
    const bool gotgrad = getInterceptGradient<ArrType,OperType>(
				in, *trendx, intercept, gradient, true );
    if ( gotgrad )
    {
	getScaled<ArrType,OperType,OperType>( *trendx, 0, gradient, intercept,
					      true, true );
	getSum<ArrType,OperType,OperType>( in, *trendx, out, (OperType)1,
					   (OperType)(-1), false, true );
    }

    delete trendx;
    return gotgrad;
}


/*!\brief Removes a 1st order (linear) trend from an ArrayND. */

template <class ArrType,class OperType>
inline bool removeTrend( ArrayND<ArrType>& inout )
{
    const ArrayND<ArrType>& inconst =
			    const_cast<const ArrayND<ArrType>&>( inout );
    return removeTrend<ArrType,OperType>( inconst, inout );
}

} // namespace ArrayMath


/*!\brief Returns whether there are undefs in the Array.  */

template <class fT>
inline bool hasUndefs( const ArrayND<fT>& in )
{
    const fT* vals = in.getData();
    typedef ArrayNDInfo::total_size_type total_size_type;
    const total_size_type sz = in.totalSize();
    if ( vals )
    {
	for ( total_size_type idx=0; idx<sz; idx++ )
	{
	    if ( mIsUdf(vals[idx]) )
		return true;
	}

	return false;
    }

    const ValueSeries<fT>* stor = in.getStorage();
    if ( stor )
    {
	for ( total_size_type idx=0; idx<sz; idx++ )
	{
	    if ( mIsUdf(stor->value(idx)) )
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


/*! fills all the undefined values in a Array1D
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
    typedef ArrayNDInfo::idx_type idx_type;
    typedef ArrayNDInfo::size_type size_type;
    const size_type sz = in.getSize( 0 );
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	const fT val = in.get( idx );
	if ( !mIsUdf(val) )
	    data.add( mCast(fT,idx), val );
    }

    for ( idx_type idx=0; idx<sz; idx++ )
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
			mTypeDefArrNDTypes;

    enum WindowType	{ Box, Hamming, Hanning, Blackman, Bartlett,
			  CosTaper5, CosTaper10, CosTaper20 };
			mDeclareEnumUtils(WindowType);

			ArrayNDWindow(const ArrayNDInfo&,bool rectangular,
				      WindowType=Hamming);
			ArrayNDWindow(const ArrayNDInfo&,bool rectangular,
				      const char* winnm,
				      float paramval=mUdf(float));
			ArrayNDWindow(const ArrayNDInfo&,bool rectangular,
				      const char* winnm,
				      const TypeSet<float>& paramvals);
			~ArrayNDWindow();

    bool		isOK() const		{ return window_; }

    float		getParamVal(int dim=0) const { return paramval_[dim]; }
    void		setParamVal(int dim=0, float paramval=mUdf(float));
    void		setParamVals(const TypeSet<float>&);
    float*		getValues() const	{ return window_; }

    void		setValue(od_int64 idx,float val) { window_[idx]=val; }
    bool		setType(WindowType);
    bool		setType(const char*,float paramval=mUdf(float));
    bool		setType(const char*,const TypeSet<float>&);

    bool		resize(const ArrayNDInfo&);

    template <class Type> bool	apply(  ArrayND<Type>* in,
					ArrayND<Type>* out_=0 ) const
    {
	ArrayND<Type>* out = out_ ? out_ : in;

	if ( out_ && in->info() != out_->info() )
	    return false;
	if ( in->info() != arrinfo_ )
	    return false;

	const total_size_type totalsz = arrinfo_.totalSize();

	Type* indata = in->getData();
	Type* outdata = out->getData();
	if ( indata && outdata )
	{
	    for ( total_size_type idx=0; idx<totalsz; idx++ )
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
		for ( total_size_type idx=0; idx<totalsz; idx++ )
		{
		    Type inval = instorage->value( idx );
		    outstorage->setValue( idx,
			    mIsUdf( inval ) ? inval : inval * window_[idx] );
		}
	    }
	    else
	    {
		ArrayNDIter iter( arrinfo_ );
		od_int64 idx = 0;
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
    ArrayNDInfoImpl		arrinfo_;
    bool			rectangular_;

    BufferString		windowtypename_;
    TypeSet<float>		paramval_;

    bool			buildWindow(const char* winnm,float pval);
    bool			buildWindow(const char* winnm);
};


template<class T>
inline T Array3DInterpolate( const Array3D<T>& array,
			     float p0, float p1, float p2,
			     bool posperiodic = false )
{
    typedef ArrayNDInfo::idx_type idx_type;
    const Array3DInfo& size = array.info();
    idx_type intpos0 = roundOff<idx_type>( p0 );
    float dist0 = p0 - intpos0;
    idx_type prevpos0 = intpos0;
    if ( dist0 < 0 )
    {
	prevpos0--;
	dist0++;
    }
    if ( posperiodic ) prevpos0 = dePeriodize( prevpos0, size.getSize(0) );

    idx_type intpos1 = roundOff<idx_type>( p1 );
    float dist1 = p1 - intpos1;
    idx_type prevpos1 = intpos1;
    if ( dist1 < 0 )
	{ prevpos1--; dist1++; }
    if ( posperiodic )
	prevpos1 = dePeriodize( prevpos1, size.getSize(1) );

    idx_type intpos2 = roundOff<idx_type>( p2 );
    float dist2 = p2 - intpos2;
    idx_type prevpos2 = intpos2;
    if ( dist2 < 0 )
	{ prevpos2--; dist2++; }
    if ( posperiodic )
	prevpos2 = dePeriodize( prevpos2, size.getSize(2) );

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

    idx_type firstpos0 = prevpos0 - 1;
    idx_type nextpos0 = prevpos0 + 1;
    idx_type lastpos0 = prevpos0 + 2;

    if ( posperiodic )
	firstpos0 = dePeriodize( firstpos0, size.getSize(0) );
    if ( posperiodic )
	nextpos0 = dePeriodize( nextpos0, size.getSize(0) );
    if ( posperiodic )
	lastpos0 = dePeriodize( lastpos0, size.getSize(0) );

    idx_type firstpos1 = prevpos1 - 1;
    idx_type nextpos1 = prevpos1 + 1;
    idx_type lastpos1 = prevpos1 + 2;

    if ( posperiodic )
	firstpos1 = dePeriodize( firstpos1, size.getSize(1) );
    if ( posperiodic )
	nextpos1 = dePeriodize( nextpos1, size.getSize(1) );
    if ( posperiodic )
	lastpos1 = dePeriodize( lastpos1, size.getSize(1) );

    idx_type firstpos2 = prevpos2 - 1;
    idx_type nextpos2 = prevpos2 + 1;
    idx_type lastpos2 = prevpos2 + 2;

    if ( posperiodic )
	firstpos2 = dePeriodize( firstpos2, size.getSize(2) );
    if ( posperiodic )
	nextpos2 = dePeriodize( nextpos2, size.getSize(2) );
    if ( posperiodic )
	lastpos2 = dePeriodize( lastpos2, size.getSize(2) );

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


template <class T,class IdxType>
inline bool ArrayNDCopyPeriodic( ArrayND<T>& dest, const ArrayND<T>& src,
				 ArrayNDInfo::NDPos copypos )
{
    const ArrayNDInfo& srcsz = src.info();

    const auto ndim = dest.nrDims();
    if ( ndim != srcsz.nrDims() )
	return false;

    ArrayNDIter destiter( dest );
    mDefNDPosBuf( srcposition, ndim );

    do
    {
	for ( ArrayNDInfo::dim_idx_type idx=0; idx<ndim; idx++ )
	    srcposition[idx] = dePeriodize( copypos[idx] + destiter[idx],
					    srcsz.getSize(idx) );
	dest.setND( destiter.getPos(), src.get( srcposition ));

    } while ( destiter.next() );

    return true;
}


template <class T>
inline bool Array3DCopyPeriodic( Array3D<T>& dest, const Array3D<T>& src,
     ArrayNDInfo::idx_type p0, ArrayNDInfo::idx_type p1,
     ArrayNDInfo::idx_type p2 )
{
    if ( src.isEmpty() )
	return true;

    typedef ArrayNDInfo::size_type size_type;
    typedef ArrayNDInfo::idx_type idx_type;
    const size_type destsz0 = dest.getSize( 0 );
    const size_type destsz1 = dest.getSize( 1 );
    const size_type destsz2 = dest.getSize( 2 );
    const size_type srcsz0 = src.getSize( 0 );
    const size_type srcsz1 = src.getSize( 1 );
    const size_type srcsz2 = src.getSize( 2 );

    T* ptr = dest.getData();
    if ( !ptr )
    {
	ArrayNDIter it( src );
	do {
	    ArrayNDInfo::NDPos srcpos = it.getPos();
	    dest.setND( it.getPos(),
			src.get( dePeriodize( srcpos[0]+p0, srcsz0 ),
				 dePeriodize( srcpos[1]+p1, srcsz1 ),
				 dePeriodize( srcpos[2]+p2, srcsz2 ) ) );
	} while ( it.next() );
    }

    for ( idx_type id0=0; id0<destsz0; id0++ )
    {
	for ( idx_type id1=0; id1<destsz1; id1++ )
	{
	    for ( idx_type id2=0; id2<destsz2; id2++ )
	    {
		*ptr = src.get( dePeriodize( id0+p0, srcsz0 ),
				dePeriodize( id1+p1, srcsz1 ),
				dePeriodize( id2+p2, srcsz2 ) );
		ptr++;

	    }
	}
    }

    return true;
}


template <class T>
inline bool ArrayNDPaste( ArrayND<T>& dest, const ArrayND<T>& src,
			  ArrayNDInfo::NDPos pastepos, bool destperiodic=false )
{
    const auto ndim = dest.nrDims();
    if ( src.isEmpty() || dest.isEmpty() || ndim != src.nrDims() )
	return false;

    typedef ArrayNDInfo::dim_idx_type dim_idx_type;
    for ( dim_idx_type idx=0; idx<ndim; idx++ )
    {
	if ( !destperiodic &&
	     pastepos[idx] + src.getSize(idx) > dest.getSize(idx) )
	    return false;
    }

    ArrayNDIter srciter( src );
    mDefNDPosBuf( destpos, ndim );
    do
    {
	for ( dim_idx_type idx=0; idx<ndim; idx++ )
	{
	    destpos[idx] = pastepos[idx] + srciter[idx];
	    if ( destperiodic )
		destpos[idx] = dePeriodize( destpos[idx], dest.getSize(idx) );
	}
	dest.setND( destpos, src.getND(srciter.getPos()) );

    } while ( srciter.next() );

    return true;
}


template <class T>
inline bool Array2DPaste( Array2D<T>& dest, const Array2D<T>& src,
			  ArrayNDInfo::idx_type p0, ArrayNDInfo::idx_type p1,
			  bool destperiodic=false )
{
    const T* ptr = src.getData();
    if ( !ptr )
    {
	mDefNDPosBuf( posbuf, 2 );
	posbuf[0] = p0; posbuf[1] = p1;
	ArrayNDPaste( dest, src, mNDPosFromPosBuf(posbuf), destperiodic );
    }

    typedef ArrayNDInfo::size_type size_type;
    typedef ArrayNDInfo::idx_type idx_type;
    const size_type srcsz0 = src.getSize( 0 );
    const size_type srcsz1 = src.getSize( 1 );
    const size_type destsz0 = dest.getSize( 0 );
    const size_type destsz1 = dest.getSize( 1 );

    if ( !destperiodic
      && ( p0 + srcsz0 > destsz0
	|| p1 + srcsz1 > destsz1 ) )
	 return false;

    for ( idx_type id0=0; id0<srcsz0; id0++ )
    {
	for ( idx_type id1=0; id1<srcsz1; id1++ )
	{
	    dest.set( dePeriodize(id0 + p0,destsz0),
		      dePeriodize(id1 + p1,destsz1), *ptr );
	    ptr++;
	}
    }

    return true;
}


template <class T>
inline bool Array3DPaste( Array3D<T>& dest, const Array3D<T>& src,
			  ArrayNDInfo::idx_type p0, ArrayNDInfo::idx_type p1,
			  ArrayNDInfo::idx_type p2, bool destperiodic=false )
{
    const T* ptr = src.getData();
    if ( !ptr )
    {
	mDefNDPosBuf( posbuf, 3 );
	posbuf[0] = p0; posbuf[1] = p1; posbuf[2] = p2;
	ArrayNDPaste( dest, src, mNDPosFromPosBuf(posbuf), destperiodic );
    }

    typedef ArrayNDInfo::size_type size_type;
    typedef ArrayNDInfo::idx_type idx_type;
    const size_type srcsz0 = src.getSize( 0 );
    const size_type srcsz1 = src.getSize( 1 );
    const size_type srcsz2 = src.getSize( 2 );
    const size_type destsz0 = dest.getSize( 0 );
    const size_type destsz1 = dest.getSize( 1 );
    const size_type destsz2 = dest.getSize( 2 );

    if ( !destperiodic
      && ( p0 + srcsz0 > destsz0
	|| p1 + srcsz1 > destsz1
	|| p2 + srcsz2 > destsz2 ) )
	 return false;

    for ( idx_type id0=0; id0<srcsz0; id0++ )
    {
	for ( idx_type id1=0; id1<srcsz1; id1++ )
	{
	    for ( idx_type id2=0; id2<srcsz2; id2++ )
	    {
		dest.set( dePeriodize( id0+p0, destsz0 ),
			  dePeriodize( id1+p1, destsz1 ),
			  dePeriodize( id2+p2, destsz2 ), *ptr );
		ptr++;
	    }
	}
    }

    return true;
}


/*!<Replaces the undefined samples in a 2D/3D array. Optionally provides
    the list of replaced samples.
    If a PosInfo::CubeData is provided the samples where traces are not present
    will not be substituted
 */

template <class T>
mClass(Algo) ArrayUdfValReplacer : public ParallelTask
{ mODTextTranslationClass(ArrayUdfValReplacer)
public:

ArrayUdfValReplacer( Array2D<T>& inp, LargeValVec<od_int64>* undefidxs )
    : ParallelTask("Array Udf Replacer")
    , inp_(inp)
    , undefidxs_(undefidxs)
    , totalnr_(inp.totalSize()/inp.getSize(1))
{}

ArrayUdfValReplacer( Array3D<T>& inp, LargeValVec<od_int64>* undefidxs )
    : ParallelTask("Array Udf Replacer")
    , inp_(inp)
    , undefidxs_(undefidxs)
    , totalnr_(inp.totalSize()/inp.getSize(2))
{}

uiString message() const	{ return tr("Replacing undefined values"); }
uiString nrDoneText() const	{ return sTracesDone(); }

void setReplacementValue( T val )	{ replval_ = val; }

void setSampling( const Survey::HorSubSel& hss,
		  const PosInfo::LineCollData* lcd )
{
    lhss_ = hss.asLineHorSubSel();
    chss_ = hss.asCubeHorSubSel();
    lcd_ = lcd;
    lcd2d_ = lcd ? lcd->asLinesData() : nullptr;
    lcd3d_ = lcd ? lcd->asCubeData() : nullptr;
}

protected:

od_int64 nrIterations() const override	{ return totalnr_; }

private:

bool doPrepare( int ) override
{
    if ( undefidxs_ )
	undefidxs_->setEmpty();

    return true;
}


bool doWork( od_int64 start, od_int64 stop, int ) override
{
    const bool isrect = lcd_ && (lhss_ || chss_)
		      ? lcd_->isFullyRegular() : true;
    const ArrayNDInfo& info = inp_.info();
    const int nrtrcsp = info.getSize( inp_.get1DDim() );
    T* dataptr = inp_.getData();
    ValueSeries<T>* datastor = inp_.getStorage();
    const bool hasarrayptr = dataptr;
    const bool hasstorage = datastor;
    const bool neediterator = !hasarrayptr && !hasstorage;
    const od_int64 offset = start * nrtrcsp;
    dataptr += offset;
    od_int64 validx = offset;
    ArrayNDIter* iter = neediterator
		      ? new ArrayNDIter( info ) : 0;
    if ( iter )
	iter->setGlobalPos( offset );

    const T replval = replval_;
    for ( od_int64 idx=start; idx<=stop; idx++,
					 quickAddToNrDone(idx))
    {
	const bool hastrcdata = isrect ? true
			      : (lcd2d_ ? lcd2d_->hasPosition(*lhss_,idx)
					: lcd3d_->hasPosition(*chss_,idx));
	if ( hastrcdata )
	{
	    for ( int idz=0; idz<nrtrcsp; idz++ )
	    {
		const int* pos = iter ? iter->getPos() : 0;
		const T val = hasarrayptr ? *dataptr
			    : hasstorage
				? datastor->value( validx )
				: inp_.getND( pos );
		if ( !mIsUdf(val) )
		{
		    if ( hasarrayptr ) dataptr++;
		    else if ( hasstorage ) validx++;
		    else iter->next();

		    continue;
		}

		if ( undefidxs_ )
		{
		    lck_.lock();
		    *undefidxs_ += idx*nrtrcsp + idz;
		    lck_.unLock();
		}

		if ( hasarrayptr )
		    *dataptr++ = replval;
		else if ( hasstorage )
		    datastor->setValue( validx++, replval );
		else
		{
		    inp_.setND( pos, replval );
		    iter->next();
		}
	    }
	}
	else
	{
	    if ( hasarrayptr )
	    {
		dataptr =
		OD::sysMemValueSet( dataptr, replval, nrtrcsp );
	    }
	    else if ( hasstorage )
	    {
		for ( int idz=0; idz<nrtrcsp; idz++ )
		    datastor->setValue( validx++, replval );
	    }
	    else
	    {
		for ( int idz=0; idz<nrtrcsp; idz++ )
		{
		    inp_.setND( iter->getPos(), replval );
		    iter->next();
		}
	    }
	}
    }

    delete iter;

    return true;
}

    ArrayND<T>&			inp_;
    T				replval_	= 0.f;
    LargeValVec<od_int64>*	undefidxs_	= nullptr;
    const LineHorSubSel*	lhss_		= nullptr;
    const CubeHorSubSel*	chss_		= nullptr;
    const PosInfo::LineCollData*	lcd_	= nullptr;
    const PosInfo::LinesData*	lcd2d_		= nullptr;
    const PosInfo::CubeData*	lcd3d_		= nullptr;
    const od_int64		totalnr_;
    Threads::Mutex		lck_;

};


/*!<Replaces undefined values back to an ND array */

template <class T>
mClass(Algo) ArrayUdfValRestorer : public ParallelTask
{ mODTextTranslationClass(ArrayUdfValRestorer)
public:

		ArrayUdfValRestorer( const LargeValVec<od_int64>& undefidxs,
				      ArrayND<T>& arr )
		    : ParallelTask("Udf retriever")
		    , undefidxs_(undefidxs)
		    , arr_(arr)
		    , totalnr_(undefidxs.size())
		{}

    uiString	message() const { return tr("Replacing undefined values"); }

    uiString	nrDoneText() const	{ return sPositionsDone(); }

protected:

    od_int64	nrIterations() const	{ return totalnr_; }

private:

    bool	doWork( od_int64 start, od_int64 stop, int )
		{
		    T* outpptr = arr_.getData();
		    ValueSeries<T>* outpstor = arr_.getStorage();
		    mDefNDPosBuf( pos, arr_.nrDims() );

		    const T udfval = mUdf(T);
		    const ArrayNDInfo& info = arr_.info();
		    for ( od_int64 idx=start; idx<=stop; idx++,
							 quickAddToNrDone(idx) )
		    {
			const od_int64 sidx = undefidxs_[idx];
			if ( outpptr )
			    outpptr[sidx] = udfval;
			else if ( outpstor )
			    outpstor->setValue( sidx, udfval );
			else
			{
			    info.getArrayPos( sidx, pos );
			    arr_.setND( pos, udfval );
			}
		    }

		    return true;
		}

    const LargeValVec<od_int64>& undefidxs_;
    ArrayND<T>&			arr_;
    const od_int64		totalnr_;

};


/*!<Replaces undefined values back from missing traces to a 3D array */

template <class T>
mClass(Algo) Array3DUdfTrcRestorer : public ParallelTask
{ mODTextTranslationClass(Array3DUdfTrcRestorer)
public:

    mUseType( Survey, HorSubSel );

Array3DUdfTrcRestorer( const PosInfo::LineCollData& lcd, const HorSubSel& hss,
		       Array3D<T>& arr )
    : ParallelTask("Udf traces restorer")
    , lcd_(lcd)
    , hss_(hss)
    , arr_(arr)
    , totalnr_(lcd.totalSizeInside(hss) == mCast(int,hss.totalSize()) ? 0 :
	       arr.totalSize()/arr.getSize(2))
{
}


uiString message() const	{ return tr("Restoring undefined values"); }
uiString nrDoneText() const	{ return sTracesDone(); }

protected:

od_int64 nrIterations() const	{ return totalnr_; }

bool doWork( od_int64 start, od_int64 stop, int )
{
    const Array3DInfo& info = arr_.info();
    const int nrtrcsp = info.getSize( arr_.get1DDim() );
    T* dataptr = arr_.getData();
    ValueSeries<T>* datastor = arr_.getStorage();
    const bool hasarrayptr = dataptr;
    const bool hasstorage = datastor;
    const od_int64 offset = start * nrtrcsp;
    dataptr += offset;
    od_uint64 validx = offset;
    const Array2DInfoImpl hinfo( info.getSize(0),
				 info.getSize(1) );
    ArrayNDIter* hiter = !hasarrayptr && !hasstorage
		       ? new ArrayNDIter( hinfo ) : 0;
    if ( hiter )
	hiter->setGlobalPos( start );

    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	if ( lcd_.hasPosition(hss_,idx) )
	{
	    if ( hasarrayptr ) dataptr+=nrtrcsp;
	    else if ( hasstorage ) validx+=nrtrcsp;
	    else hiter->next();

	    continue;
	}

	if ( hasarrayptr )
	{
	    dataptr =
		OD::sysMemValueSet( dataptr, mUdf(T), nrtrcsp );
	}
	else if ( hasstorage )
	{
	    for ( int idz=0; idz<nrtrcsp; idz++ )
		datastor->setValue( validx++, mUdf(T) );
	}
	else
	{
	    const int inlidx = (*hiter)[0];
	    const int crlidx = (*hiter)[1];
	    for ( int idz=0; idz<nrtrcsp; idz++ )
		arr_.set( inlidx, crlidx, idz, mUdf(T) );
	}
    }

    delete hiter;

    return true;
}

    Array3D<T>&			arr_;
    const PosInfo::LineCollData&	lcd_;
    const HorSubSel&		hss_;
    const od_int64		totalnr_;

};


/*!\brief Transfers the common samples from one horizontal 2D array to another*/

template <class T>
mClass(Algo) CubeHorArrayCopier : public ParallelTask
{ mODTextTranslationClass(CubeHorCopier)
public:

CubeHorArrayCopier( const Array2D<T>& in, Array2D<T>& out,
		    const CubeHorSubSel& chssin, const CubeHorSubSel& chssout )
    : ParallelTask("CubeHor Array Copier")
    , chssin_(chssin)
    , chssout_(chssout)
    , in_(in)
    , out_(out)
{
    chssin.getIntersection( chssout, commonchss_ );
    totalnr_ = canCopyAll() ? 1 : commonchss_.nrInl();
}

uiString message() const override	{ return tr("Copying data"); }
uiString nrDoneText() const override
{
    return tr("%1 %2").arg( uiStrings::sInline(mPlural) )
		      .arg( uiStrings::sDone().toLower() );
}

void setReplacementValue( T val )	{ replval_ = val; }

protected:

od_int64 nrIterations() const		{ return totalnr_; }

private:

bool canCopyAll() const
{
    return chssout_ == chssin_ && in_.getData() &&
	   ( out_.getData() || out_.getStorage() );
}

bool doPrepare( int ) override
{
    if ( in_.info().getSize(0) != chssin_.nrInl() ||
	 in_.info().getSize(1) != chssin_.nrCrl() )
    {
	return false;
    }

    if ( out_.info().getSize(0) != chssout_.nrInl() ||
	 out_.info().getSize(1) != chssout_.nrCrl() )
    {
	mDynamicCastGet(Array2DImpl<T>*,outimpl,&out_)
	if ( !outimpl || !outimpl->setSize( chssout_.nrInl(),
					    chssout_.nrCrl() ) )
	{
	    return false;
	}

	out_.setAll( mUdf(T) );
    }

    alreadycopied_ = false;
    if ( canCopyAll() )
    {
	if ( out_.getData() )
	{
	    in_.getAll( out_.getData() );
	    alreadycopied_ = true;
	}
	else if ( out_.getStorage() )
	{
	    in_.getAll( *out_.getStorage() );
	    alreadycopied_ = true;
	}
    }

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int ) override
{
    if ( alreadycopied_ )
	return true;

    const CubeHorSubSel chssin( chssin_ );
    const CubeHorSubSel chssout( chssout_ );
    const CubeHorSubSel chss( commonchss_ );

    const bool usearrayptrs = in_.getData() && out_.getData() &&
			      in_.getStorage() &&
			      out_.getStorage();
    OffsetValueSeries<T>* invals = !usearrayptrs ? 0 :
	    new OffsetValueSeries<T>( *in_.getStorage(), 0, in_.getSize(2) );
    OffsetValueSeries<T>* outvals = !usearrayptrs ? 0 :
	    new OffsetValueSeries<T>( *out_.getStorage(), 0, out_.getSize(2) );
    const int nrcrl = chss.nrCrl();
    const od_int64 nrbytes = nrcrl * sizeof(T);

    const int startcrl = chss.crl4Idx( 0 );
    const int startcrlidyin = chssin.idx4Crl( startcrl );
    const int startcrlidyout = chssout.idx4Crl( startcrl );
    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++)
    {
	const int inl = chss.inl4Idx( idx );
	const int inlidxin = chssin.idx4Inl( inl );
	const int inlidxout = chssout.idx4Inl( inl );
	if ( usearrayptrs )
	{
	    invals->setOffset(
		    in_.info().getOffset(inlidxin,startcrlidyin) );
	    outvals->setOffset(
		    out_.info().getOffset(inlidxout,startcrlidyout) );
	    OD::sysMemCopy(outvals->arr(),invals->arr(), nrbytes);
	    continue;
	}
	else
	{
	    for ( int idy=0; idy<nrcrl; idy++ )
	    {
		const T val = in_.get( inlidxin, startcrlidyin+idy );
		out_.set( inlidxout, startcrlidyout+idy, val );
	    }
	}
    }

    delete invals;
    delete outvals;

    return true;
}

bool doFinish( bool success )
{
    if ( !success || mIsUdf(replval_) )
	return success;

    ArrayUdfValReplacer<T> replacer( out_, nullptr );
    replacer.setReplacementValue( replval_ );

    return replacer.execute();
}

    const CubeHorSubSel&	chssin_;
    const CubeHorSubSel&	chssout_;
    CubeHorSubSel		commonchss_;
    od_int64			totalnr_;
    bool			alreadycopied_ = false;

    const Array2D<T>&		in_;
    Array2D<T>&			out_;
    T				replval_ = mUdf(T);

};


/*!\brief Transfers the common samples from one Line 2D array to another */

template <class T>
mClass(Algo) Line2DArrayCopier : public ParallelTask
{ mODTextTranslationClass(LineArrayCopier)
public:

Line2DArrayCopier( const Array2D<T>& in, Array2D<T>& out,
		   const LineSubSel& lssin, const LineSubSel& lssout )
    : ParallelTask("Line2D Array Copier")
    , lssin_(lssin)
    , lssout_(lssout)
    , in_(in)
    , out_(out)
{
    totalnr_ = canCopyAll() ? 1 : lssout.lineHorSubSel().totalSize();
}

uiString message() const override	{ return tr("Copying line2d data"); }
uiString nrDoneText() const override	{ return uiStrings::sPositionsDone(); }

void setReplacementValue( T val )	{ replval_ = val; }

protected:

od_int64 nrIterations() const	{ return totalnr_; }

private:

bool canCopyAll() const
{
    return lssout_ == lssin_ && in_.getData() &&
	   ( out_.getData() || out_.getStorage() );
}

#undef mGetInfo
#define mGetInfo() \
    const Array2DInfoImpl infoin( lssin_.nrTrcs(), lssin_.nrZ() ); \
    const Array2DInfoImpl infoout( lssout_.nrTrcs(), lssout_.nrZ() );

bool doPrepare( int ) override
{
    mGetInfo()
    if ( in_.info() != infoin )
	return false;

    if ( out_.info() != infoout && !out_.setInfo(infoout) )
	return false;

    const auto zrgin( lssin_.zRange() );
    const auto zrgout( lssout_.zRange() );
    if ( !zrgin.isCompatible(zrgout) )
	return false; //Not supported

    out_.setAll( mUdf(T) );
    alreadycopied_ = false;
    if ( canCopyAll() )
    {
	if ( out_.getData() )
	{
	    in_.getAll( out_.getData() );
	    alreadycopied_ = true;
	}
	else if ( out_.getStorage() )
	{
	    in_.getAll( *out_.getStorage() );
	    alreadycopied_ = true;
	}
    }

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int ) override
{
    if ( alreadycopied_ )
	return true;

    mGetInfo()
    const LineHorSubSel lhssin( lssin_.lineHorSubSel() );
    const LineHorSubSel lhssout( lssout_.lineHorSubSel() );
    const int nrzout = infoout.getSize(1);
    const ZSampling zsampin( lssin_.zRange() );
    const ZSampling zsampout( lssout_.zRange() );
    ZSampling zrg( zsampout );
    zrg.limitTo( zsampin );
    const int nrztocopy = zrg.nrSteps() + 1;
    const int z0in = zsampin.nearestIndex( zrg.start );
    const int z0out = zsampout.nearestIndex( zrg.start );
    const od_int64 nrbytes = mCast(od_int64,nrztocopy) * sizeof(T);
    const T* inptr = in_.getData();
    T* outptr = out_.getData();
    const ValueSeries<T>* instor = in_.getStorage();
    ValueSeries<T>* outstor = out_.getStorage();
    const bool hasarrayptr = inptr && outptr;
    const bool hasstorage = instor && outstor;
    const bool needgetset = !hasarrayptr && !hasstorage;

    const Array1DInfoImpl info1d( infoout.getSize( 0 ) );
    ArrayNDIter iter( info1d );
    iter.setGlobalPos( start );

    const od_int64 offsetout = start * nrzout + z0out;
    outptr += offsetout;
    od_uint64 validxout = offsetout;

    for ( od_int64 idx=start; idx<=stop; idx++, iter.next(),
	  outptr+=nrzout, validxout+=nrzout,
	  quickAddToNrDone(idx) )
    {
	const int trcidx = iter[0];
	const int trcnr = lhssout.trcNr4Idx( trcidx );
	if ( !lhssin.includes(trcnr) )
	    continue;

	const int trcidxin = lhssin.idx4TrcNr( trcnr );
	const od_int64 offsetin = needgetset ? 0
				: infoin.getOffset( trcidxin, z0in );
	if ( hasarrayptr )
	{
	    OD::sysMemCopy( outptr, inptr+offsetin, nrbytes );
	}
	else if ( hasstorage )
	{
	    for ( int idz=0; idz<nrztocopy; idz++ )
	    {
		outstor->setValue( validxout+idz,
				   instor->value(offsetin+idz));
	    }
	}
	else
	{
	    for ( int idz=0, idzin=z0in; idz<nrztocopy; idz++,
							idzin++)
	    {
		const T val = in_.get( trcidxin, idzin );
		out_.set( trcidx, idz, val );
	    }
	}

    }

    return true;
}

bool doFinish( bool success )
{
    if ( !success || mIsUdf(replval_) )
	return success;

    ArrayUdfValReplacer<T> replacer( out_, nullptr );
    replacer.setReplacementValue( replval_ );

    return replacer.execute();
}

    const LineSubSel&		lssin_;
    const LineSubSel&		lssout_;
    od_int64			totalnr_;
    bool			alreadycopied_ = false;

    const Array2D<T>&		in_;
    Array2D<T>&			out_;
    T				replval_ = mUdf(T);

};


/*!\brief Transfers the common samples from one 3D array to another */

template <class T>
mClass(Algo) CubeArrayCopier : public ParallelTask
{ mODTextTranslationClass(CubeArrayCopier)
public:

CubeArrayCopier( const Array3D<T>& in, Array3D<T>& out,
		 const CubeSubSel& cssin, const CubeSubSel& cssout )
    : ParallelTask("Cube Array Copier")
    , cssin_(cssin)
    , cssout_(cssout)
    , in_(in)
    , out_(out)
{
    totalnr_ = canCopyAll() ? 1 : cssout.cubeHorSubSel().totalSize();
}

uiString message() const override	{ return tr("Copying cube data"); }
uiString nrDoneText() const override	{ return uiStrings::sPositionsDone(); }

void setReplacementValue( T val )	{ replval_ = val; }

protected:

od_int64 nrIterations() const		{ return totalnr_; }

private:

bool canCopyAll() const
{
    return cssout_ == cssin_ && in_.getData() &&
	   ( out_.getData() || out_.getStorage() );
}


#undef mGetInfo
#define mGetInfo() \
    const Array3DInfoImpl infoin( cssin_.nrInl(), \
				  cssin_.nrCrl(), cssin_.nrZ() ); \
    const Array3DInfoImpl infoout( cssout_.nrInl(), \
				   cssout_.nrCrl(), cssout_.nrZ() );

bool doPrepare( int ) override
{
    mGetInfo()
    if ( in_.info() != infoin )
	return false;

    if ( out_.info() != infoout && !out_.setInfo(infoout) )
	return false;

    const auto zrgin( cssin_.zRange() );
    const auto zrgout( cssout_.zRange() );
    if ( !zrgin.isCompatible(zrgout) )
	return false; //Not supported

    out_.setAll( mUdf(T) );
    alreadycopied_ = false;
    if ( canCopyAll() )
    {
	if ( out_.getData() )
	{
	    in_.getAll( out_.getData() );
	    alreadycopied_ = true;
	}
	else if ( out_.getStorage() )
	{
	    in_.getAll( *out_.getStorage() );
	    alreadycopied_ = true;
	}
    }

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int ) override
{
    if ( alreadycopied_ )
	return true;

    mGetInfo()
    const CubeHorSubSel chssin( cssin_.cubeHorSubSel() );
    const CubeHorSubSel chssout( cssout_.cubeHorSubSel() );
    const int nrzout = infoout.getSize(2);
    const ZSampling zsampin( cssin_.zRange() );
    const ZSampling zsampout( cssout_.zRange() );
    ZSampling zrg( zsampout );
    zrg.limitTo( zsampin );
    const int nrztocopy = zrg.nrSteps() + 1;
    const int z0in = zsampin.nearestIndex( zrg.start );
    const int z0out = zsampout.nearestIndex( zrg.start );
    const od_int64 nrbytes = mCast(od_int64,nrztocopy) * sizeof(T);
    const T* inptr = in_.getData();
    T* outptr = out_.getData();
    const ValueSeries<T>* instor = in_.getStorage();
    ValueSeries<T>* outstor = out_.getStorage();
    const bool hasarrayptr = inptr && outptr;
    const bool hasstorage = instor && outstor;
    const bool needgetset = !hasarrayptr && !hasstorage;

    const Array2DInfoImpl info2d( infoout.getSize( 0 ),
				  infoout.getSize( 1 ) );
    ArrayNDIter iter( info2d );
    iter.setGlobalPos( start );

    const od_int64 offsetout = start * nrzout + z0out;
    outptr += offsetout;
    od_uint64 validxout = offsetout;

    for ( od_int64 idx=start; idx<=stop; idx++, iter.next(),
	  outptr+=nrzout, validxout+=nrzout,
	  quickAddToNrDone(idx) )
    {
	const int inlidx = iter[0];
	const int crlidx = iter[1];
	const BinID bid( chssout.binID4RowCol(RowCol(inlidx,crlidx)) );
	if ( !chssin.includes(bid) )
	    continue;

	const int inlidxin = chssin.idx4Inl( bid.inl() );
	const int crlidxin = chssin.idx4Crl( bid.crl() );
	const od_int64 offsetin = needgetset ? 0
				: infoin.getOffset( inlidxin, crlidxin, z0in );
	if ( hasarrayptr )
	{
	    OD::sysMemCopy( outptr, inptr+offsetin, nrbytes );
	}
	else if ( hasstorage )
	{
	    for ( int idz=0; idz<nrztocopy; idz++ )
	    {
		outstor->setValue( validxout+idz,
				   instor->value(offsetin+idz));
	    }
	}
	else
	{
	    for ( int idz=0, idzin=z0in; idz<nrztocopy; idz++,
							idzin++)
	    {
		const T val = in_.get( inlidxin, crlidxin, idzin );
		out_.set( inlidx, crlidx, idz, val );
	    }
	}

    }

    return true;
}


bool doFinish( bool success )
{
    if ( !success || mIsUdf(replval_) )
	return success;

    ArrayUdfValReplacer<T> replacer( out_, nullptr );
    replacer.setReplacementValue( replval_ );

    return replacer.execute();
}

    const CubeSubSel&		cssin_;
    const CubeSubSel&		cssout_;
    od_int64			totalnr_;
    bool			alreadycopied_ = false;

    const Array3D<T>&		in_;
    Array3D<T>&			out_;
    T				replval_ = mUdf(T);

};


/*!
  \brief Polynomial trend with order 0 (mean), 1 (linear) or 2 (parabolic)
  The trend is derived from a set of values with positions
  and can be applied thereafter on any other position
*/

mExpClass(Algo) PolyTrend
{ mODTextTranslationClass(PolyTrend);
public:
				PolyTrend();

    bool			operator==(const PolyTrend&) const;

				enum Order	{ None, Order0, Order1, Order2};
				mDeclareEnumUtils(Order)

    static const char*		sKeyOrder()	{ return "Polynomial Order"; }
    static bool			getOrder(int nrpoints,Order&,uiString* =0);

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
    auto sz = poslist.size();
    if ( order_ == PolyTrend::None )
	return false;

    f0_ = f1_ = f2_ = f11_ = f12_ = f22_ = posc_.x_ = posc_.y_ = 0.;
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
    getOrder( sz, order_ );
    if ( order_ == Order0 )
	initOrder0( valsnoudf );
    else if ( order_ == Order1 )
	initOrder1( posnoudf, valsnoudf );
    else if ( order_ == Order2 )
	initOrder2( posnoudf, valsnoudf );
    else
	return false;

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

    const double dx = pos.x_ - posc_.x_;
    const double dy = pos.y_ - posc_.y_;
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


/*!<Determines the start/end of live data in a 2D/3D array. The returned index
    is the index of the first live sample
    The output arrays must have one dimension less than the data array
 */

template <class T>
mClass(Algo) MuteArrayExtracter : public ParallelTask
{ mODTextTranslationClass(MuteArrayExtracter)
public:

MuteArrayExtracter( const ArrayND<T>& data, ArrayND<int>& topmute,
		    ArrayND<int>& tailmute )
    : ParallelTask("Mute Array Extracter")
    , data_(data)
    , topmute_(topmute)
    , tailmute_(tailmute)
    , totalnr_(data.totalSize()/data.getSize(data.get1DDim()))
{
}

uiString message() const
{
    return tr("Extracting mute positions");
}

uiString nrDoneText() const	{ return sTracesDone(); }

void setPositions( const PosInfo::CubeData& cd, const CubeHorSubSel& hss )
{
    hss_ = hss; cubedata_ = cd;
    if ( !cubedata_.isAll(hss_) )
    {
	cubedata_.limitTo( hss_ );
	havesubsel_ = true;
    }
}

protected:

od_int64 nrIterations() const	{ return totalnr_; }

bool doPrepare( int )
{
    const int data1ddim = data_.get1DDim();
    if ( ( data1ddim != 1 && data1ddim != 2 ) ||
	 topmute_.get1DDim() != data1ddim-1 ||
	 tailmute_.get1DDim() != data1ddim-1 )
	return false;

    topmute_.setAll( 0 );
    const int nrz = mCast(int,data_.totalSize()/totalnr_);
    tailmute_.setAll( nrz-1 );

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int )
{
    const T* dataptr = data_.getData();
    int* topmuteptr = topmute_.getData();
    int* tailmuteptr = tailmute_.getData();
    const ValueSeries<T>* datastor = data_.getStorage();
    ValueSeries<int>* topmutestor = topmute_.getStorage();
    ValueSeries<int>* tailmutestor = tailmute_.getStorage();
    const bool hasarrayptr = dataptr && topmuteptr &&
			     tailmuteptr;
    const bool hasstorage = datastor && topmutestor &&
			    tailmutestor;
    const bool neediterator = !hasarrayptr && !hasstorage;
    const ArrayNDInfo& info = data_.info();
    const auto zidx = data_.get1DDim();
    const auto nrtrcsp = info.getSize( zidx );
    const od_int64 offset = start * nrtrcsp;
    if ( hasarrayptr )
    {
	dataptr += offset;
	topmuteptr += start;
	tailmuteptr += start;
    }

    auto validx = offset;
    const auto ndim = info.nrDims();
    const bool is2d = ndim == 2;
    const auto nrlines = is2d ? 1 : info.getSize(0);
    const auto nrtrcs = info.getSize( is2d ? 0 : 1 );
    const Array2DInfoImpl hinfo( nrlines, nrtrcs );
    ArrayNDIter* hiter = neediterator
		       ? new ArrayNDIter( hinfo ) : 0;
    if ( hiter )
	hiter->setGlobalPos( start );

    const T zeroval = mCast(T,0);
    mDefNDPosBuf( pos, ndim );
    typedef ArrayNDInfo::dim_idx_type dim_idx_type;

    for ( auto idx=start; idx<=stop; idx++, quickAddToNrDone(idx) )
    {
	if ( havesubsel_ && !cubedata_.includes(hss_.atGlobIdx(idx)) )
	{
	    // skip this position
	    if ( hasarrayptr )
	    {
		dataptr+=nrtrcsp;
		topmuteptr++;
		tailmuteptr++;
	    }
	    if ( hasstorage ) validx+=nrtrcsp;
	    else hiter->next();

	    continue;
	}

	ArrayNDInfo::NDPos hpos = hiter ? hiter->getPos() : 0;
	if ( hiter )
	{
	    for ( dim_idx_type ipos=0; ipos<ndim; ipos++ )
		pos[ipos] = hpos[ipos];
	    hiter->next();
	}

	bool allnull = true;
	for ( int idz=0; idz<nrtrcsp; idz++ )
	{
	    if ( hiter )
		pos[zidx] = idz;
	    const T val = hasarrayptr
			    ? *dataptr++
			    : hasstorage
				? datastor->value( validx++ )
				: data_.getND( pos );
	    if ( val == zeroval )
		continue;

	    if ( hasarrayptr )
	    {
		*topmuteptr++ = idz;
		dataptr += nrtrcsp-idz-2;
	    }
	    else if ( hasstorage )
	    {
		topmutestor->setValue( idx, idz );
		validx += nrtrcsp-idz-2;
	    }
	    else
		topmute_.setND( hpos, idz );
	    allnull = false;
	    break;
	}

	if ( allnull )
	{
	    if ( hasarrayptr )
	    {
		*topmuteptr++ = nrtrcsp;
		*tailmuteptr++ = -1;
	    }
	    else if ( hasstorage )
	    {
		topmutestor->setValue( idx, nrtrcsp );
		tailmutestor->setValue( idx, -1 );
	    }
	    else
	    {
		topmute_.setND( hpos, nrtrcsp );
		tailmute_.setND( hpos, -1 );
	    }

	    continue;
	}

	for ( int idz=nrtrcsp-1; idz>=0; idz-- )
	{
	    if ( hiter )
		pos[zidx] = idz;
	    const T val = hasarrayptr
			    ? *dataptr--
			    : hasstorage
				? datastor->value( validx-- )
				: data_.getND( pos );
	    if ( val == zeroval )
		continue;

	    if ( hasarrayptr )
	    {
		*tailmuteptr++ = idz;
		dataptr += nrtrcsp-idz+1;
	    }
	    else if ( hasstorage )
	    {
		tailmutestor->setValue( idx, idz );
		validx += nrtrcsp-idz+1;
	    }
	    else
		tailmute_.setND( hpos, idz );

	    break;
	}
    }

    delete hiter;
    return true;
}

    const ArrayND<T>&		data_;
    CubeHorSubSel		hss_;
    PosInfo::CubeData		cubedata_;
    ArrayND<int>&		topmute_;
    ArrayND<int>&		tailmute_;
    bool			havesubsel_	    = false;

    const od_int64		totalnr_;

};


/*!<Pads/extends data in a 3D array by copying the nearest trace */

template <class T>
mClass(Algo) Array3DNearPadder : public ParallelTask
{ mODTextTranslationClass(Array3DNearPadder)
public:

    mUseType( Survey, HorSubSel );

    Array3DNearPadder( BinID padsize, const PosInfo::LineCollData& lcd,
		       const CubeSubSel& css, Array3D<T>& arr )
    : ParallelTask("Padding by nearest trace")
    , lcd_(lcd)
    , css_(css)
    , arr_(arr)
    , padsize_(padsize)
    {
	const HorSubSel& hss = css.horSubSel();
	if ( lcd_.totalSizeInside(hss)==mCast(int,hss.totalSize()) )
	    totalnr_ = 0;
	else
	    totalnr_ = arr_.totalSize()/arr_.getSize(2);

    }


    uiString message() const	{ return tr("Padding by nearest trace"); }
    uiString nrDoneText() const { return sTracesDone(); }

protected:

    od_int64 nrIterations() const	{ return totalnr_; }

    bool doWork( od_int64 start, od_int64 stop, int )
    {
	const Array3DInfoImpl info( css_.nrInl(), css_.nrCrl(), css_.nrZ() );
	const CubeHorSubSel chss( css_.cubeHorSubSel() );
	const int nrtrcsp = info.getSize(2);

	T* arrptr = arr_.getData();
	ValueSeries<T>* datastor = arr_.getStorage();
	const bool hasarrayptr = arrptr;
	const bool hasstorage = datastor;

	const Array2DInfoImpl hinfo( info.getSize(0), info.getSize(1) );
	ArrayNDIter hiter( hinfo );
	hiter.setGlobalPos( start );

	const od_int64 nbytes = nrtrcsp * sizeof(T);
	const PosInfo::CubeData* cd = lcd_.asCubeData();
	const int pad = mMIN( padsize_.inl(), padsize_.crl() );

	for ( od_int64 idx=start; idx<=stop; idx++, hiter.next() )
	{
	    const int inlidx = hiter[0];
	    const int crlidx = hiter[1];
	    const BinID bid( chss.binID4RowCol(RowCol(inlidx,crlidx)) );

	    if ( cd->includes( bid ) )
		continue;

	    const BinID nrbid = cd->nearestBinID( bid, pad );
	    if ( abs((bid-nrbid).inl())>padsize_.inl() ||
		 abs((bid-nrbid).crl())>padsize_.crl() )
		continue;

	    const int nrinlidx = chss.idx4Inl( nrbid.inl() );
	    const int nrcrlidx = chss.idx4Crl( nrbid.crl() );

	    if ( hasarrayptr )
	    {
		const od_int64 off = info.getOffset( inlidx, crlidx, 0 );
		const od_int64 nroff = info.getOffset( nrinlidx, nrcrlidx, 0 );
		OD::sysMemCopy( arrptr+off, arrptr+nroff, nbytes );
	    }
	    else if ( hasstorage )
	    {
		const od_int64 off = info.getOffset( inlidx, crlidx, 0 );
		const od_int64 nroff = info.getOffset( nrinlidx, nrcrlidx, 0 );
		for ( int idz=0; idz<nrtrcsp; idz++ )
		{
		    const T val = datastor->value( nroff+idz );
		    datastor->setValue( off+idz, val );
		}
	    }
	    else
	    {
		for ( int idz=0; idz<nrtrcsp; idz++ )
		{
		    const T val = arr_.get( nrinlidx, nrcrlidx, idz );
		    arr_.set( inlidx, crlidx, idz, val );
		}
	    }
	}

	return true;
    }

    Array3D<T>&			arr_;
    const PosInfo::LineCollData&	lcd_;
    const CubeSubSel&		css_;
    od_int64			totalnr_;
    BinID			padsize_;

};
