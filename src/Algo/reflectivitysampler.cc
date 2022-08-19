/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "reflectivitysampler.h"

#include "arrayndinfo.h"
#include "fourier.h"
#include "math2.h"
#include "odmemory.h"
#include "scaler.h"
#include "varlenarray.h"


ReflectivitySampler::ReflectivitySampler()
    : outsampling_(ZSampling::udf())
    , fftsampling_(mUdf(float),mUdf(float))
{
    buffers_.setNullAllowed();
}


ReflectivitySampler::~ReflectivitySampler()
{
    removeBuffers();
}


void ReflectivitySampler::removeBuffers()
{
    deepEraseArr( buffers_ );
}


void ReflectivitySampler::setInput( const ReflectivityModelTrace& model,
				    const float* spikestwt,
				    const ZSampling& outsampling )
{
    model_ = &model;
    spikestwt_ = spikestwt;
    totalnr_ = model.size();

    const ZSampling oldsampling = outsampling;
    outsampling_ = outsampling;
    newsampling_ = !outsampling_.isEqual(oldsampling,1e-8f) ||
		    fftsampling_.isUdf();
}


void ReflectivitySampler::setFreqOutput( ReflectivityModelTrace& reflectivities)
{
    sampledfreqreflectivities_ = &reflectivities;
}


void ReflectivitySampler::setTimeOutput( ReflectivityModelTrace& reflectivities,
					 float_complex* tempvals, int bufsz )
{
    sampledtimereflectivities_ = &reflectivities;
    tempreflectivities_ = tempvals && bufsz == reflectivities.size()
			? tempvals : nullptr;
}


bool ReflectivitySampler::doPrepare( int nrthreads )
{
    if ( !model_ || !spikestwt_ ||
	 (!sampledfreqreflectivities_ && !sampledtimereflectivities_) )
	return false;

    removeBuffers();
    const int sz = outsampling_.nrSteps() + 1;
    ConstPtrMan<Fourier::CC> fft = new Fourier::CC;
    const int fftsz = fft->getFastSize( sz );

    if ( sampledfreqreflectivities_ &&
	 ((sampledfreqreflectivities_->size() != fftsz &&
	  sampledfreqreflectivities_->setSize(fftsz)) ||
	 !sampledfreqreflectivities_->isOK()) )
	return false;
    else if ( !sampledfreqreflectivities_ && !tempreflectivities_ )
    {
	if ( !creflectivities_ )
	    creflectivities_ = new ReflectivityModelTrace( fftsz );
	else if ( creflectivities_->size() != fftsz &&
		 !creflectivities_->setSize(fftsz) )
	    return false;

	if ( !creflectivities_->isOK() )
	    return false;

	tempreflectivities_ = creflectivities_->arr();
    }

    buffers_ += nullptr;
    for ( int idx=1; idx<nrthreads; idx++ )
	buffers_ += new float_complex[fftsz];

    if ( !sampledtimereflectivities_ )
	return true;

    if ( sampledtimereflectivities_->size() != fftsz &&
	 !sampledtimereflectivities_->setSize(fftsz) )
	return false;

    if ( !sampledtimereflectivities_->isOK() )
	return false;

    if ( !tempreflectivities_ )
    {
	if ( !creflectivities_ )
	    creflectivities_ = new ReflectivityModelTrace( fftsz );
	else if ( creflectivities_->size() != fftsz &&
		  !creflectivities_->setSize(fftsz) )
	    return false;

	if ( !creflectivities_->isOK() )
	    return false;

	tempreflectivities_ = creflectivities_->arr();
    }

    if ( newsampling_ )
	updateTimeSamplingCache();

    return true;
}


bool ReflectivitySampler::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    ReflectivityModelTrace* retref = sampledfreqreflectivities_
				   ? sampledfreqreflectivities_.ptr()
				   : sampledtimereflectivities_.ptr();
    const int size = retref->size();
    const float_complex* modelrefs = model_->arr();
    float_complex* buffer;
    buffer = threadidx ? buffers_[threadidx]
		       : (sampledfreqreflectivities_
			   ? sampledfreqreflectivities_->arr()
			   : tempreflectivities_);
    if ( threadidx )
	OD::sysMemZero( buffer, size*sizeof(float_complex) );

    TypeSet<float> frequencies;
    Fourier::CC::getFrequencies( outsampling_.step, size, frequencies );
    TypeSet<float> angvel;
    const float fact = 2.f * M_PIf;
    for ( int idx=0; idx<size; idx++ )
	angvel += frequencies[idx] * fact;

    const float_complex* stopptr = buffer+size;
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	const float_complex reflectivity = modelrefs[idx];
	const float time = spikestwt_[idx];
	if ( mIsUdf(reflectivity) || mIsUdf(time) ||
	     !outsampling_.includes(time,false) )
	    continue;

	float_complex* ptr = buffer;

	int freqidx = 0;
	while ( ptr!=stopptr )
	{
	    const float angle = angvel[freqidx] * -1.f * time;
	    const float_complex cexp = float_complex( cos(angle), sin(angle) );
	    *ptr += cexp * reflectivity;
	    ptr++;
	    freqidx++;
	}
    }

    return true;
}


