/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl/Y.Liu
 * DATE     : 8-20-2010
-*/

static const char* rcsID = "$Id$";

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
	fft_ = Fourier::CC::createDefault();
	fft_->setInputInfo( x_->info() );
	fft_->setNormalization( true );
    }

    return true;
}

#define mInitFreqDomain( realdomain, freqdomain ) \
    if ( !freqdomain || update##freqdomain ) \
    { \
	if ( !freqdomain ) \
	{ \
	    mTryAlloc( freqdomain, float_complex[totalsz] ); \
	    if ( !freqdomain ) \
	        return false; \
	} \
 \
	memset( freqdomain, 0, sizeof(float_complex)*totalsz ); \
 \
	const float* xptr = realdomain->getData(); \
 \
	mPointerOperation( float, (float*) freqdomain, = *xptr, \
			   totalsz*2, += 2; xptr++ ); \
	fft_->setInput( freqdomain ); \
	fft_->setOutput( freqdomain ); \
	fft_->setDir( true ); \
	if ( !fft_->execute() ) \
	    return false; \
	update##freqdomain = false; \
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

    const float_complex* xfptr = xf_;
    const float_complex* yfptr = yf_;

    mPointerOperation( float_complex, zf_, = *xfptr * *yfptr, totalsz,
	   		++; xfptr++; yfptr++ );

    fft_->setInput( zf_ );
    fft_->setOutput( zf_ );
    fft_->setDir( false );
    fft_->execute();

    const float* zfptr_real = (float*) zf_;

    mPointerOperation( float, z_->getData(), = *zfptr_real, totalsz,
	    	       ++; zfptr_real += 2 );

    return true;
}

