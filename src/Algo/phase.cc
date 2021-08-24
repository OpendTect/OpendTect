/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A. Huck
 * DATE     : Dec 2013
-*/


#include "phase.h"
#include "arrayndimpl.h"
#include "fourier.h"
#include "math2.h"
#include "odcomplex.h"


Phase::Phase( const Array1DImpl<float_complex>& cfrequencies )
    : cfreq_(*new Array1DImpl<float_complex>(cfrequencies))
    , phase_(*new Array1DImpl<float>(0))
    , domfreqidx_(-1)
    , avgphase_(0.f)
{
    init();
}

#define mDoFFT( isforward, inp, outp, sz )\
{\
	PtrMan<Fourier::CC> fft = Fourier::CC::createDefault(); \
	fft->setInputInfo( Array1DInfoImpl(sz) ); \
	fft->setDir( isforward ); \
	fft->setNormalization(!isforward); \
	fft->setInput( inp.getData() ); \
	fft->setOutput( outp.getData() ); \
	fft->run( true ); \
}


Phase::Phase( const Array1DImpl<float>& timesignal )
    : cfreq_(*new Array1DImpl<float_complex>(0))
    , phase_(*new Array1DImpl<float>(0))
    , domfreqidx_(-1)
    , avgphase_(0.f)
{
    const int sz = timesignal.info().getSize(0);
    if ( !sz )
	return;

    Array1DImpl<float_complex> signal( sz );
    for ( int idx=0; idx<sz; idx++ )
	signal.set( idx, float_complex(timesignal.get(idx),0.f) );

    cfreq_.setSize( sz );
    mDoFFT( true, signal, cfreq_, sz );

    init();
}


Phase::~Phase()
{
    delete &cfreq_;
    delete &phase_;
}


void Phase::init()
{
    const int sz = cfreq_.info().getSize(0);
    phase_.setSize( sz );
    phase_.setAll( mUdf(float) );
}


bool Phase::calculate( bool unwrap )
{
    bool res = extract();
    if ( res && unwrap )
	res = unWrap();
    if ( res )
	res = convert();
    return res;
}


bool Phase::convert()
{
    if ( !indegrees_ )
	return true;

    const int sz = phase_.info().getSize(0);
    for ( int idx=0; idx<sz; idx++ )
    {
	const float ph = phase_.get( idx );
	if ( mIsUdf(ph) ) continue;

	phase_.set( idx, Math::toDegrees(ph) );
    }

    if ( !mIsUdf(avgphase_) )
	avgphase_ = Math::toDegrees( avgphase_ );

    return true;
}


bool Phase::extract()
{
    float max = -1.f;
    const int sz = cfreq_.info().getSize(0);
    for ( int idx=0;idx<sz;idx++ )
    {
	const float val = std::abs( cfreq_.get(idx) );
	if ( mIsUdf(val) )
	    continue;

	if ( val > max && idx < sz/2 ) //only scanning the positive frequencies
	{
	    domfreqidx_ = idx;
	    max = val;
	}

	phase_.set( idx, std::arg( cfreq_.get(idx) ) );
    }

    return domfreqidx_ >= 0;
}


bool Phase::unWrap( float wrapparam )
{
    const int sz = cfreq_.info().getSize(0);
    if ( domfreqidx_ < 0 || domfreqidx_ >= sz )
    {
	return false;
    }

    if ( mIsZero(wrapparam,1e-6f) )
    {
	pFreeFnErrMsg("wrapping parameter is zero");
	return false;
    }

    float prevph = phase_.get( domfreqidx_ );
    float domval = std::abs( cfreq_.get(domfreqidx_) );
    float prevdph = 0.f;
    const float pibyw = mCast(float, wrapparam/M_PI );
    int phcount = 0;
    for ( int idx=domfreqidx_+1; idx<sz; idx++ )
    {
	const float val = std::abs( cfreq_.get(idx) );
	const float ph = phase_.get(idx);
	const float dph = fabs( ph - prevph );
	const bool unwrap = !mIsEqual( dph, prevdph, pibyw ) || mIsUdf(ph) ||
			    val/domval < 0.20f;
	prevph += unwrap ? prevdph : dph;
	if ( !unwrap )
	{
	    prevdph = dph;
	    avgphase_ += ph;
	    phcount++;
	}

	phase_.set( idx, prevph );
    }

    prevph = phase_.get( domfreqidx_ );
    domval = std::abs( cfreq_.get(domfreqidx_) );
    prevdph = 0.f;
    for ( int idx=domfreqidx_-1;idx>=0; idx-- )
    {
	const float val = std::abs( cfreq_.get(idx) );
	const float ph = phase_.get(idx);
	const float dph = fabs( ph - prevph );
	const bool unwrap = !mIsEqual( dph, prevdph, pibyw ) || mIsUdf(ph) ||
			    val/domval < 0.20f;
	prevph += unwrap ? prevdph : dph;
	if ( !unwrap )
	{
	    prevdph = dph;
	    avgphase_ += ph;
	    phcount++;
	}

	phase_.set( idx, prevph );
    }

    if ( phcount != 0 )
	avgphase_ /= mCast(float,phcount);
    else
	avgphase_ = phase_.get( domfreqidx_ );

    return true;
}