bool ReflectivitySampler::doFinish( bool success )
{
    if ( !success )
	return false;

    ReflectivityModelTrace* retref = sampledfreqreflectivities_
				   ? sampledfreqreflectivities_.ptr()
				   : sampledtimereflectivities_.ptr();
    const int size = retref->size();
    float_complex* res = sampledfreqreflectivities_
		       ? sampledfreqreflectivities_->arr()
		       : tempreflectivities_;
    const float_complex* stopptr = res+size;

    const int nrbuffers = buffers_.size()-1;
    mAllocVarLenArr( float_complex*, buffers, nrbuffers );
    for ( int idx=nrbuffers-1; idx>=0; idx-- )
	buffers[idx] = buffers_[idx+1];

    while ( res!=stopptr )
    {
	for ( int idx=nrbuffers-1; idx>=0; idx-- )
	{
	    *res += *buffers[idx];
	    buffers[idx]++;
	}

	res++;
    }

    removeBuffers();
    return sampledtimereflectivities_ ? computeSampledTimeReflectivities()
				      : true;
}


void ReflectivitySampler::updateTimeSamplingCache()
{
    if ( outsampling_.isUdf() )
	return;

    const int sz = outsampling_.nrSteps() + 1;
    const float step = outsampling_.step;
    float start = mCast( float, mCast( int, outsampling_.start/step ) ) * step;
    if ( start < outsampling_.start - 1e-4f )
	start += step;

    width_ = step * sz;
    const int nperiods = mCast( int, Math::Floor( start/width_ ) ) + 1;
    fftsampling_.start = start;
    fftsampling_.step = step;
    firsttwt_ = width_ * nperiods;
    stoptwt_ = start + width_;

    stopidx_ = fftsampling_.nearestIndex( firsttwt_ );
    startidx_ = sampledtimereflectivities_->size() - stopidx_;
}


bool ReflectivitySampler::applyInvFFT()
{
    PtrMan<Fourier::CC> fft = new Fourier::CC;
    if ( !fft )
	return false;

    const int fftsz = sampledtimereflectivities_->size();
    const float_complex* freqreflectivities = sampledfreqreflectivities_
				? sampledfreqreflectivities_->arr()
				: tempreflectivities_;
    fft->setInputInfo( Array1DInfoImpl(fftsz) );
    fft->setDir( false );
    fft->setNormalization( true );
    fft->setInput( freqreflectivities );
    fft->setOutput( sampledtimereflectivities_->arr() );

    return fft->run( true );
}


bool ReflectivitySampler::computeSampledTimeReflectivities()
{
    const int fftsz = sampledtimereflectivities_->size();
    if ( !applyInvFFT() )
	return false;

    const float_complex* rawtimerefs = sampledtimereflectivities_->arr();
    float_complex* temprefs = tempreflectivities_;
    const int sz = outsampling_.nrSteps() + 1;
    float twt = firsttwt_;
    for ( int idx=0; idx<fftsz; idx++, twt+=fftsampling_.step )
    {
	if ( twt > stoptwt_ - 1e-4f )
	    twt -= width_;
	const int idy = fftsampling_.nearestIndex( twt );
	if ( idy < 0 || idy > sz-1 )
	{
	    temprefs[idx] = 0;
	    continue;
	}

	temprefs[idx] = rawtimerefs[idx];
    }

    const float_complex* unsortedrefs = temprefs;
    float_complex* sampledtimerefls = sampledtimereflectivities_->arr();
    OD::sysMemCopy( sampledtimerefls, unsortedrefs+startidx_,
		    stopidx_*sizeof(float_complex) );
    OD::sysMemCopy( sampledtimerefls+stopidx_, unsortedrefs,
		    startidx_*sizeof(float_complex) );

    return true;
}


bool ReflectivitySampler::unSort( const float_complex* inparr, int inbufsz,
				  float_complex* outarr, int outbufsz ) const
{
    if ( !sampledtimereflectivities_ )
	return false;

    const int fftsz = sampledtimereflectivities_->size();
    if ( !inparr || !outarr || inbufsz != fftsz || outbufsz != fftsz )
	return false;

    OD::sysMemCopy( outarr+startidx_, inparr,
		    stopidx_*sizeof(float_complex) );
    OD::sysMemCopy( outarr, inparr+stopidx_,
		    startidx_*sizeof(float_complex) );

    return true;
}
