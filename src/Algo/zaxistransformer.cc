/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2007
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "zaxistransformer.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "genericnumer.h"
#include "samplfunc.h"
#include "zaxistransform.h"

#define mInl	0
#define mCrl	1
#define mZ	2


ZAxisTransformer::ZAxisTransformer( ZAxisTransform& zat, bool forward )
    : transform_(zat)
    , forward_(forward)
    , input_(0)
    , output_(0)
    , voiid_(-1)
    , interpolate_(false)
    , rmvoi_(true)
{
    transform_.ref();
}


ZAxisTransformer::~ZAxisTransformer()
{
    if ( rmvoi_ )
	transform_.removeVolumeOfInterest( voiid_ );

    transform_.unRef();
    delete output_;
}


bool ZAxisTransformer::setInput( const Array3D<float>& arr,
				 const CubeSampling& cs )
{
    input_ = &arr;
    inputcs_ = cs;

    return true;
}


void ZAxisTransformer::setOutputRange( const CubeSampling& cs )
{
    outputcs_ = cs;
}


bool ZAxisTransformer::doPrepare(int) 
{
    delete output_;
    output_ = new Array3DImpl<float>( outputcs_.hrg.nrInl(),
	    			      outputcs_.hrg.nrCrl(),
				      outputcs_.zrg.nrSteps()+1 );
    if ( !output_ || !output_->isOK() )
	return false;

    return loadTransformData();
}


bool ZAxisTransformer::loadTransformData( TaskRunner* taskrunner )
{
    if ( voiid_==-1 )
	voiid_ = transform_.addVolumeOfInterest( outputcs_, forward_ );
    else
	transform_.setVolumeOfInterest( voiid_, outputcs_, forward_ );

    return transform_.loadDataIfMissing( voiid_, taskrunner );
}


Array3D<float>* ZAxisTransformer::getOutput( bool transfer )
{
    Array3D<float>* res = output_;
    if ( transfer ) output_ = 0;
    return res;
}


void ZAxisTransformer::setInterpolate( bool yn )
{ interpolate_ = yn; }


bool ZAxisTransformer::getInterpolate() const
{ return interpolate_; }


od_int64 ZAxisTransformer::nrIterations() const
{
    return input_
	? input_->info().getSize(mInl) * input_->info().getSize(mCrl) : 0;
}


bool ZAxisTransformer::doWork( od_int64 start, od_int64 stop, int )
{
    if ( !input_ ) return true;

    ZAxisTransformSampler outpsampler( transform_, forward_,
	SamplingData<double>(outputcs_.zrg.start, outputcs_.zrg.step), false );

    const int inputzsz = input_->info().getSize(mZ);
    const int outputzsz = output_->info().getSize(mZ);

    const int inlsz = input_->info().getSize(mInl);
    const int crlsz = input_->info().getSize(mCrl);
    if ( inputzsz==0 || outputzsz==0 || inlsz==0 || crlsz==0 )
	return false;

    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	const int inlidx = idx / crlsz;
	const int crlidx = idx % crlsz;

	const BinID bid = inputcs_.hrg.atIndex( inlidx, crlidx );
	outpsampler.setBinID( bid );
	outpsampler.computeCache( Interval<int>(0,outputzsz-1) );

	float* outputptr = output_->getData() +
	    output_->info().getOffset(inlidx,crlidx,0);

	if ( input_->getData() )
	{
	    const float* inputptr = input_->getData() +
		input_->info().getOffset(inlidx,crlidx,0);
	    SampledFunctionImpl<float,const float*> inputfunc(
	       inputptr, inputzsz, inputcs_.zrg.atIndex(0), inputcs_.zrg.step);

	    inputfunc.setHasUdfs( true );
	    inputfunc.setInterpolate( interpolate_ );

	    reSample( inputfunc, outpsampler, outputptr, outputzsz );
	}
	else if ( input_->getStorage() )
	{
	    const OffsetValueSeries<float> vs( *input_->getStorage(),
		input_->info().getOffset(inlidx,crlidx,0) );
		
	    SampledFunctionImpl<float,ValueSeries<float> > inputfunc(
	       vs, inputzsz, inputcs_.zrg.atIndex(0), inputcs_.zrg.step);

	    inputfunc.setHasUdfs( true );
	    inputfunc.setInterpolate( interpolate_ );

	    reSample( inputfunc, outpsampler, outputptr, outputzsz );
	}
	else
	{
	    Array1DSlice<float> vs( *input_ );
	    vs.setDimMap( 0, mZ );
	    vs.setPos( mInl, inlidx );
	    vs.setPos( mCrl, crlidx );
	    if ( !vs.init() )
		return false;

	    SampledFunctionImpl<float,ValueSeries<float> > inputfunc(
	       vs, inputzsz, inputcs_.zrg.atIndex(0), inputcs_.zrg.step);

	    inputfunc.setHasUdfs( true );
	    inputfunc.setInterpolate( interpolate_ );

	    reSample( inputfunc, outpsampler, outputptr, outputzsz );
	}
    }

    return true;
}
