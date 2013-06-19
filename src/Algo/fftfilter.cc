/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          October 2009
RCS:           $Id$
________________________________________________________________________

*/

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "fftfilter.h"
#include "fourier.h"
#include "hilberttransform.h"
#include "mathfunc.h"

#include <complex>


#define mMINNRSAMPLES	100



DefineEnumNames(FFTFilter,Type,0,"Filter type")
{ "LowPass", "HighPass", "BandPass", 0 };


FFTFilter::FFTFilter( int sz, float step )
    : fft_(Fourier::CC::createDefault())
    , step_(step)
    , timewindow_(0)
    , freqwindow_(0)
    , trendreal_(0)
    , trendimag_(0)
    , cutfreq1_(mUdf(float))
    , cutfreq2_(mUdf(float))
    , cutfreq3_(mUdf(float))
    , cutfreq4_(mUdf(float))
{
    sz_ = mMAX( sz, mMINNRSAMPLES );
    fftsz_ = fft_->getFastSize( 3 * sz_ );
    df_ = Fourier::CC::getDf( step, fftsz_ );
}


FFTFilter::~FFTFilter()
{
    delete fft_;
    if ( timewindow_ )
	delete timewindow_;
    if ( freqwindow_ )
	delete freqwindow_;
    if ( trendreal_ )
	delete trendreal_;
    if ( trendimag_ )
	delete trendimag_;
}


void FFTFilter::setLowPass( float cutf3, float cutf4 )
{
    cutfreq3_ = cutf3;
    cutfreq4_ = cutf4;
    buildFreqTaperWin();
}


void FFTFilter::setHighPass( float cutf1, float cutf2 )
{
    cutfreq1_ = cutf1;
    cutfreq2_ = cutf2;
    buildFreqTaperWin();
}


void FFTFilter::setBandPass( float cutf1, float cutf2,
			     float cutf3, float cutf4 )
{
    cutfreq1_ = cutf1;
    cutfreq2_ = cutf2;
    cutfreq3_ = cutf3;
    cutfreq4_ = cutf4;
    buildFreqTaperWin();
}


void FFTFilter::setLowPass( float cutf4 )
{
    setLowPass( cutf4 * 0.95f, cutf4 );
}


void FFTFilter::setHighPass( float cutf1 )
{
    const float nyquistfeq = Fourier::CC::getNyqvist( step_ );
    const float cutf2 = cutf1 + 0.05f * ( nyquistfeq - cutf1 );
    setHighPass( cutf1, cutf2 );
}


void FFTFilter::setBandPass( float cutf1, float cutf4 )
{
    const float cutf2 = cutf1 + 0.05f * ( cutf4 - cutf1 );
    const float cutf3 = cutf4 - 0.05f * ( cutf4 - cutf1 );
    setBandPass( cutf1, cutf2, cutf3, cutf4 );
}


#define mSetTaperWin( win, sz, var )\
    win = new ArrayNDWindow( Array1DInfoImpl(sz), false, "CosTaper", var);


void FFTFilter::buildFreqTaperWin()
{
    if ( !fft_ )
	return;

    if ( !freqwindow_ || !freqwindow_->isOK() )
	freqwindow_ = new ArrayNDWindow( Array1DInfoImpl(fftsz_), true );

    const bool dolowpasstaperwin = getFilterType() != HighPass;
    const bool dohighpasstaperwin = getFilterType() != LowPass;
    const int nyqfreqidx = fftsz_ / 2;
    int taperhp2lp;
    if ( isLowPass() )
	taperhp2lp = 0;
    else if ( isHighPass() )
	taperhp2lp = nyqfreqidx;
    else
	taperhp2lp = mCast(int, ( cutfreq2_ + cutfreq3_ ) / ( 2.f * df_ ) );

    ArrayNDWindow* highpasswin = 0;
    if ( dohighpasstaperwin )
    {
	const int f1idx = mCast( int, cutfreq1_ / df_ );
	const float var = 1.f - ( cutfreq2_ - cutfreq1_ ) /
	   		  	( df_ * taperhp2lp - cutfreq1_ );
	mSetTaperWin( highpasswin, 2 * ( taperhp2lp - f1idx ), var )
	for ( int idx=0; idx<taperhp2lp; idx++ )
	{
	    const float taperval = idx < f1idx ? 0.f
				 : highpasswin->getValues()[idx-f1idx];
	    freqwindow_->setValue( idx, taperval );
	    freqwindow_->setValue( fftsz_-idx-1, taperval );
	}
    }

    ArrayNDWindow* lowpasswin = 0;
    if ( dolowpasstaperwin )
    {
	const int f4idx = mCast( int, cutfreq4_ / df_ );
	const float var = 1.f - ( cutfreq4_ - cutfreq3_ ) /
	    		  	( cutfreq4_ - df_ * taperhp2lp );
	mSetTaperWin( lowpasswin, 2 * ( f4idx - taperhp2lp ), var )
	for ( int idx=taperhp2lp; idx<nyqfreqidx; idx++ )
	{
	    const float taperval = idx < f4idx
	       			 ? lowpasswin->getValues()[f4idx-idx-1]
				 : 0.f;
	    freqwindow_->setValue( idx, taperval );
	    freqwindow_->setValue( fftsz_-idx-1, taperval );
	}
    }
}


