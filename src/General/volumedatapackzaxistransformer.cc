/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		June 2014
________________________________________________________________________

-*/

#include "volumedatapackzaxistransformer.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "genericnumer.h"
#include "samplfunc.h"
#include "trckey.h"
#include "zaxistransform.h"

VolumeDataPackZAxisTransformer::VolumeDataPackZAxisTransformer(
						ZAxisTransform& zat,
						VolumeDataPack* out )
    : transform_(zat)
    , dpm_(DPM(DataPackMgr::SeisID()))
    , interpolate_(true)
    , inputdp_(0)
    , outputdp_(out)
{
    transform_.ref();
    zrange_.setFrom( transform_.getZInterval(false) );
    zrange_.step = transform_.getGoodZStep();
}


VolumeDataPackZAxisTransformer::~VolumeDataPackZAxisTransformer()
{
    transform_.unRef();
}


od_int64 VolumeDataPackZAxisTransformer::nrIterations() const
{
    if ( !inputdp_ ) return -1;
    auto voldp = dpm_.get<VolumeDataPack>( inputdp_->id() );
    return voldp ? voldp->nrPositions() : -1;
}


bool VolumeDataPackZAxisTransformer::doPrepare( int nrthreads )
{
    if ( !inputdp_ || !outputdp_ )
	return false;
    auto voldp = dpm_.get<VolumeDataPack>( inputdp_->id() );
    if ( voldp->isEmpty() )
	return false;

    if ( voldp->getScaler() )
	outputdp_->setScaler( *voldp->getScaler() );

    for ( int idx=0; idx<voldp->nrComponents(); idx++ )
	outputdp_->addComponent( voldp->getComponentName(idx), false );

    return true;
}


bool VolumeDataPackZAxisTransformer::doWork(
				od_int64 start, od_int64 stop, int threadid )
{
    auto voldp = dpm_.get<VolumeDataPack>( inputdp_->id() );
    if ( !voldp )
	return false;
    outputdp_ = voldp->getSimilar();
    if ( !outputdp_ )
	return false;

    const StepInterval<float>& inpzrg = voldp->zRange();
    const int nrinpsamp = inpzrg.nrSteps() + 1;
    const int nroutsamp = zrange_.nrSteps() + 1;
    const int nrtrcs = voldp->nrPositions() / voldp->data(0).getSize(0);

    ZAxisTransformSampler outputsampler( transform_, true,
	    SamplingData<double>(zrange_.start, zrange_.step), false );

    for ( int idx=0; idx<outputdp_->nrComponents(); idx++ )
    {
	for ( int posidx=mCast(int,start); posidx<=mCast(int,stop); posidx++ )
	{
	    const int lineidx = posidx / nrtrcs;
	    const int trcidx = posidx % nrtrcs;

	    TrcKey tk; voldp->getTrcKey(posidx,tk);
	    outputsampler.setTrcKey( tk );
	    outputsampler.computeCache( Interval<int>(0,nroutsamp-1) );

	    const Array3D<float>& inputdata = voldp->data( idx );
	    Array3D<float>& array = outputdp_->data( idx );
	    if ( inputdata.getData() && array.getData() )
	    {
		const float* trcptr = voldp->getTrcData( idx, posidx );
		SampledFunctionImpl<float,const float*> inputfunc(
				trcptr, nrinpsamp, inpzrg.start, inpzrg.step );
		inputfunc.setHasUdfs( true );
		inputfunc.setInterpolate( interpolate_ );

		float* arrptr = array.getData() + posidx*nroutsamp;
		reSample( inputfunc, outputsampler, arrptr, nroutsamp );
	    }
	    else if ( inputdata.getStorage() )
	    {
		const OffsetValueSeries<float> trcstor(
				voldp->getTrcStorage(idx,posidx) );
		SampledFunctionImpl<float,const ValueSeries<float> > inputfunc(
				trcstor, nrinpsamp, inpzrg.start, inpzrg.step );
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
		Array1DSlice<float> arr1dslice( voldp->data(idx) );
		arr1dslice.setPos( 0, lineidx );
		arr1dslice.setPos( 1, trcidx );
		arr1dslice.setDimMap( 0, 2 );
		if ( !arr1dslice.init() )
		    continue;

		SampledFunctionImpl<float,const ValueSeries<float> > inputfunc(
				arr1dslice, nrinpsamp, inpzrg.start,
				inpzrg.step );
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


bool VolumeDataPackZAxisTransformer::doFinish( bool success )
{
    outputdp_->setZDomain( transform_.toZDomainInfo() );
    outputdp_->setName( inputdp_->name() );
    dpm_.add( outputdp_ );
    return true;
}
