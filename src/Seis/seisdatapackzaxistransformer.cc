/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		June 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "seisdatapackzaxistransformer.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "genericnumer.h"
#include "samplfunc.h"
#include "zaxistransform.h"

SeisDataPackZAxisTransformer::SeisDataPackZAxisTransformer(
						ZAxisTransform& zat )
    : transform_(zat)
    , dpm_(DPM(DataPackMgr::SeisID()))
    , interpolate_(true)
    , inputdp_(0)
    , outputdp_(0)
    , outputid_(0)
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
    if ( !inputdp_ ) return -1;
    ConstDataPackRef<SeisDataPack> seisdp = dpm_.obtain( inputdp_->id() );
    return seisdp ? seisdp->nrTrcs() : -1;
}


bool SeisDataPackZAxisTransformer::doPrepare( int nrthreads )
{
    if ( !inputdp_ ) return false;

    ConstDataPackRef<SeisDataPack> seisdp = dpm_.obtain( inputdp_->id() );
    mDynamicCastGet(const RegularSeisDataPack*,regsdp,seisdp.ptr());
    mDynamicCastGet(const RandomSeisDataPack*,randsdp,seisdp.ptr());
    if ( !(regsdp || randsdp) || seisdp->isEmpty() )
	return false;

    const char* category = seisdp->category();
    if ( regsdp )
    {
	TrcKeyZSampling tkzs( regsdp->sampling() );
	tkzs.zsamp_.setFrom( zrange_ );
	RegularSeisDataPack* output = new RegularSeisDataPack( category );
	output->setSampling( tkzs );
	outputdp_ = output;
    }
    else if ( randsdp )
    {
	RandomSeisDataPack* output = new RandomSeisDataPack( category );
	output->setPath( randsdp->getPath() );
	output->setZRange( zrange_ );
	outputdp_ = output;
    }

    for ( int idx=0; idx<seisdp->nrComponents(); idx++ )
	outputdp_->addComponent( seisdp->getComponentName(idx) );

    return true;
}


bool SeisDataPackZAxisTransformer::doWork(
				od_int64 start, od_int64 stop, int threadid )
{
    ConstDataPackRef<SeisDataPack> seisdp = dpm_.obtain( inputdp_->id() );
    if ( !seisdp || !outputdp_ || outputdp_->isEmpty() )
	return false;

    const StepInterval<float>& inpzrg = seisdp->getZRange();
    const int nrinpsamp = inpzrg.nrSteps() + 1;
    const int nroutsamp = zrange_.nrSteps() + 1;
    const int nrtrcs = (int)(seisdp->nrTrcs()
			  / seisdp->data(0).info().getSize(0));

    ZAxisTransformSampler outputsampler( transform_, true,
	    SamplingData<double>(zrange_.start, zrange_.step), false );

    Array1DSlice<float> arr1dslice( seisdp->data(0) );
    arr1dslice.setDimMap( 0, 2 );

    for ( int idx=0; idx<outputdp_->nrComponents(); idx++ )
    {
	for ( int posidx=mCast(int,start); posidx<=mCast(int,stop); posidx++ )
	{
	    const int lineidx = posidx / nrtrcs;
	    const int trcidx = posidx % nrtrcs;
	    arr1dslice.setPos( 0, lineidx );
	    arr1dslice.setPos( 1, trcidx );
	    if ( !arr1dslice.init() )
		continue;

	    SampledFunctionImpl<float,ValueSeries<float> > inputfunc(
		    arr1dslice, nrinpsamp, inpzrg.start, inpzrg.step );
	    inputfunc.setHasUdfs( true );
	    inputfunc.setInterpolate( interpolate_ );

	    outputsampler.setTrcKey( seisdp->getTrcKey(posidx) );
	    outputsampler.computeCache( Interval<int>(0,nroutsamp-1) );

	    Array3D<float>& array = outputdp_->data( idx );
	    float* dataptr = array.getData();
	    if ( dataptr )
	    {
		float* arrptr = dataptr + posidx * array.info().getSize(2);
		reSample( inputfunc, outputsampler, arrptr, nroutsamp );
	    }
	    else
	    {
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
    ConstDataPackRef<SeisDataPack> seisdp = dpm_.obtain( inputdp_->id() );
    if ( !seisdp ) return false;

    outputdp_->setZDomain( transform_.toZDomainInfo() );
    outputdp_->setName( seisdp->name() );
    dpm_.add( outputdp_ );
    if ( outputid_ )
	*outputid_ = outputdp_->id();

    return true;
}