bool FFTFilter::setTimeTaperWindow( int sz, BufferString wintype, float var )
{
    delete timewindow_;
    timewindow_ = new ArrayNDWindow( Array1DInfoImpl( sz ),
	    			     false, wintype, var );
    return timewindow_->isOK();
}


#define mDoFFT(isstraight,inp,outp)\
{\
    fft_->setInputInfo(Array1DInfoImpl(fftsz_));\
    fft_->setDir(isstraight);\
    fft_->setNormalization(!isstraight); \
    fft_->setInput(inp);\
    fft_->setOutput(outp);\
    if (!fft_->run(true))\
    	return false;\
}


bool FFTFilter::apply( Array1DImpl<float_complex>& outp, bool dopreproc )
{
    const int sz = outp.info().getSize(0);
    if ( !sz || !fft_ )
	return false;

    const bool needresize = sz < mMINNRSAMPLES;
    Array1DImpl<float_complex> signal( sz );
    if ( dopreproc )
    {
	if ( !deTrend(outp) || !interpUdf(outp) )
	    return false;

	if ( timewindow_ )
	    timewindow_->apply( &outp );

	if ( needresize )
	    reSize( outp, signal );
    }

    Array1DImpl<float_complex>* inp = needresize ? &signal : &outp;
    Array1DImpl<float_complex> timedomain( fftsz_ );
    for ( int idy=0; idy<sz_; idy++ )
	timedomain.set( sz_+idy, inp->get( idy ) );

    Array1DImpl<float_complex> freqdomain( fftsz_ );
    mDoFFT( true, timedomain.getData(), freqdomain.getData() );
    if ( freqwindow_ )
	freqwindow_->apply( &freqdomain );

    mDoFFT( false, freqdomain.getData(), timedomain.getData() );
    for ( int idy=0; idy<sz_; idy++ )
	inp->set( idy, timedomain.get( sz_ + idy ) );

    if ( needresize )
	restoreSize( signal, outp );

    if ( dopreproc )
    {
	if ( isLowPass() )
	{
	    if ( !restoreTrend(outp) )
		return false;
	}
	else
	    restoreUdf( outp );
    }

    return true;
}


#define mDoHilbert(inp,outp)\
{\
    HilbertTransform hilbert;\
    hilbert.setInputInfo( Array1DInfoImpl(sz_) );\
    hilbert.setCalcRange( 0, sz_, 0 );\
    hilbert.setDir( true );\
    hilbert.init();\
    if (!hilbert.transform(inp,outp) )\
    	return false;\
}


bool FFTFilter::apply( Array1DImpl<float>& outp )
{
    const int sz = outp.info().getSize(0);
    if ( !sz )
	return false;

    if ( !deTrend(outp) || !interpUdf(outp) )
	return false;

    const bool needresize = sz < mMINNRSAMPLES;
    Array1DImpl<float> signal( sz );
    reSize( outp, signal );

    Array1DImpl<float>* inp = needresize ? &signal : &outp;
    Array1DImpl<float> hilberttrace( sz_ );
    mDoHilbert(*inp,hilberttrace)

    Array1DImpl<float_complex> ctrace( sz_ );
    for ( int idx=0; idx<sz_; idx++ )
	ctrace.set( idx, float_complex( inp->get( idx ),
		    -1.f * hilberttrace.get( idx ) ) );

    if ( !apply(ctrace,false) )
	return false;

    for ( int idx=0; idx<sz_; idx++ )
	inp->set( idx, ctrace.get( idx ).real() );

    if ( needresize )
	restoreSize( *inp, outp );

    if ( isLowPass() )
    {
	if ( !restoreTrend(outp) )
	    return false;
    }
    else
	restoreUdf( outp );

    return true;
}


