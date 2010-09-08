/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl/Y.Liu
 * DATE     : 8-20-2010
-*/

static const char* rcsID = "$Id: convolve2d.cc,v 1.1 2010-09-08 20:50:09 cvskris Exp $";

#include "convolve2d.h"

#include "arrayndimpl.h"
#include "fourier.h"

template <>
Convolver2D<float>::~Convolver2D()
{
    delete [] xf_;
    delete [] yf_;
    delete [] zf_;

    delete fft_;
}

template <>
bool Convolver2D<float>::doPrepare( int )
{
    if ( !shouldFFT() )
    {
	delete fft_;
	fft_ = 0;
	return true;
    }

    if ( !fft_ )
    {
	fft_ = new Fourier::CC;
	fft_->setInputInfo( x_->info() );
    }

    return true;
}

#define mInitFreqDomain( realdomain, freqdomain ) \
    if ( !freqdomain ) \
    { \
	freqdomain = new float_complex[totalsz]; \
	const float* xptr = realdomain->getData(); \
	const float* stopptr = xptr + totalsz; \
	float* xfptr = (float*) freqdomain; \
	memset( xfptr, 0, sizeof(float_complex)*totalsz ); \
 \
	while ( xptr!=stopptr ) \
	{ \
	    *xfptr = *xptr; \
	    xptr++; \
	    xfptr += 2; \
	} \
 \
	fft_->setInput( freqdomain ); \
	fft_->setOutput( freqdomain ); \
	fft_->setDir( true ); \
	fft_->execute(); \
    }

template <>
bool Convolver2D<float>::doWork( od_int64 start, od_int64 stop, int threadid )
{
    if ( !fft_ )
	return doNonFFTWork( start, stop, threadid );

    if ( start || stop || threadid )
	return false;

    const int totalsz = x_->info().getTotalSz();

    mInitFreqDomain( x_, xf_ );
    mInitFreqDomain( y_, yf_ );
    if ( !zf_ ) zf_ = new float_complex[totalsz];

    float_complex* zfptr = zf_;
    const float_complex* xfptr = xf_;
    const float_complex* yfptr = yf_;
    const float_complex* stopptr = zfptr + totalsz;

    while ( zfptr!=stopptr )
    {
	*zfptr = *xfptr * *yfptr;
	xfptr++;
	yfptr++;
	zfptr++;
    }

    fft_->setInput( zf_ );
    fft_->setOutput( zf_ );
    fft_->setDir( false );
    fft_->execute();

    float* zptr = z_->getData();
    float* zfptr_real = (float*) zf_;

    while ( zfptr!=stopptr )
    {
	*zptr = *zfptr_real;
	zptr++;
	zfptr_real += 2; 
    }

    return true;
}

