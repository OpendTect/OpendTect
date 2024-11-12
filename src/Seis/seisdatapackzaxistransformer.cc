/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisdatapackzaxistransformer.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "genericnumer.h"
#include "samplfunc.h"
#include "zdomain.h"


SeisDataPackZAxisTransformer::SeisDataPackZAxisTransformer( ZAxisTransform& zat,
						    SeisVolumeDataPack* out )
    : transform_(&zat)
    , outputdp_(out)
{
    zrange_ = transform_->getZInterval( false );
}


SeisDataPackZAxisTransformer::~SeisDataPackZAxisTransformer()
{
}


od_int64 SeisDataPackZAxisTransformer::nrIterations() const
{
    return inputdp_ ? inputdp_->nrTrcs() : -1;
}


bool SeisDataPackZAxisTransformer::doPrepare( int nrthreads )
{
    if ( !inputdp_ || (!inputdp_->isRegular() && !inputdp_->isRandom()) )
	return false;

    mDynamicCastGet(const RegularSeisDataPack*,regsdp,inputdp_.ptr());
    mDynamicCastGet(const RandomSeisDataPack*,randsdp,inputdp_.ptr());
    if ( !(regsdp || randsdp) || inputdp_->isEmpty() )
	return false;

    if ( !outputdp_ )
    {
	const char* category = inputdp_->category();
	const BinDataDesc* desc = &inputdp_->getDataDesc();
	if ( regsdp )
	{
	    TrcKeyZSampling tkzs( regsdp->sampling() );
	    tkzs.zsamp_ = zrange_;
	    auto* output = new RegularSeisDataPack( category, desc );
	    output->setSampling( tkzs );
	    outputdp_ = output;
	}
	else if ( randsdp )
	{
	    auto* output = new RandomSeisDataPack( category, desc );
	    output->setRandomLineID( randsdp->getRandomLineID() );
	    output->setPath( randsdp->getPath() );
	    output->setZRange( zrange_ );
	    outputdp_ = output;
	}
    }

    if ( !outputdp_ )
	return false;

    outputdp_->setName( inputdp_->name() );
    if ( inputdp_->getScaler() )
	outputdp_->setScaler( *inputdp_->getScaler() );

    for ( int idx=0; idx<inputdp_->nrComponents(); idx++ )
	outputdp_->addComponent( inputdp_->getComponentName(idx) );

    return true;
}


bool SeisDataPackZAxisTransformer::doWork(
				od_int64 start, od_int64 stop, int threadid )
{
    if ( !inputdp_ || !outputdp_ )
	return false;

    const SeisVolumeDataPack& voldp = *inputdp_.ptr();
    SeisVolumeDataPack& outdp = *outputdp_.ptr();
    const ZSampling inpzrg = voldp.zRange();
    const int nrinpsamp = inpzrg.nrSteps() + 1;
    const int nroutsamp = zrange_.nrSteps() + 1;
    const int nrtrcs = voldp.nrTrcs() / voldp.data(0).info().getSize(0);

    ZAxisTransformSampler outputsampler( *transform_.ptr(), true,
		 SamplingData<double>(zrange_.start_, zrange_.step_), false );

    for ( int idx=0; idx<outdp.nrComponents(); idx++ )
    {
	for ( int posidx=mCast(int,start); posidx<=mCast(int,stop); posidx++ )
	{
	    const int lineidx = posidx / nrtrcs;
	    const int trcidx = posidx % nrtrcs;

	    outputsampler.setTrcKey( voldp.getTrcKey(posidx) );
	    outputsampler.computeCache( Interval<int>(0,nroutsamp-1) );

	    const Array3D<float>& inputdata = voldp.data( idx );
	    Array3D<float>& array = outdp.data( idx );
	    if ( inputdata.getData() && array.getData() )
	    {
		const float* trcptr = voldp.getTrcData( idx, posidx );
		SampledFunctionImpl<float,const float*> inputfunc(
                            trcptr, nrinpsamp, inpzrg.start_, inpzrg.step_ );
		inputfunc.setHasUdfs( true );
		inputfunc.setInterpolate( interpolate_ );

		float* arrptr = array.getData() + posidx*nroutsamp;
		reSample( inputfunc, outputsampler, arrptr, nroutsamp );
	    }
	    else if ( inputdata.getStorage() )
	    {
		const OffsetValueSeries<float> trcstor(
				voldp.getTrcStorage(idx,posidx) );
		SampledFunctionImpl<float,const ValueSeries<float> > inputfunc(
                            trcstor, nrinpsamp, inpzrg.start_, inpzrg.step_ );
		inputfunc.setHasUdfs( true );
		inputfunc.setInterpolate( interpolate_ );

		TypeSet<float> trcvals( nroutsamp, mUdf(float) );
		float* arrptr = trcvals.arr();
		reSample( inputfunc, outputsampler, arrptr, nroutsamp );

		for ( int zidx=0; zidx<nroutsamp; zidx++ )
		    array.set( lineidx, trcidx, zidx, trcvals[zidx] );
	    }
	    else
	    {
		Array1DSlice<float> arr1dslice( voldp.data(idx) );
		arr1dslice.setPos( 0, lineidx );
		arr1dslice.setPos( 1, trcidx );
		arr1dslice.setDimMap( 0, 2 );
		if ( !arr1dslice.init() )
		    continue;

		SampledFunctionImpl<float,const ValueSeries<float> > inputfunc(
                            arr1dslice, nrinpsamp, inpzrg.start_,
                            inpzrg.step_ );
		inputfunc.setHasUdfs( true );
		inputfunc.setInterpolate( interpolate_ );

		for ( int zidx=0; zidx<nroutsamp; zidx++ )
		{
		    const float sampleval = outputsampler[zidx];
		    const float outputval = mIsUdf(sampleval) ? mUdf(float) :
					    inputfunc.getValue(sampleval);
		    array.set( lineidx, trcidx, zidx, outputval );
		}
	    }
	}
    }

    return true;
}


bool SeisDataPackZAxisTransformer::doFinish( bool success )
{
    outputdp_->setZDomain( transform_->toZDomainInfo() );
    outputdp_->setName( inputdp_->name() );
    return true;
}