FFTFilter::Type FFTFilter::getFilterType() const
{
    if ( isLowPass() )
	return LowPass;

    if ( isHighPass() )
	return HighPass;

    return BandPass;
}


bool FFTFilter::isLowPass() const
{
    return mIsUdf(cutfreq1_) && mIsUdf(cutfreq2_) &&
          !mIsUdf(cutfreq3_) && !mIsUdf(cutfreq4_);
}


bool FFTFilter::isHighPass() const
{
    return !mIsUdf(cutfreq1_) && !mIsUdf(cutfreq2_) &&
            mIsUdf(cutfreq3_) && mIsUdf(cutfreq4_);
}


bool FFTFilter::interpUdf( Array1DImpl<float>& outp, bool isimag )
{
    const int sz = outp.info().getSize(0);
    if ( !sz )
	return false;

    BoolTypeSet& isudf = isimag ? isudfimag_ : isudfreal_;
    if ( !isudf.isEmpty() || isudf.size() != sz )
    {
	isudf.setEmpty();
	isudf.setSize( sz );
    }

    PointBasedMathFunction data( PointBasedMathFunction::Poly,
	    			 PointBasedMathFunction::EndVal );
    for ( int idx=0; idx<sz; idx++ )
    {
	const float val = outp[idx];
	const bool validval = !mIsUdf(val);
	isudf[idx] = !validval;
	if ( validval )
	    data.add( idx, val );
    }

    if ( data.isEmpty() )
	return false;

    if ( data.size() == sz )
	return true;

    for ( int idx=0; idx<sz; idx++ )
    {
	if ( isudf[idx] )
	    outp.set( idx, data.getValue( idx ) );
    }

    return true;
}


bool FFTFilter::interpUdf( Array1DImpl<float_complex>& outp )
{
    const int sz = outp.info().getSize(0);
    if ( !sz )
	return false;

    Array1DImpl<float> realvals( sz );
    Array1DImpl<float> imagvals( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	realvals.set( idx, outp[idx].real() );
	imagvals.set( idx, outp[idx].imag() );
    }

    if ( !interpUdf(realvals) || !interpUdf(imagvals,true) )
	return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	if ( isudfreal_[idx] || isudfimag_[idx] )
	{
	    const float_complex val( realvals[idx], imagvals[idx] );
	    outp.set( idx, val );
	}
    }

    return true;
}


void FFTFilter::restoreUdf( Array1DImpl<float>& outp, bool isimag ) const
{
    const int sz = outp.info().getSize(0);
    const BoolTypeSet& isudf = isimag ? isudfimag_ : isudfreal_;
    if ( !sz || (isudf.size() != sz) )
	return;

    for ( int idx=0; idx<sz; idx++ )
	if ( isudf[idx] )
	    outp.set( idx, mUdf(float) );

}


void FFTFilter::restoreUdf( Array1DImpl<float_complex>& outp ) const
{
    const int sz = outp.info().getSize(0);
    if ( !sz || (isudfreal_.size() != sz) || (isudfimag_.size() != sz) )
	return;

    for ( int idx=0; idx<sz; idx++ )
    {
	const bool replacerealval = isudfreal_[idx];
	const bool replaceimagval = isudfimag_[idx];
	if ( replacerealval || replaceimagval )
	{
	    const float realval = replacerealval ? mUdf(float)
						 : outp[idx].real();
	    const float imagval = replaceimagval ? mUdf(float)
	       					 : outp[idx].imag();
	    outp.set( idx, float_complex( realval, imagval ) );
	}
    }
}


