/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		June 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "attribdatapackzaxistransformer.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "flatposdata.h"
#include "genericnumer.h"
#include "samplfunc.h"
#include "zaxistransform.h"

namespace Attrib
{

FlatDataPackZAxisTransformer::FlatDataPackZAxisTransformer(
						ZAxisTransform& zat )
    : transform_(zat)
    , dpm_(DPM(DataPackMgr::FlatID()))
    , interpolate_(true)
    , inputdp_(0)
    , outputid_(0)
    , arr2d_(0)
{
    transform_.ref();
    zrange_.setFrom( transform_.getZInterval(false) );
    zrange_.step = transform_.getGoodZStep();
}


FlatDataPackZAxisTransformer::~FlatDataPackZAxisTransformer()
{
    transform_.unRef();
}


od_int64 FlatDataPackZAxisTransformer::nrIterations() const
{
    if ( !inputdp_ ) return -1;
    ConstDataPackRef<FlatDataPack> fdp = dpm_.obtain( inputdp_->id() );
    return fdp ? fdp->posData().nrPts(true) : -1;
}


bool FlatDataPackZAxisTransformer::doPrepare( int nrthreads )
{
    if ( !inputdp_ ) return false;

    ConstDataPackRef<FlatDataPack> fdp = dpm_.obtain( inputdp_->id() );
    if ( !fdp ) return false;

    const int sz0 = fdp->posData().nrPts( true );
    const int sz1 = zrange_.nrSteps()+1;
    mTryAlloc( arr2d_, Array2DImpl<float>( sz0, sz1 ) );
    if ( !arr2d_->isOK() )
	return false;

    arr2d_->setAll( mUdf(float) );
    return true;
}


bool FlatDataPackZAxisTransformer::doWork(
				od_int64 start, od_int64 stop, int threadid )
{
    ConstDataPackRef<FlatDataPack> fdp = dpm_.obtain( inputdp_->id() );
    mDynamicCastGet(const Flat2DDHDataPack*,dp2ddh,fdp.ptr());
    mDynamicCastGet(const FlatRdmTrcsDataPack*,dprdm,fdp.ptr());
    if ( !(dp2ddh || dprdm) ) return false;

    const StepInterval<double> inpzrg = fdp->posData().range( false );
    const int nroutsamp = zrange_.nrSteps()+1;

    ZAxisTransformSampler outputsampler( transform_, true,
	    SamplingData<double>(zrange_.start, zrange_.step), false );

    Array1DSlice<float> arr1dslice( fdp->data() );
    arr1dslice.setDimMap( 0, 1 );

    for ( int posidx=mCast(int,start); posidx<=mCast(int,stop); posidx++ )
    {
	arr1dslice.setPos( 0, posidx );
	if ( !arr1dslice.init() )
	    continue;

	SampledFunctionImpl<float,ValueSeries<float> > inputfunc(
		arr1dslice, inpzrg.nrSteps()+1, inpzrg.start, inpzrg.step );
	inputfunc.setHasUdfs( true );
	inputfunc.setInterpolate( interpolate_ );

	if ( dp2ddh )
	    outputsampler.setTrcKey( dp2ddh->getTrcKey(posidx) );
	else if ( dprdm )
	    outputsampler.setBinID( (*dprdm->pathBIDs())[posidx] );

	outputsampler.computeCache( Interval<int>(0,nroutsamp-1) );

	float* dataptr = arr2d_->getData();
	if ( dataptr )
	{
	    float* arrptr = dataptr + arr2d_->info().getOffset( posidx, 0 );
	    reSample( inputfunc, outputsampler, arrptr, nroutsamp );
	}
	else
	{
	    for ( int zidx=0; zidx<nroutsamp; zidx++ )
	    {
		const float sampleval = outputsampler[zidx];
		const float outputval = mIsUdf(sampleval) ? mUdf(float) :
					inputfunc.getValue(sampleval);
		arr2d_->set( posidx, zidx, outputval );
	    }
	}
    }

    return true;
}


bool FlatDataPackZAxisTransformer::doFinish( bool success )
{
    ConstDataPackRef<FlatDataPack> fdp = dpm_.obtain( inputdp_->id() );
    mDynamicCastGet(const Flat2DDHDataPack*,dp2ddh,fdp.ptr());
    mDynamicCastGet(const FlatRdmTrcsDataPack*,dprdm,fdp.ptr());
    if ( !(dp2ddh || dprdm) ) return false;

    FlatDataPack* outputdp = 0;
    const SamplingData<float> sd( zrange_.start, zrange_.step );
    if ( dp2ddh )
	outputdp = new Flat2DDHDataPack( dp2ddh->descID(), arr2d_,
					 dp2ddh->getGeomID(), sd,
					 dp2ddh->getTraceRange() );
    else if ( dprdm )
	outputdp = new FlatRdmTrcsDataPack( dprdm->descID(), arr2d_, sd,
					    dprdm->pathBIDs() );

    outputdp->setName( fdp->name() );
    dpm_.add( outputdp );
    if ( outputid_ )
	*outputid_ = outputdp->id();

    return true;
}


} // namespace Attrib

