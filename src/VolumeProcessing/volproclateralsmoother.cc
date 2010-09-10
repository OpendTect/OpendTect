/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	K. Tingdahl
 *Date:		Feb 2008
-*/

static const char* rcsID = "$Id: volproclateralsmoother.cc,v 1.11 2010-09-10 17:37:32 cvskris Exp $";

#include "volproclateralsmoother.h"

#include "arrayndslice.h"
#include "array2dinterpolimpl.h"
#include "arrayndimpl.h"
#include "fourier.h"
#include "keystrs.h"
#include "smoother2d.h"
#include "survinfo.h"


namespace VolProc
{


class LateralSmootherTask : public ParallelTask
{
public:
LateralSmootherTask(const Array3D<float>& input,
	int i0, int i1, int i2,
	Array3D<float>& output,
	int o0, int o1, int o2,
	const Interval<int>& i0samples,
	const Interval<int>& i1samples,
	const Interval<int>& i2samples,
	const Array2DFilterPars& pars )
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
{
}


~LateralSmootherTask()
{ delete kernel_; }


od_int64		nrIterations() const {return i2samples_.width()+1; }
od_int64		totalNr() const { return totalsz_; }
const char*		message() const { return "Smothing laterally"; }
const char*		nrDoneText() const { return "Samples processed"; }

private:

void reportRowDone(CallBacker*) 
{ addToNrDone(i1samples_.width()+1); }


bool doWork( od_int64 start, od_int64 stop, int thread )
{
    if ( pars_.type_==Stats::Average ) //&& !mIsUdf(pars_.rowdist_) )
	return processKernel( start, stop, thread );
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
	const int depthindex = i2samples_.start+idx;
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


bool processKernel( int start, int stop, int thread )
{
    const int ksz0 =
	Fourier::CC::nextFastSize( 2*pars_.stepout_.row+i0samples_.width()+1 );
    const int ksz1 =
	Fourier::CC::nextFastSize( 2*pars_.stepout_.col+i1samples_.width()+1 );

    Smoother2D<float> smoother;
    smoother.setWindow( mIsUdf(pars_.rowdist_)
		? BoxWindow::sName()
		: HanningWindow::sName(),
	    mUdf(float), pars_.stepout_.row*2+1, pars_.stepout_.col*2+1 );

    Array2DImpl<float> inputslice( ksz0, ksz1 );
    if ( !inputslice.isOK() )
	return false;

    Array2DImpl<float> outputslice( ksz0, ksz1 );
    if ( !outputslice.isOK() )
	return false;

    PtrMan<Array2DImpl<bool> > wasudf = 0;
    if ( !pars_.filludf_ )
    {
	wasudf = new Array2DImpl<bool>( ksz0, ksz1 );
	if ( !wasudf->isOK() )
	    return false;
    }

    smoother.setInput( inputslice, false );
    smoother.setOutput( outputslice );

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
    float* inputsliceptr = inputslice.getData();
    const float* outputsliceptr = outputslice.getData();

    for ( od_int64 depthidx=start; depthidx<=stop && shouldContinue();
	  depthidx++ )
    {
	const int depthindex = i2samples_.start+depthidx;
	const int inputdepth = depthindex-i2_;
	const int outputdepth = depthindex-o2_;

	inputslice.setAll( mUdf(float) );
	if ( wasudf ) wasudf->setAll( true );

	bool missingdata = false;
	for ( int idx0=0; idx0<ksz0; idx0++ )
	{
	    const int inputpos0 = kernelorigin0+idx0;
	    if ( inputpos0<0 )
	    {
		idx0 -= inputpos0+1;
		missingdata = true;
		continue; 
	    }

	    if ( inputpos0>lastinput0 )
	    {
		missingdata = true;
		break;
	    }

	    for ( int idx1=0; idx1<ksz1; idx1++ )
	    {
		const int inputpos1 = kernelorigin1+idx1;
		if ( inputpos1<0 )
		{
		    idx1 -= inputpos1+1;
		    missingdata = true;
		    continue; 
		}

		if ( inputpos1>lastinput1 )
		{
		    missingdata = true;
		    break;
		}

		const int offset = inputslice.info().getOffset( idx0, idx1 );
		const float val = input_.get(inputpos0,inputpos1,inputdepth);
		if ( mIsUdf(val) )
		    missingdata = true;
		else if ( wasudfptr )
		    wasudfptr[offset] = false;

		inputsliceptr[offset] = val;
	    }
	}

	if ( missingdata )
	{
	    interpol.setArray( inputslice, 0 );
	    interpol.execute();
	}

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
		    outputslice.info().getOffset( kernelpos0, kernelpos1 );

		if ( wasudfptr && wasudfptr[offset] )
		    continue;

		const float val = outputsliceptr[offset];
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
};


void LateralSmoother::initClass()
{
    VolProc::PS().addCreator( create, LateralSmoother::sKeyType(),
	    		      LateralSmoother::sUserName() );
}


const char* LateralSmoother::type() const
{ return sKeyType(); }


bool LateralSmoother::needsInput(const HorSampling&) const
{ return true; }
    
    
LateralSmoother::LateralSmoother(Chain& pc)
    : Step( pc )
{}


LateralSmoother::~LateralSmoother()
{}    


HorSampling LateralSmoother::getInputHRg( const HorSampling& hrg ) const
{
    HorSampling res = hrg;
    res.start.inl = hrg.start.inl - res.step.inl * pars_.stepout_.row;
    res.start.crl = hrg.start.crl - res.step.crl * pars_.stepout_.col;
    res.stop.inl = hrg.stop.inl + res.step.inl * pars_.stepout_.row;
    res.stop.crl = hrg.stop.crl + res.step.crl * pars_.stepout_.col;
    return res;
}


Step*  LateralSmoother::create( Chain& pc )
{ return new LateralSmoother( pc ); }


void LateralSmoother::setPars( const Array2DFilterPars& pars )
{
    pars_ = pars;
}


void LateralSmoother::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );

    pars.setYN( sKeyIsMedian(), pars_.type_==Stats::Median );
    pars.setYN( sKeyIsWeighted(),
	    pars_.type_!=Stats::Median && !mIsUdf(pars_.rowdist_) );
    pars.set( sKey::StepOutInl, pars_.stepout_.row );
    pars.set( sKey::StepOutCrl, pars_.stepout_.col );
}


bool LateralSmoother::usePar( const IOPar& pars )
{
    if ( !Step::usePar( pars ) )
	return false;

    bool ismedian, isweighted;
    if ( !pars.getYN( sKeyIsMedian(), ismedian ) || 
	 !pars.get( sKey::StepOutInl, pars_.stepout_.row ) ||
	 !pars.get( sKey::StepOutCrl, pars_.stepout_.col ) ||
	 !pars.getYN( sKeyIsWeighted(), isweighted ) )
    {
	return false;
    }

    pars_.type_ = ismedian ? Stats::Median : Stats::Average;
    pars_.rowdist_ = isweighted ? 1 : mUdf(float);

    return true;
}


Task* LateralSmoother::createTask()
{
    if ( !input_ || !output_ )
	return 0;

    if ( input_->inlsampling_.step!=output_->inlsampling_.step ||
	 input_->crlsampling_.step!=output_->crlsampling_.step || 
	 !mIsEqual(input_->zstep_,output_->zstep_,1e-3*SI().zRange(true).step)) 
    {
	return 0;
    }

    if ( pars_.type_!=Stats::Median )
    {
	if ( !mIsUdf(pars_.rowdist_) )
	{
	    pars_.rowdist_ = (SI().inlDistance()*input_->inlsampling_.step)/
			     (SI().crlDistance()*input_->crlsampling_.step);
	}
    }
    else
    {
	pars_.rowdist_ = mUdf(float);
    }

    pars_.filludf_ = true;

    Interval<int> inlsamples( input_->inlsampling_.nearestIndex(hrg_.start.inl),
	    		      input_->inlsampling_.nearestIndex(hrg_.stop.inl));

    Interval<int> crlsamples( input_->crlsampling_.nearestIndex(hrg_.start.crl),
	    		      input_->crlsampling_.nearestIndex(hrg_.stop.crl));


    return new LateralSmootherTask( input_->getCube( 0 ),
	    input_->inlsampling_.start,
	    input_->crlsampling_.start,
	    input_->z0_,
	    output_->getCube( 0 ),
	    output_->inlsampling_.start,
	    output_->crlsampling_.start,
	    output_->z0_,
	    inlsamples, crlsamples, zrg_,
	    pars_ );

    return 0;
}


}; //namespace