bool FFTFilter::deTrend( Array1DImpl<float>& outp, bool isimag )
{
    const int sz = outp.info().getSize(0);
    if ( !sz )
	return false;

    Array1DImpl<float> inp( sz );
    Array1DImpl<float>* trend = new Array1DImpl<float>( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	inp.set( idx, outp[idx] );
	trend->set( idx, outp[idx] );
    }

    if ( !removeBias<float,float>(&inp,&outp,false) )
	return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	const float inpval = inp[idx];
	if ( mIsUdf(inpval) )
	    continue;

	const float notrendval = outp[idx];
	trend->set( idx, inpval - notrendval );
    }

    if ( isimag )
    {
       if ( trendimag_ ) delete trendimag_;
    }
    else
       if ( trendreal_ ) delete trendreal_;

    if ( isimag )
	trendimag_ = trend;
    else
	trendreal_ = trend;

    return true;
}


bool FFTFilter::deTrend( Array1DImpl<float_complex>& outp )
{
    const int sz = outp.info().getSize(0);
    if ( !sz )
	return false;

    Array1DImpl<float> real( sz );
    Array1DImpl<float> imag( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	real.set( idx, outp.get( idx ).real() );
	imag.set( idx, outp.get( idx ).imag() );
    }

    if ( !deTrend(real) || !deTrend(imag,true) )
	return false;

    for ( int idx=0; idx<sz; idx++ )
	outp.set( idx, float_complex( real[idx], imag[idx] ) );

    return true;
}


bool FFTFilter::restoreTrend( Array1DImpl<float>& outp, bool isimag ) const
{
    const int sz = outp.info().getSize(0);
    const Array1DImpl<float>* trend = isimag ? trendimag_ : trendreal_;
    if ( !sz || !trend )
	return false;

    if ( trend->info().getSize(0) != sz )
	return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	const float trendval = trend->get( idx );
	const float outval = mIsUdf(trendval) ? mUdf(float)
	    				      : trendval + outp[idx];
	outp.set( idx, outval );
    }

    return true;
}


bool FFTFilter::restoreTrend( Array1DImpl<float_complex>& outp ) const
{
    const int sz = outp.info().getSize(0);
    if ( !sz )
	return true;

    Array1DImpl<float> real( sz );
    Array1DImpl<float> imag( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	real.set( idx, outp.get( idx ).real() );
	imag.set( idx, outp.get( idx ).imag() );
    }

    if ( !restoreTrend(real) || !restoreTrend(imag,true) )
	return false;

    for ( int idx=0; idx<sz; idx++ )
	outp.set( idx, float_complex( real[idx], imag[idx] ) );

    return true;
}


void FFTFilter::reSize( const Array1DImpl<float_complex>& inp,
			Array1DImpl<float_complex>& outp ) const
{
    const int sz = inp.info().getSize(0);
    const int shift = mNINT32((float) sz/2) - mNINT32((float) sz_/2);
    for ( int idx=0; idx<sz_; idx++ )
    {
	const int cidx = idx + shift;
	const int idy = cidx < 0 ? 0 : cidx >= sz ? sz-1 : cidx;
	outp.set( idx, inp.get( idy ) );
    }
}


void FFTFilter::reSize( const Array1DImpl<float>& inp,
			Array1DImpl<float>& outp ) const
{
    const int sz = inp.info().getSize(0);
    const int shift = mNINT32((float) sz/2) - mNINT32((float) sz_/2);
    for ( int idx=0; idx<sz_; idx++ )
    {
	const int cidx = idx + shift;
	const int idy = cidx < 0 ? 0 : cidx >= sz ? sz-1 : cidx;
	outp.set( idx, inp.get( idy ) );
    }
}


void FFTFilter::restoreSize( const Array1DImpl<float_complex>& inp,
			     Array1DImpl<float_complex>& outp ) const
{
    const int sz = outp.info().getSize(0);
    const int shift = mNINT32((float) sz_/2) - mNINT32((float) sz/2);
    for ( int idx=0; idx<sz; idx++ )
	outp.set( idx, inp.get( idx + shift ) );
}


void FFTFilter::restoreSize( const Array1DImpl<float>& inp,
			     Array1DImpl<float>& outp ) const
{
    const int sz = outp.info().getSize(0);
    const int shift = mNINT32((float) sz_/2) - mNINT32((float) sz/2);
    for ( int idx=0; idx<sz; idx++ )
	outp.set( idx, inp.get( idx + shift ) );
}

