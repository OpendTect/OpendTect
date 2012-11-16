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
				TypeSet<float_complex>& output,
				bool usenmotime )
    : model_( model )
    , outsampling_( outsampling )
    , output_( output )
    , usenmotime_( usenmotime )
    , fft_( new Fourier::CC )
{
    buffers_.allowNull( true );
}


ReflectivitySampler::~ReflectivitySampler()
{
    removeBuffers();
    delete fft_;
}


void ReflectivitySampler::removeBuffers()
{
    deepEraseArr( buffers_ );
}


bool ReflectivitySampler::doPrepare( int nrthreads )
{
    removeBuffers();
    output_.erase();

    int sz = outsampling_.nrSteps()+1;
    if ( fft_ )
    {
	sz = fft_->getFastSize( sz );
	fft_->setInputInfo( Array1DInfoImpl(sz) );
	fft_->setDir( false );
	fft_->setNormalization( true );
    }

    output_.setSize( sz, float_complex(0,0) );

    buffers_ += 0;
    for ( int idx=1; idx<nrthreads; idx++ )
	buffers_ += new float_complex[output_.size()];

    return true;
}


bool ReflectivitySampler::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    const int size = output_.size();
    float_complex* buffer;
    if ( threadidx )
    {
	buffer = buffers_[threadidx];
	memset( buffer, 0, size*sizeof(float_complex) );
    }
    else
    {
	buffer = output_.arr();
    }

    const float df = Fourier::CC::getDf( outsampling_.step, size );
    const float nyqfreq = Fourier::CC::getNyqvist( outsampling_.step );
    const float tapersz = mCast( float, (int)( size/10 ) );
    const float maxfreq = nyqfreq + tapersz*df;
    LinScaler cosscale( nyqfreq, 0, maxfreq, M_PI/2 );

    const float_complex* stopptr = buffer+size;
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	const ReflectivitySpike& spike = model_[idx];
	const float time = usenmotime_ ? spike.correctedtime_ : spike.time_ ;
	if ( mIsUdf(time) )
	    continue;

	float_complex reflectivity = spike.reflectivity_;
	if ( mIsUdf( reflectivity ) )
	    reflectivity = float_complex(0,0);

	float_complex* ptr = buffer;

	int freqidx = 0;
	const float anglesampling = -time * df;
	while ( ptr!=stopptr )
	{
	    const float angle = (float) ( 2*M_PI *anglesampling * freqidx );
	    const float freq = df * freqidx;
	    const float_complex cexp = float_complex( cos(angle), sin(angle) );
	    const float_complex cpexref = cexp * reflectivity;
	    *ptr += freq > nyqfreq ?  
			freq > maxfreq ? 0 : cpexref*(float)cosscale.scale(freq)
		      : cpexref ;
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

    const int size = output_.size();
    float_complex* res = output_.arr();
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
	fft_->setInput( output_.arr() );
	fft_->setOutput( output_.arr() );
	fft_->run( true );
    }

    return true;
}


void ReflectivitySampler::setTargetDomain( bool fourier )
{
    if ( fourier != ((bool) fft_ ) )
	return;

    if ( fft_ )
    {
	delete fft_;
	fft_ = 0;
    }
    else
    {
	fft_ = new Fourier::CC;
    }
}




