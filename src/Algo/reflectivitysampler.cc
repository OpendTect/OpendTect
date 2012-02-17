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
    const float tapersz = (int)( size/10 );
    const float maxfreq = nyqfreq + tapersz*df;
    LinScaler cosscale( nyqfreq, 0, maxfreq, M_PI/2 );

    const float_complex* stopptr = buffer+size;
    for ( int idx=start; idx<=stop; idx++ )
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
	    const float angle = 2*M_PI *anglesampling * freqidx;
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




InternalMultipleComputer::InternalMultipleComputer( const ReflectivityModel& rm,
				    const StepInterval<float>& timesampling,
				    TypeSet<float_complex>& outp )
    : model_(rm)
    , output_(outp)	       
    , size_(timesampling.nrSteps() + 1)
    , outsampling_(timesampling)    
{
    workset_.setSize( size_, 0 );

    TypeSet<int> pos; 
    for ( int idx=0; idx<rm.size(); idx++ )
	pos += timesampling.nearestIndex( rm[idx].time_ );

    for ( int idx=0; idx<size_; idx++ )
    {
	for ( int idpos=0; idpos<pos.size(); idpos++ )
	{
	    if ( idx == pos[idpos] )
		workset_[idx] = rm[idpos].reflectivity_;
	}
    }

    Polynomial P0; P0 += 1;
    Polynomial Q0; Q0 += 0;
    Ps_ += P0;
    Qs_ += Q0;

    computePolynomials();
    computeOutput();
}


void InternalMultipleComputer::Polynomial::substract( const Polynomial& Q)
{
    int idq = 0;
    for ( int idp=0; idp<this->size(); idp++ )
    {
	idq = idp;
	if ( idq < Q.size() )
	    (*this)[idp] -= Q[idq];
    }
    idq ++;
    while ( idq < Q.size() )
	{ (*this) += -Q[idq]; idq ++; }
} 


void InternalMultipleComputer::Polynomial::multiplyByConst( float_complex cval) 
{
    for ( int idx =0; idx<this->degree(); idx++ )
	(*this)[idx] *= cval;
}


void InternalMultipleComputer::Polynomial::increaseDegree( int by ) 
{
    for ( int idx =0; idx<by; idx++ )
	this->insert( 0, 0 );
}


void InternalMultipleComputer::computePolynomials()
{
    for ( int idx=1; idx<size_+1; idx++ )
    {
	float_complex ref = workset_[idx-1];

	Polynomial Q( Qs_[idx-1] );
	Polynomial QRev( Q ); QRev.reverse();
	QRev.increaseDegree(); QRev.multiplyByConst( ref );

	Polynomial P( Ps_[idx-1] );
	Polynomial PRev( P ); PRev.reverse();
	PRev.increaseDegree(); PRev.multiplyByConst( ref );

	P.substract( QRev );
	Q.substract( PRev );

	Ps_ += P; 
	Qs_ += Q;
    }
}


void InternalMultipleComputer::computeOutput()
{
    const Polynomial& P = Ps_[Ps_.size()-1];
    for ( int idp=0; idp<outsampling_.nrSteps()+1; idp ++ )
    {
	float_complex val = 0;
	const float_complex ref = workset_[idp];
	for ( int idk=1; idk<idp; idk++)
	{
	    val += P[idk] * output_[idp-idk];
	}
	output_ += ref - val; 
    }
    output_[0] = 0; //shot
}
