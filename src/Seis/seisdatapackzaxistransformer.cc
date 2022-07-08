/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		June 2014
________________________________________________________________________

-*/

#include "seisdatapackzaxistransformer.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "genericnumer.h"
#include "samplfunc.h"
#include "zaxistransform.h"

SeisDataPackZAxisTransformer::SeisDataPackZAxisTransformer( ZAxisTransform& zat,
							    SeisDataPack* out )
    : transform_(zat)
    , dpm_(DPM(DataPackMgr::SeisID()))
    , outputdp_(out)
{
    transform_.ref();
    zrange_.setFrom( transform_.getZInterval(false) );
    zrange_.step = transform_.getGoodZStep();
}


SeisDataPackZAxisTransformer::~SeisDataPackZAxisTransformer()
{
    transform_.unRef();
}


od_int64 SeisDataPackZAxisTransformer::nrIterations() const
{
    if ( !inputdp_ )
	return -1;

    ConstRefMan<SeisDataPack> seisdp = dpm_.get<SeisDataPack>( inputdp_->id() );
    return seisdp ? seisdp->nrTrcs() : -1;
}


bool SeisDataPackZAxisTransformer::doPrepare( int nrthreads )
{
    if ( !inputdp_ )
	return false;

    ConstRefMan<SeisDataPack> seisdp = dpm_.get<SeisDataPack>( inputdp_->id() );
    mDynamicCastGet(const RegularSeisDataPack*,regsdp,seisdp.ptr());
    mDynamicCastGet(const RandomSeisDataPack*,randsdp,seisdp.ptr());
    if ( !(regsdp || randsdp) || seisdp->isEmpty() )
	return false;

    if ( !outputdp_ )
    {
	const char* category = seisdp->category();
	const BinDataDesc* desc = &seisdp->getDataDesc();
	if ( regsdp )
	{
	    TrcKeyZSampling tkzs( regsdp->sampling() );
	    tkzs.zsamp_.setFrom( zrange_ );
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
    if ( seisdp->getScaler() )
	outputdp_->setScaler( *seisdp->getScaler() );

    for ( int idx=0; idx<seisdp->nrComponents(); idx++ )
	outputdp_->addComponent( seisdp->getComponentName(idx) );

    return true;
}


bool SeisDataPackZAxisTransformer::doWork(
				od_int64 start, od_int64 stop, int threadid )
{
    ConstRefMan<SeisDataPack> voldp = dpm_.get<SeisDataPack>( inputdp_->id() );
    if ( !inputdp_ || !outputdp_ )
	return false;

    const ZSampling inpzrg = voldp->zRange();
    const int nrinpsamp = inpzrg.nrSteps() + 1;
    const int nroutsamp = zrange_.nrSteps() + 1;
    const int nrtrcs = voldp->nrTrcs() / voldp->data(0).info().getSize(0);

    ZAxisTransformSampler outputsampler( transform_, true,
	    SamplingData<double>(zrange_.start, zrange_.step), false );

    for ( int idx=0; idx<outputdp_->nrComponents(); idx++ )
    {
	for ( int posidx=mCast(int,start); posidx<=mCast(int,stop); posidx++ )
	{
	    const int lineidx = posidx / nrtrcs;
	    const int trcidx = posidx % nrtrcs;

	    outputsampler.setTrcKey( voldp->getTrcKey(posidx) );
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


bool SeisDataPackZAxisTransformer::doFinish( bool success )
{
    outputdp_->setZDomain( transform_.toZDomainInfo() );
    outputdp_->setName( inputdp_->name() );
    dpm_.add( outputdp_ );
    return true;
}
