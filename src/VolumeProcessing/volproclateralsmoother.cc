/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volproclateralsmoother.h"

#include "arrayndslice.h"
#include "array2dinterpolimpl.h"
#include "arrayndimpl.h"
#include "fourier.h"
#include "keystrs.h"
#include "seisdatapack.h"
#include "smoother2d.h"
#include "survinfo.h"
#include "paralleltask.h"


namespace VolProc
{


class LateralSmootherTask : public ParallelTask
{ mODTextTranslationClass(LateralSmootherTask);
public:
LateralSmootherTask(const Array3D<float>& input,
	int i0, int i1, int i2,
	Array3D<float>& output,
	int o0, int o1, int o2,
	const Interval<int>& i0samples,
	const Interval<int>& i1samples,
	const Interval<int>& i2samples,
	const Array2DFilterPars& pars,
	bool mirroredges, bool interpolateudfs, float fixedval )
    : input_( input )
    , output_( output )
    , i0_( i0 ), i1_( i1 ), i2_( i2 )
    , o0_( o0 ), o1_( o1 ), o2_( o2 )
    , pars_( pars )
    , totalsz_( (i0samples.width()+1)*
		(i1samples.width()+1)*(i2samples.width()+1) )
    , i0samples_( i0samples )
    , i1samples_( i1samples )
    , i2samples_( i2samples )
    , kernel_( 0 )
    , domt_( false )
    , mirroredges_( mirroredges )
    , interpolateundefs_( interpolateudfs )
    , fixedval_( fixedval )
{
}


~LateralSmootherTask()
{ delete kernel_; }

od_int64	nrIterations() const override {return i2samples_.width()+1; }
od_int64	totalNr() const override { return totalsz_; }
uiString	uiMessage() const override { return tr("Smothing laterally"); }
uiString	uiNrDoneText() const override
		{ return tr("Samples processed"); }

bool		doPrepare(int nrthreads) override
		{ domt_ = nrthreads==1; return true; }

private:

void reportRowDone(CallBacker*)
{ addToNrDone(i1samples_.width()+1); }


bool doWork( od_int64 start, od_int64 stop, int thread ) override
{
    if ( pars_.type_==Stats::Average ) //&& !mIsUdf(pars_.rowdist_) )
	return processKernel( mCast(int,start), mCast(int,stop), thread );
    return processFilter( start, stop, thread );
}


bool processFilter( od_int64 start, od_int64 stop, int thread )
{
    Array2DSlice<float> inputslice( input_ );
    inputslice.setDimMap( 0, 0 );
    inputslice.setDimMap( 1, 1 );

    Array2DSlice<float> outputslice( output_ );
    outputslice.setDimMap( 0, 0 );
    outputslice.setDimMap( 1, 1 );

    const RowCol origin( o0_-i0_,  o1_-i1_ );

    for ( od_int64 idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int depthindex = mCast( int, i2samples_.start+idx );
	const int inputdepth = depthindex-i2_;
	const int outputdepth = depthindex-o2_;
	inputslice.setPos( 2, inputdepth );
	if ( !inputslice.init() )
	    return false;

	outputslice.setPos( 2, outputdepth );
	if ( !outputslice.init() )
	    return false;

	PtrMan<Array2DFilterer<float> > filter =
	    new Array2DFilterer<float>( inputslice, outputslice, origin,
					pars_ );

	filter->setScope( i0samples_, i1samples_ );
	filter->poststep.notify(
		mCB(this,LateralSmootherTask,reportRowDone) );

	if ( !filter->execute() )
	    return false;
    }

    return true;
}

#define mFixEdges( dim ) \
if ( inputpos##dim<0 ) \
{ \
    if ( mirroredges_ ) \
    { \
	inputpos##dim = -inputpos##dim; \
	if ( inputpos##dim>lastinput##dim ) \
	    inputpos##dim = lastinput##dim; \
    } \
    else \
    { \
	idx##dim -= inputpos##dim+1; \
	missingdata = true; \
	continue;  \
    } \
} \
else if ( inputpos##dim>lastinput##dim ) \
{ \
    if ( mirroredges_ ) \
    { \
	inputpos##dim = 2*lastinput##dim-inputpos##dim; \
	if ( inputpos##dim<0 ) \
	    inputpos##dim = 0; \
    } \
    else \
    { \
	missingdata = true; \
	break; \
    } \
}
bool processKernel( int start, int stop, int thread )
{
    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();
    const int ksz0 =
	fft->getFastSize( 2*pars_.stepout_.row()+i0samples_.width()+1 );
    const int ksz1 =
	fft->getFastSize( 2*pars_.stepout_.col()+i1samples_.width()+1 );

    Smoother2D<float> smoother;
    smoother.setWindow( mIsUdf(pars_.rowdist_)
		? BoxWindow::sName()
		: HanningWindow::sName(),
	    mUdf(float), pars_.stepout_.row()*2+1, pars_.stepout_.col()*2+1 );

    Array2DImpl<float> slice( ksz0, ksz1 );
    if ( !slice.isOK() )
	return false;

    const bool dontfilludf = !pars_.filludf_;

    PtrMan<Array2DImpl<bool> > wasudf = 0;
    if ( dontfilludf || !interpolateundefs_ )
    {
	wasudf = new Array2DImpl<bool>( ksz0, ksz1 );
	if ( !wasudf->isOK() )
	    return false;
    }

    smoother.setOutput( slice );

    const int kernelorigin0 = (i0samples_.stop+i0samples_.start-ksz0)/2;
    const int kernelorigin1 = (i1samples_.stop+i1samples_.start-ksz1)/2;
    const int lastinput0 = input_.info().getSize(0)-1;
    const int lastinput1 = input_.info().getSize(1)-1;
    const int outputsz0 = i0samples_.width()+1;
    const int outputsz1 = i1samples_.width()+1;

    InverseDistanceArray2DInterpol interpol;
    interpol.setFillType( Array2DInterpol::Full );
    interpol.setRowStep( 1 );
    interpol.setColStep( 1 );
    interpol.setClassification( false );
    interpol.setSearchRadius( 10 );

    bool* wasudfptr = wasudf ? wasudf->getData() : 0;
    float* sliceptr = slice.getData();

    for ( od_int64 depthidx=start; depthidx<=stop && shouldContinue();
	  depthidx++ )
    {
	double sum = 0;
	od_int64 nrvals = 0;
	const int depthindex = mCast( int, i2samples_.start+depthidx );
	const int inputdepth = depthindex-i2_;
	const int outputdepth = depthindex-o2_;

	slice.setAll( mUdf(float) );
	if ( wasudf ) wasudf->setAll( true );

	bool missingdata = false;
	for ( int idx0=0; idx0<ksz0; idx0++ )
	{
	    int inputpos0 = kernelorigin0+idx0;
	    mFixEdges( 0 );

	    for ( int idx1=0; idx1<ksz1; idx1++ )
	    {
		int inputpos1 = kernelorigin1+idx1;
		mFixEdges( 1 );

		const int offset = mCast(int,slice.info().getOffset(idx0,idx1));
		const float val = input_.get(inputpos0,inputpos1,inputdepth);
		if ( mIsUdf(val) )
		    missingdata = true;
		else
		{
		    if ( wasudfptr )
			wasudfptr[offset] = false;

		    sum += val;
		    nrvals++;
		}


		sliceptr[offset] = val;
	    }
	}

	if ( missingdata )
	{
	    if ( interpolateundefs_ || !wasudfptr )
	    {
		interpol.setArray( slice, 0 );
		interpol.executeParallel( domt_ );
	    }
	    else
	    {
		float val = 0;
		if ( mIsUdf(fixedval_) )
		{
		    if ( nrvals )
			val = (float) sum/nrvals;
		}
		else
		{
		    val = fixedval_;
		}

		for ( int idx=mCast(int,slice.info().getTotalSz()-1); idx>=0;
									idx-- )
		{
		    if ( wasudfptr[idx] )
			sliceptr[idx] = val;
		}
	    }
	}

	smoother.setInput( slice, false ); //trigger deletion of xf_
	smoother.execute();

	for ( int idx0=0; idx0<outputsz0; idx0++ )
	{
	    const int inputpos0 = idx0+i0samples_.start;
	    const int globalpos0 = inputpos0+i0_;
	    const int kernelpos0 = inputpos0-kernelorigin0;
	    const int outputpos0 = globalpos0-o0_;
	    for ( int idx1=0; idx1<outputsz1; idx1++ )
	    {
		const int inputpos1 = idx1+i1samples_.start;
		const int globalpos1 = inputpos1+i1_;
		const int kernelpos1 = inputpos1-kernelorigin1;
		const int outputpos1 = globalpos1-o1_;

		const int offset =
		   mCast( int, slice.info().getOffset(kernelpos0, kernelpos1) );

		if ( dontfilludf && wasudfptr[offset] )
		    continue;

		const float val = sliceptr[offset];
		output_.set( outputpos0, outputpos1, outputdepth, val );
	    }
	}

	addToNrDone( outputsz0 * outputsz1 );
    }

    return true;
}

    od_int64			totalsz_;

    const Array3D<float>&	input_;
    int				i0_, i1_, i2_;

    Array3D<float>&		output_;
    int				o0_, o1_, o2_;

    const Interval<int>		i0samples_;
    const Interval<int>		i1samples_;
    const Interval<int>		i2samples_;
    const Array2DFilterPars&	pars_;

    Array2DImpl<float_complex>*	kernel_;

    bool			domt_;

    bool			mirroredges_;
    bool			interpolateundefs_;
    float			fixedval_;
};


bool LateralSmoother::needsInput() const
{ return true; }


LateralSmoother::LateralSmoother()
    : mirroredges_( true )
    , interpolateundefs_( false )
    , fixedvalue_( mUdf(float) )
{
    setStepouts();
}


LateralSmoother::~LateralSmoother()
{}


TrcKeySampling LateralSmoother::getInputHRg( const TrcKeySampling& hrg ) const
{ return Step::getInputHRg( hrg ); }


void LateralSmoother::setStepouts()
{
    setHStep( pars_.stepout_ );
}


void LateralSmoother::setPars( const Array2DFilterPars& pars )
{
    pars_ = pars;
    setStepouts();
}


void LateralSmoother::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );

    pars.setYN( sKeyIsMedian(), pars_.type_==Stats::Median );
    pars.setYN( sKeyIsWeighted(),
	    pars_.type_!=Stats::Median && !mIsUdf(pars_.rowdist_) );
    pars.set( sKey::StepOutInl(), pars_.stepout_.row() );
    pars.set( sKey::StepOutCrl(), pars_.stepout_.col() );
}


bool LateralSmoother::usePar( const IOPar& pars )
{
    mirroredges_ = true;
    interpolateundefs_ = false;
    fixedvalue_ = mUdf(float);

    if ( !Step::usePar( pars ) )
	return false;

    bool ismedian, isweighted=true;
    if ( !pars.getYN( sKeyIsMedian(), ismedian ) ||
	 !pars.get( sKey::StepOutInl(), pars_.stepout_.row() ) ||
	 !pars.get( sKey::StepOutCrl(), pars_.stepout_.col() ) ||
	 !pars.getYN( sKeyIsWeighted(), isweighted ) )
    {
	return false;
    }

    pars_.type_ = ismedian ? Stats::Median : Stats::Average;
    pars_.rowdist_ = isweighted ? 1 : mUdf(float);

    pars.getYN( sKeyMirrorEdges(), mirroredges_ );
    pars.getYN( sKeyInterpolateUdf(), interpolateundefs_ );
    pars.get( sKeyFixedValue(), fixedvalue_ );

    setStepouts();

    return true;
}


Task* LateralSmoother::createTask()
{
    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !input || !output || input->isEmpty()  || output->isEmpty() )
	return 0;

    const TrcKeySampling& inphs = input->sampling().hsamp_;
    const TrcKeySampling& ouths = output->sampling().hsamp_;

    if ( inphs.step_ != ouths.step_ ||
	 !mIsEqual(input->sampling().zsamp_.step,
		   output->sampling().zsamp_.step,
		   1e-3*SI().zRange(true).step))
    {
	return 0;
    }

    if ( pars_.type_!=Stats::Median )
    {
	if ( !mIsUdf(pars_.rowdist_) )
	{
	    pars_.rowdist_ = (SI().inlDistance()*inphs.step_.inl())/
			     (SI().crlDistance()*inphs.step_.crl());
	}
    }
    else
    {
	pars_.rowdist_ = mUdf(float);
    }

    pars_.filludf_ = true;

    Interval<int> inlsamples(inphs.inlRange().nearestIndex(ouths.start_.inl()),
			     inphs.inlRange().nearestIndex(ouths.stop_.inl()));

    Interval<int> crlsamples(inphs.crlRange().nearestIndex(ouths.start_.crl()),
			     inphs.crlRange().nearestIndex(ouths.stop_.crl()));
    Interval<int> zsamples( input->sampling().zsamp_.nearestIndex(
					    output->sampling().zsamp_.start ),
			    input->sampling().zsamp_.nearestIndex(
					    output->sampling().zsamp_.stop ) );

    const int inpz0 =
	mNINT32(input->sampling().zsamp_.start/input->sampling().zsamp_.step);
    const int outpz0 =
	mNINT32(output->sampling().zsamp_.start/output->sampling().zsamp_.step);

    return new LateralSmootherTask( input->data( 0 ),
	    inphs.start_.inl(),
	    inphs.start_.crl(),
	    inpz0,
	    output->data( 0 ),
	    ouths.start_.inl(),
	    ouths.start_.crl(),
	    outpz0,
	    inlsamples, crlsamples, zsamples,
	    pars_, mirroredges_, interpolateundefs_, fixedvalue_ );
}


od_int64 LateralSmoother::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling& hsamp, const StepInterval<int>& ) const
{
    return getComponentMemory( hsamp, false );
}


} // namespace VolProc
