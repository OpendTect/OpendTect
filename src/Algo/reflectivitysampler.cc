/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2011
-*/


#include "reflectivitysampler.h"

#include "arrayndinfo.h"
#include "fourier.h"
#include "scaler.h"
#include "varlenarray.h"


ReflectivitySampler::ReflectivitySampler(const ReflectivityModel& model,
				const StepInterval<float>& outsampling,
				TypeSet<float_complex>& freqreflectivities,
				bool usenmotime )
    : model_(model)
    , outsampling_(outsampling)
    , freqreflectivities_(freqreflectivities)
    , usenmotime_(usenmotime)
    , fft_(0)
    , creflectivities_(0)
{
    buffers_.allowNull( true );
}


ReflectivitySampler::~ReflectivitySampler()
{
    removeBuffers();
    if ( fft_ )
    {
	delete fft_;
	delete creflectivities_;
    }
}


void ReflectivitySampler::doTimeReflectivities()
{
    fft_ = new Fourier::CC;
    creflectivities_ = new TypeSet<float_complex>;
}


TypeSet<float_complex>& ReflectivitySampler::reflectivities( bool time ) const
{
    return time && fft_ && creflectivities_ ? *creflectivities_
					    : freqreflectivities_;
}


void ReflectivitySampler::getTimeReflectivities( TypeSet<float>& refls ) const
{
    if ( !fft_ || !creflectivities_ )
	return;

    refls.setEmpty();
    for ( int idx=0; idx<creflectivities_->size(); idx++ )
	refls += (*creflectivities_)[idx].real();
}


void ReflectivitySampler::removeBuffers()
{
    deepEraseArr( buffers_ );
}


bool ReflectivitySampler::doPrepare( int nrthreads )
{
    removeBuffers();
    int sz = outsampling_.nrSteps() + 1;
    if ( fft_ )
    {
	creflectivities_->erase();
	sz = fft_->getFastSize( sz );
	creflectivities_->setSize( sz, float_complex(0,0) );
	fft_->setInputInfo( Array1DInfoImpl(sz) );
	fft_->setDir( false );
	fft_->setNormalization( true );
    }

    freqreflectivities_.erase();
    freqreflectivities_.setSize( sz, float_complex(0,0) );
    buffers_ += 0;
    for ( int idx=1; idx<nrthreads; idx++ )
	buffers_ += new float_complex[freqreflectivities_.size()];

    return true;
}


bool ReflectivitySampler::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    const int size = freqreflectivities_.size();
    float_complex* buffer;
    buffer = threadidx ? buffers_[threadidx] : freqreflectivities_.arr();
    if ( threadidx )
	memset( buffer, 0, size*sizeof(float_complex) );

    TypeSet<float> frequencies;
    Fourier::CC::getFrequencies( outsampling_.step, size, frequencies );
    TypeSet<float> angvel;
    const float fact = 2.0f * M_PIf;
    for ( int idx=0; idx<size; idx++ )
	angvel += frequencies[idx] * fact;

    const float_complex* stopptr = buffer+size;
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	const ReflectivitySpike& spike = model_[idx];
	if ( !spike.isDefined() )
	    continue;

	const float time = usenmotime_ ? spike.correctedtime_ : spike.time_ ;
	if ( !outsampling_.includes(time,false) )
	    continue;

	const float_complex reflectivity = spike.reflectivity_;
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

	addToNrDone( 1 );
    }

    return true;
}


bool ReflectivitySampler::doFinish( bool success )
{
    if ( !success )
	return false;

    const int size = freqreflectivities_.size();
    float_complex* res = freqreflectivities_.arr();
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

    if ( fft_ )
    {
	applyInvFFT();
	sortOutput();
    }

    return true;
}


bool ReflectivitySampler::applyInvFFT()
{
    if ( !fft_ || !creflectivities_ )
	return false;

    fft_->setInput( freqreflectivities_.arr() );
    fft_->setOutput( creflectivities_->arr() );

    return fft_->run( true );
}


void ReflectivitySampler::sortOutput()
{
    if ( !fft_ || !creflectivities_ )
	return;

    const int fftsz = creflectivities_->size();
    const float step =  outsampling_.step;
    float start = mCast( float, mCast( int, outsampling_.start/step ) ) * step;
    if ( start <  outsampling_.start - 1e-4f )
	start += step;

    const float width = step * fftsz;
    const int nperiods = mCast( int, floor( start/width ) ) + 1;
    const SamplingData<float> fftsampling( start, step );
    const int sz = outsampling_.nrSteps() + 1;

    mDeclareAndTryAlloc( float_complex*, fftrefl, float_complex[fftsz] );
    if ( !fftrefl ) return;
    memcpy( fftrefl, creflectivities_, fftsz*sizeof(float_complex) );
    float_complex* realres = creflectivities_->arr();
    memset( realres, 0, sz*sizeof(float_complex) );

    const float stoptwt = start + width;
    float twt = width * nperiods - step;
    for ( int idx=0; idx<fftsz; idx++ )
    {
	twt += step;
	if ( twt > stoptwt - 1e-4f )
	    twt -= width;

	const int idy = fftsampling.nearestIndex( twt );
	if ( idy < 0 || idy > sz-1 )
	    continue;

	realres[idy] = fftrefl[idx];
    }

    delete [] fftrefl;
}

