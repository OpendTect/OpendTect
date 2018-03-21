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
#include "enums.h"
#include "mathfunc.h"
#include "periodicvalue.h"
#include "posinfo.h"
#include "trckeyzsampling.h"
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

    uiString	nrDoneText() const	{ return ParallelTask::sPosFinished(); }
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
    ArrayNDInfo::TotalSzType	sz_;
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

    uiString	nrDoneText() const	{ return ParallelTask::sPosFinished(); }
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
    const auto sz = in.totalSize();
    if ( vals )
    {
	for ( auto idx=0; idx<sz; idx++ )
	{
	    if ( mIsUdf(vals[idx]) )
		return true;
	}

	return false;
    }

    const ValueSeries<fT>* stor = in.getStorage();
    if ( stor )
    {
	for ( auto idx=0; idx<sz; idx++ )
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
    const auto sz = in.getSize( 0 );
    for ( auto idx=0; idx<sz; idx++ )
    {
	const fT val = in.get( idx );
	if ( !mIsUdf(val) )
	    data.add( mCast(fT,idx), val );
    }

    for ( auto idx=0; idx<sz; idx++ )
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
			~ArrayNDWindow();

    bool		isOK() const		{ return window_; }

    float		getParamVal() const	{ return paramval_; }
    float*		getValues() const	{ return window_; }

    void		setValue(od_int64 idx,float val) { window_[idx]=val; }
    bool		setType(WindowType);
    bool		setType(const char*,float paramval=mUdf(float));

    bool		resize(const ArrayNDInfo&);

    template <class Type> bool	apply(  ArrayND<Type>* in,
					ArrayND<Type>* out_=0 ) const
    {
	ArrayND<Type>* out = out_ ? out_ : in;

	if ( out_ && in->info() != out_->info() )
	    return false;
	if ( in->info() != arrinfo_ )
	    return false;

	const auto totalsz = arrinfo_.totalSize();

	Type* indata = in->getData();
	Type* outdata = out->getData();
	if ( indata && outdata )
	{
	    for ( auto idx=0; idx<totalsz; idx++ )
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
		for ( auto idx=0; idx<totalsz; idx++ )
		{
		    Type inval = instorage->value(idx);
		    outstorage->setValue(idx,
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
    float			paramval_;

    bool			buildWindow(const char* winnm,float pval);
};


template<class T>
inline T Array3DInterpolate( const Array3D<T>& array,
			     float p0, float p1, float p2,
			     bool posperiodic = false )
{
    typedef ArrayNDInfo::IdxType IdxType;
    const Array3DInfo& size = array.info();
    IdxType intpos0 = roundOff<IdxType>( p0 );
    float dist0 = p0 - intpos0;
    IdxType prevpos0 = intpos0;
    if ( dist0 < 0 )
    {
	prevpos0--;
	dist0++;
    }
    if ( posperiodic ) prevpos0 = dePeriodize( prevpos0, size.getSize(0) );

    IdxType intpos1 = roundOff<IdxType>( p1 );
    float dist1 = p1 - intpos1;
    IdxType prevpos1 = intpos1;
    if ( dist1 < 0 )
	{ prevpos1--; dist1++; }
    if ( posperiodic )
	prevpos1 = dePeriodize( prevpos1, size.getSize(1) );

    IdxType intpos2 = roundOff<IdxType>( p2 );
    float dist2 = p2 - intpos2;
    IdxType prevpos2 = intpos2;
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

    IdxType firstpos0 = prevpos0 - 1;
    IdxType nextpos0 = prevpos0 + 1;
    IdxType lastpos0 = prevpos0 + 2;

    if ( posperiodic )
	firstpos0 = dePeriodize( firstpos0, size.getSize(0) );
    if ( posperiodic )
	nextpos0 = dePeriodize( nextpos0, size.getSize(0) );
    if ( posperiodic )
	lastpos0 = dePeriodize( lastpos0, size.getSize(0) );

    IdxType firstpos1 = prevpos1 - 1;
    IdxType nextpos1 = prevpos1 + 1;
    IdxType lastpos1 = prevpos1 + 2;

    if ( posperiodic )
	firstpos1 = dePeriodize( firstpos1, size.getSize(1) );
    if ( posperiodic )
	nextpos1 = dePeriodize( nextpos1, size.getSize(1) );
    if ( posperiodic )
	lastpos1 = dePeriodize( lastpos1, size.getSize(1) );

    IdxType firstpos2 = prevpos2 - 1;
    IdxType nextpos2 = prevpos2 + 1;
    IdxType lastpos2 = prevpos2 + 2;

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
	for ( auto idx=0; idx<ndim; idx++ )
	    srcposition[idx] = dePeriodize( copypos[idx] + destiter[idx],
					    srcsz.getSize(idx) );
	dest.setND( destiter.getPos(), src.get( srcposition ));

    } while ( destiter.next() );

    return true;
}


template <class T>
inline bool Array3DCopyPeriodic( Array3D<T>& dest, const Array3D<T>& src,
     ArrayNDInfo::IdxType p0, ArrayNDInfo::IdxType p1, ArrayNDInfo::IdxType p2 )
{
    if ( src.isEmpty() )
	return true;

    const auto destsz0 = dest.getSize( 0 );
    const auto destsz1 = dest.getSize( 1 );
    const auto destsz2 = dest.getSize( 2 );

    const auto srcsz0 = src.getSize( 0 );
    const auto srcsz1 = src.getSize( 1 );
    const auto srcsz2 = src.getSize( 2 );

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

    for ( auto id0=0; id0<destsz0; id0++ )
    {
	for ( auto id1=0; id1<destsz1; id1++ )
	{
	    for ( auto id2=0; id2<destsz2; id2++ )
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

    typedef ArrayNDInfo::DimIdxType DimIdxType;
    for ( DimIdxType idx=0; idx<ndim; idx++ )
    {
	if ( !destperiodic &&
	     pastepos[idx] + src.getSize(idx) > dest.getSize(idx) )
	    return false;
    }

    ArrayNDIter srciter( src );
    mDefNDPosBuf( destpos, ndim );
    do
    {
	for ( DimIdxType idx=0; idx<ndim; idx++ )
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
			  ArrayNDInfo::IdxType p0, ArrayNDInfo::IdxType p1,
			  bool destperiodic=false )
{
    const T* ptr = src.getData();
    if ( !ptr )
    {
	mDefNDPosBuf( posbuf, 2 );
	posbuf[0] = p0; posbuf[1] = p1;
	ArrayNDPaste( dest, src, mNDPosFromPosBuf(posbuf), destperiodic );
    }

    const auto srcsz0 = src.getSize( 0 );
    const auto srcsz1 = src.getSize( 1 );

    const auto destsz0 = dest.getSize( 0 );
    const auto destsz1 = dest.getSize( 1 );

    if ( !destperiodic
      && ( p0 + srcsz0 > destsz0
	|| p1 + srcsz1 > destsz1 ) )
	 return false;

    for ( auto id0=0; id0<srcsz0; id0++ )
    {
	for ( auto id1=0; id1<srcsz1; id1++ )
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
			  ArrayNDInfo::IdxType p0, ArrayNDInfo::IdxType p1,
			  ArrayNDInfo::IdxType p2, bool destperiodic=false )
{
    const T* ptr = src.getData();
    if ( !ptr )
    {
	mDefNDPosBuf( posbuf, 3 );
	posbuf[0] = p0; posbuf[1] = p1; posbuf[2] = p2;
	ArrayNDPaste( dest, src, mNDPosFromPosBuf(posbuf), destperiodic );
    }

    const auto srcsz0 = src.getSize( 0 );
    const auto srcsz1 = src.getSize( 1 );
    const auto srcsz2 = src.getSize( 2 );

    const auto destsz0 = dest.getSize( 0 );
    const auto destsz1 = dest.getSize( 1 );
    const auto destsz2 = dest.getSize( 2 );

    if ( !destperiodic
      && ( p0 + srcsz0 > destsz0
	|| p1 + srcsz1 > destsz1
	|| p2 + srcsz2 > destsz2 ) )
	 return false;

    for ( auto id0=0; id0<srcsz0; id0++ )
    {
	for ( auto id1=0; id1<srcsz1; id1++ )
	{
	    for ( auto id2=0; id2<srcsz2; id2++ )
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

    uiString	nrDoneText() const
		    { return tr("%1 done").arg(uiStrings::sInline(mPlural)); }
    uiString	message() const	{ return tr("Transferring grid data");}

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
		    if ( in_.getSize(0) != tksin_.nrLines() ||
			 in_.getSize(1) != tksin_.nrTrcs() )
		    {
			return false;
		    }

		    if ( out_.getSize(0) != tksout_.nrLines() ||
			 out_.getSize(1) != tksout_.nrTrcs() )
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
		    for ( int idx=(int)start; idx<=stop; idx++ )
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
			    OD::sysMemCopy( outvals->arr(),invals->arr(),
					    nrbytes );
			    continue;
			}
			else
			{
			    for ( int idy=0; idy<nrcrl; idy++ )
			    {
				const T val =
					in_.get( inlidxin, startcrlidyin+idy );
				out_.set( inlidxout, startcrlidyout+idy, val );
			    }
			}
		    }

		    delete invals;
		    delete outvals;

		    return true;
		}

    const Array2D<T>&		in_;
    const TrcKeySampling&	tksin_;
    const TrcKeySampling&	tksout_;
    TrcKeySampling		commontks_;
    Array2D<T>&			out_;
};


/*!\brief Transfers the common samples from one 3D array to another */

template <class T>
mClass(Algo) Array3DCopier : public ParallelTask
{ mODTextTranslationClass(Array3DCopier)
public:
		Array3DCopier( const Array3D<T>& in, Array3D<T>& out,
			       const TrcKeyZSampling& tkzsin,
			       const TrcKeyZSampling& tkzsout )
		    : ParallelTask("Array 3D Resizer")
		    , tkzsin_(tkzsin)
		    , tkzsout_(tkzsout)
		    , totalnr_(tkzsout.hsamp_.totalNr())
		    , in_(in)
		    , out_(out)
		{}

    uiString	message() const	{ return tr("Transferring volume data"); }

    uiString	nrDoneText() const	{ return ParallelTask::sTrcFinished(); }

protected:

    od_int64	nrIterations() const	{ return totalnr_; }

private:

#define mGetInfo() \
    const Array3DInfoImpl infoin( tkzsin_.hsamp_.nrLines(), \
				  tkzsin_.hsamp_.nrTrcs(), tkzsin_.nrZ() ); \
    const Array3DInfoImpl infoout( tkzsout_.hsamp_.nrLines(), \
				   tkzsout_.hsamp_.nrTrcs(), tkzsout_.nrZ() );

    bool	doPrepare( int )
		{
		    mGetInfo()
		    if ( in_.info() != infoin )
			return false;

		    if ( out_.info() != infoout && !out_.setInfo(infoout) )
			return false;

		    if ( tkzsin_.hsamp_.survid_ != tkzsout_.hsamp_.survid_ )
			return false;

		    if ( !tkzsin_.zsamp_.isCompatible(tkzsout_.zsamp_) )
			return false; //Not supported

		    out_.setAll( mUdf(T) );

		    return true;
		}


    bool	doWork( od_int64 start, od_int64 stop, int )
		{
		    mGetInfo()
		    const TrcKeySampling tksin( tkzsin_.hsamp_ );
		    const TrcKeySampling tksout( tkzsout_.hsamp_ );
		    const int nrzout = infoout.getSize(2);
		    StepInterval<float> zrg( tkzsout_.zsamp_ );
		    zrg.limitTo( tkzsin_.zsamp_ );
		    const int nrztocopy = zrg.nrSteps() + 1;
		    const int z0in = tkzsin_.zsamp_.nearestIndex( zrg.start );
		    const int z0out = tkzsout_.zsamp_.nearestIndex( zrg.start );
		    const od_int64 nrbytes =
				   mCast(od_int64,nrztocopy) * sizeof(T);
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
		    od_int64 validxout = offsetout;

		    for ( od_int64 idx=start; idx<=stop; idx++, iter.next(),
			  outptr+=nrzout, validxout+=nrzout,
			  quickAddToNrDone(idx) )
		    {
			const int inlidx = iter[0];
			const int crlidx = iter[1];
			const BinID bid( tksout.atIndex(inlidx,crlidx) );
			if ( !tksin.includes(bid) )
			    continue;

			const int inlidxin = tksin.lineIdx( bid.lineNr() );
			const int crlidxin = tksin.trcIdx( bid.trcNr() );
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
				const T val =
					in_.get( inlidxin, crlidxin, idzin );
				out_.set( inlidx, crlidx, idz, val );
			    }
			}
		    }

		    return true;
		}

    const TrcKeyZSampling&	tkzsin_;
    const TrcKeyZSampling&	tkzsout_;
    const od_int64		totalnr_;

    const Array3D<T>&		in_;
    Array3D<T>&			out_;
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
    for ( auto idx=0; idx<sz; idx++ )
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


/*!<Replaces the undefined samples in a 2D/3D array. Optionally provides
    the list of replaced samples.
    If a PosInfo::CubeData is provided the samples where traces are not present
    will not be substituted
 */

template <class T>
mClass(Algo) ArrayUdfValReplacer : public ParallelTask
{ mODTextTranslationClass(ArrayUdfValReplacer)
public:
		ArrayUdfValReplacer( Array2D<T>& inp,
				     LargeValVec<od_int64>* undefidxs )
		    : ParallelTask("Array Udf Replacer")
		    , inp_(inp)
		    , replval_(0.f)
		    , undefidxs_(undefidxs)
		    , tks_(0)
		    , trcssampling_(0)
		    , totalnr_(inp.totalSize()/inp.getSize(1))
		{}

		ArrayUdfValReplacer( Array3D<T>& inp,
				     LargeValVec<od_int64>* undefidxs )
		    : ParallelTask("Array Udf Replacer")
		    , inp_(inp)
		    , replval_(0.f)
		    , undefidxs_(undefidxs)
		    , tks_(0)
		    , trcssampling_(0)
		    , totalnr_(inp.totalSize()/inp.getSize(2))
		{}

    uiString	message() const
		{
		    return tr("Replacing undefined values");
		}

    uiString	nrDoneText() const	{ return ParallelTask::sTrcFinished(); }

    void	setReplacementValue( T val )	{ replval_ = val; }

    void	setSampling( const TrcKeySampling& tks,
			     const PosInfo::CubeData* trcssampling )
		{
		    tks_ = &tks;
		    trcssampling_ = trcssampling;
		}

protected:

    od_int64	nrIterations() const	{ return totalnr_; }

private:

    bool	doPrepare( int )
		{
		    if ( undefidxs_ )
			undefidxs_->setEmpty();

		    return true;
		}

    bool	doWork( od_int64 start, od_int64 stop, int )
		{
		    const bool isrect = tks_ && trcssampling_
				       ? trcssampling_->isFullyRectAndReg()
				       : true;
		    const int nrtrcsp = inp_.getSize( inp_.get1DDim() );
		    T* dataptr = inp_.getData();
		    ValueSeries<T>* datastor = inp_.getStorage();
		    const bool hasarrayptr = dataptr;
		    const bool hasstorage = datastor;
		    const bool neediterator = !hasarrayptr && !hasstorage;
		    const od_int64 offset = start * nrtrcsp;
		    dataptr += offset;
		    od_int64 validx = offset;
		    ArrayNDIter* iter = neediterator ? new ArrayNDIter( inp_ )
						     : 0;
		    if ( iter )
			iter->setGlobalPos( offset );

		    const T replval = replval_;
		    for ( od_int64 idx=start; idx<=stop; idx++,
							 quickAddToNrDone(idx))
		    {
			const bool hastrcdata = isrect ? true
					: trcssampling_->isValid(idx,*tks_);
			if ( hastrcdata )
			{
			    for ( int idz=0; idz<nrtrcsp; idz++ )
			    {
				ArrayNDInfo::NDPos pos = iter ? iter->getPos()
							      : 0;
				const T val = hasarrayptr ? *dataptr
					    : hasstorage
						? datastor->value( validx )
						: inp_.getND( pos );
				if ( !mIsUdf(val) )
				{
				    if ( hasarrayptr )
					dataptr++;
				    else if ( hasstorage )
					validx++;
				    else
					iter->next();
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
				OD::sysMemValueSet(dataptr, replval, nrtrcsp);
			    }
			    else if ( hasstorage )
			    {
				for ( auto idz=0; idz<nrtrcsp; idz++ )
				    datastor->setValue( validx++, replval );
			    }
			    else
			    {
				for ( auto idz=0; idz<nrtrcsp; idz++ )
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
    T				replval_;
    LargeValVec<od_int64>*	undefidxs_;
    const TrcKeySampling*	tks_;
    const PosInfo::CubeData*	trcssampling_;
    const od_int64		totalnr_;
    Threads::Mutex		lck_;
};


/*!<Replaces undefined values back to an ND array */

template <class T>
mClass(Algo) ArrayUdfValRestorer : public ParallelTask
{ mODTextTranslationClass(ArrayUdfValRestorer)
public:
		ArrayUdfValRestorer( const LargeValVec<od_int64>& undefidxs,
				      ArrayND<T>& outp )
		    : ParallelTask("Udf retriever")
		    , undefidxs_(undefidxs)
		    , outp_(outp)
		    , totalnr_(undefidxs.size())
		{}

    uiString	message() const { return tr("Replacing undefined values"); }

    uiString	nrDoneText() const	{ return ParallelTask::sPosFinished(); }

protected:

    od_int64	nrIterations() const	{ return totalnr_; }

private:

    bool	doWork( od_int64 start, od_int64 stop, int )
		{
		    T* outpptr = outp_.getData();
		    ValueSeries<T>* outpstor = outp_.getStorage();
		    mDefNDPosBuf( pos, outp_.nrDims() );

		    const T udfval = mUdf(T);
		    const ArrayNDInfo& info = outp_.info();
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
			    outp_.setND( pos, udfval );
			}
		    }

		    return true;
		}

    const LargeValVec<od_int64>&	undefidxs_;
    ArrayND<T>&			outp_;
    const od_int64		totalnr_;
};


/*!<Replaces undefined values back from missing traces to a 3D array */

template <class T>
mClass(Algo) Array3DUdfTrcRestorer : public ParallelTask
{ mODTextTranslationClass(Array3DUdfTrcRestorer)
public:
		Array3DUdfTrcRestorer( const PosInfo::CubeData& trcssampling,
				       const TrcKeySampling& tks,
				       Array3D<T>& outp )
		    : ParallelTask("Udf traces retriever")
		    , trcssampling_(trcssampling)
		    , tks_(tks)
		    , outp_(outp)
		    , totalnr_(trcssampling.totalSizeInside(tks) ==
			       mCast(int,tks.totalNr()) ? 0 :
			       outp.totalSize()/outp.getSize(2))
		{}

    uiString	message() const { return tr("Restoring undefined values"); }

    uiString	nrDoneText() const	{ return ParallelTask::sTrcFinished(); }

protected:

    od_int64	nrIterations() const	{ return totalnr_; }

private:

    bool	doWork( od_int64 start, od_int64 stop, int )
		{
		    const Array3DInfo& info = outp_.info();
		    const int nrtrcsp = info.getSize( outp_.get1DDim() );
		    T* outpptr = outp_.getData();
		    ValueSeries<T>* outstor = outp_.getStorage();
		    const bool hasarrayptr = outpptr;
		    const bool hasstorage = outstor;
		    const od_int64 offset = start * nrtrcsp;
		    outpptr += offset;
		    od_int64 validx = offset;
		    const Array2DInfoImpl hinfo( info.getSize(0),
						 info.getSize(1) );
		    ArrayNDIter* hiter = !hasarrayptr && !hasstorage
				       ? new ArrayNDIter( hinfo ) : 0;
		    if ( hiter )
			hiter->setGlobalPos( start );

		    for ( od_int64 idx=start; idx<=stop; idx++ )
		    {
			if ( trcssampling_.isValid(idx,tks_) )
			{
			    if ( hasarrayptr ) outpptr+=nrtrcsp;
			    else if ( hasstorage ) validx+=nrtrcsp;
			    else hiter->next();

			    continue;
			}

			if ( hasarrayptr )
			{
			    outpptr =
				OD::sysMemValueSet( outpptr, mUdf(T), nrtrcsp);
			}
			else if ( hasstorage )
			{
			    for ( auto idz=0; idz<nrtrcsp; idz++ )
				outstor->setValue( validx++, mUdf(T) );
			}
			else
			{
			    const int inlidx = (*hiter)[0];
			    const int crlidx = (*hiter)[1];
			    for ( auto idz=0; idz<nrtrcsp; idz++ )
				outp_.set( inlidx, crlidx, idz, mUdf(T) );
			}
		    }

		    delete hiter;

		    return true;
		}

    const PosInfo::CubeData&	trcssampling_;
    const TrcKeySampling&	tks_;
    Array3D<T>&			outp_;

    const od_int64		totalnr_;
};


/*!<Determines the start/end of live data in a 2D/3D array. The returned index
    is the index of the first live sample
    The output arrays must have one dimension less than the data array
 */

template <class T>
mClass(Algo) MuteArrayExtracter : public ParallelTask
{ mODTextTranslationClass(MuteArrayExtracter)
public:
		MuteArrayExtracter( const ArrayND<T>& data,
				    ArrayND<int>& topmute,
				    ArrayND<int>& tailmute )
		    : ParallelTask("Mute Array Extracter")
		    , data_(data)
		    , topmute_(topmute)
		    , tailmute_(tailmute)
		    , tks_(0)
		    , trcssampling_(0)
		    , totalnr_(data.totalSize()/
			       data.getSize(data.get1DDim()))
		{}

    uiString	message() const
		{
		    return tr("Extracting mute positions");
		}

    uiString	nrDoneText() const	{ return ParallelTask::sTrcFinished(); }

    void	setSampling( const TrcKeySampling& tks,
			     const PosInfo::CubeData* trcssampling )
		{
		    tks_ = &tks;
		    trcssampling_ = trcssampling;
		}

protected:

    od_int64	nrIterations() const	{ return totalnr_; }

private:

    bool	doPrepare( int )
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

    bool	doWork( od_int64 start, od_int64 stop, int )
		{
		    const bool isrect = tks_ && trcssampling_
				       ? trcssampling_->isFullyRectAndReg()
				       : true;
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

		    for ( auto idx=start; idx<=stop; idx++,
						     quickAddToNrDone(idx) )
		    {
			const bool hastrcdata = isrect ? true
					: trcssampling_->isValid(idx,*tks_);
			if ( !hastrcdata )
			{
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
			    for ( auto ipos=0; ipos<ndim; ipos++ )
				pos[ipos] = hpos[ipos];
			    hiter->next();
			}

			bool allnull = true;
			for ( auto idz=0; idz<nrtrcsp; idz++ )
			{
			    if ( hiter )
				pos[zidx] = idz;
			    const float val = hasarrayptr
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

			for ( auto idz=nrtrcsp-1; idz>=0; idz-- )
			{
			    if ( hiter )
				pos[zidx] = idz;
			    const float val = hasarrayptr
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
    const TrcKeySampling*	tks_;
    const PosInfo::CubeData*	trcssampling_;
    ArrayND<int>&		topmute_;
    ArrayND<int>&		tailmute_;

    const od_int64		totalnr_;

};
