/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2011
-*/


#include "reflectivitysampler.h"

#include "arrayndinfo.h"
#include "fourier.h"
#include "math2.h"
#include "odmemory.h"
#include "reflectivitymodel.h"
#include "scaler.h"
#include "varlenarray.h"


Interval<float> ReflectivityModelSet::getTimeRange( bool usenmo ) const
{
    Interval<float> sampling;
    getTimeRange( *this, sampling, usenmo );
    return sampling;
}


bool ReflectivityModelSet::getTimeRange(
				const ObjectSet<ReflectivityModel>& models,
					 Interval<float>& sampling, bool usenmo)
{
    sampling.set( mUdf(float), -mUdf(float) );
    for ( auto model : models )
    {
	const int nrspikes = model->size();
	for ( int idz=0; idz<nrspikes; idz++ )
	{
	    const auto& spike = (*model)[idz];
	    if ( !spike.isDefined() )
		continue;

	    const auto twt = spike.time( usenmo );
	    sampling.include( twt, false );
	    if ( usenmo )
		break;
	}
	if ( !usenmo )
	    continue;

	for ( int idz=nrspikes-1; idz>=0; idz-- )
	{
	    const auto& spike = (*model)[idz];
	    if ( !spike.isDefined() )
		continue;

	    const auto twt = spike.time( usenmo );
	    sampling.include( twt, false );
	    break;
	}
    }

    return sampling.isUdf();
}



ReflectivitySampler::ReflectivitySampler(const ReflectivityModel& model,
				const StepInterval<float>& outsampling,
				TypeSet<float_complex>& freqreflectivities,
				bool usenmotime )
    : model_(model)
    , outsampling_(outsampling)
    , freqreflectivities_(freqreflectivities)
    , usenmotime_(usenmotime)
{
    buffers_.setNullAllowed( true );
}


ReflectivitySampler::~ReflectivitySampler()
{
    removeBuffers();
}


void ReflectivitySampler::getReflectivities( ReflectivityModel& sampledrefmodel)
{
    PointBasedMathFunction timefunc, depthfunc;
    getRefModelInterpolator( depthfunc, false, usenmotime_ );
    getRefModelInterpolator( timefunc, true, !usenmotime_ );

    const int sz = outsampling_.nrSteps() + 1;
    TypeSet<float_complex> creflectivities;
    applyInvFFT( creflectivities );
    sampledrefmodel.setSize( creflectivities.size(), ReflectivitySpike() );
    const float step = outsampling_.step;
    float start = mCast( float, mCast( int, outsampling_.start/step ) ) * step;
    if ( start < outsampling_.start - 1e-4f )
	start += step;
    const float width = step * sz;
    const int nperiods = mCast( int, Math::Floor( start/width ) ) + 1;
    const SamplingData<float> fftsampling( start, step );
    const float stoptwt = start + width;
    float twt = width * nperiods - step;
    for ( int idx=0; idx<creflectivities.size(); idx++ )
    {
	twt += step;
	if ( twt > stoptwt - 1e-4f )
	    twt -= width;
	const int idy = fftsampling.nearestIndex( twt );
	if ( idy < 0 || idy > sz-1 )
	    continue;
	ReflectivitySpike& spike = sampledrefmodel[idx];
	spike.reflectivity_ = creflectivities[idx];
	spike.depth_ = depthfunc.getValue( twt );
	if ( usenmotime_ )
	{
	    spike.correctedtime_ = twt;
	    spike.time_ = timefunc.getValue( spike.depth_ );
	}
	else
	{
	    spike.time_ = twt;
	    spike.correctedtime_ = timefunc.getValue( spike.depth_ );
	}
    }
}


void ReflectivitySampler::removeBuffers()
{
    deepEraseArr( buffers_ );
}


bool ReflectivitySampler::doPrepare( int nrthreads )
{
    removeBuffers();
    int sz = outsampling_.nrSteps() + 1;
    PtrMan<Fourier::CC> fft = new Fourier::CC;
    const int fftsz = fft->getFastSize( sz );
    freqreflectivities_.erase();
    freqreflectivities_.setSize( fftsz, float_complex(0,0) );
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
	OD::sysMemZero( buffer, size*sizeof(float_complex) );

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

	const float time = spike.time( usenmotime_ );
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


    return true;
}


bool ReflectivitySampler::applyInvFFT( TypeSet<float_complex>& creflectivities )
{
    PtrMan<Fourier::CC> fft = new Fourier::CC;
    if ( !fft )
	return false;

    const int fftsz = freqreflectivities_.size();
    creflectivities.setSize( fftsz, float_complex(0,0) );
    fft->setInputInfo( Array1DInfoImpl(fftsz) );
    fft->setDir( false );
    fft->setNormalization( true );
    fft->setInput( freqreflectivities_.arr() );
    fft->setOutput( creflectivities.arr() );

    return fft->run( true );
}


void ReflectivitySampler::getRefModelInterpolator( PointBasedMathFunction& func,
						   bool valueistime,
						   bool isnmo ) const
{
    func.setEmpty();
    func.setInterpolType( PointBasedMathFunction::Linear );
    func.setExtrapolateType( PointBasedMathFunction::ExtraPolGradient );
    for ( int idx=0; idx<model_.size(); idx++ )
    {
	const ReflectivitySpike& spike = model_[idx];
	if ( !spike.isDefined() )
	    continue;

	const float depth = spike.depth_;
	const float time = isnmo ? spike.correctedtime_ : spike.time_;
	if ( valueistime )
	    func.add( depth, time );
	else
	    func.add( time, depth );
    }
}
