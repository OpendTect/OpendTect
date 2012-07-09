/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Nov 2009
 RCS:           $Id: waveletattrib.cc,v 1.13 2012-07-09 15:14:30 cvsbert Exp $
________________________________________________________________________

-*/

#include "waveletattrib.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "fourier.h"
#include "hilberttransform.h"
#include "wavelet.h"


WaveletAttrib::WaveletAttrib( const Wavelet& wvlt )
	: wvltarr_(0)
	, wvltsz_(0)  
{
    setNewWavelet( wvlt );
}


WaveletAttrib::~WaveletAttrib()
{ delete wvltarr_; }


void WaveletAttrib::setNewWavelet( const Wavelet& wvlt )
{
    wvltsz_ = wvlt.size();
    delete wvltarr_; wvltarr_ = new Array1DImpl<float>( wvltsz_ );
    memcpy( wvltarr_->getData(), wvlt.samples(), wvltsz_*sizeof(float) );
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


void WaveletAttrib::getHilbert(Array1DImpl<float>& hilb ) const
{
    HilbertTransform hilbert;
    hilbert.setCalcRange( 0, wvltsz_, 0 );
    hilbert.setInputInfo( Array1DInfoImpl(wvltsz_) );
    hilbert.setDir( true );
    hilbert.init();
    hilbert.transform( *wvltarr_, hilb );
} 


void WaveletAttrib::getPhase( Array1DImpl<float>& phase, bool degree ) const
{
    Array1DImpl<float_complex> cindata( wvltsz_ );
    for ( int sampidx=0; sampidx<wvltsz_; sampidx++ )
	cindata.set( sampidx, wvltarr_->get( sampidx ) );

    Array1DImpl<float_complex> coutdata( wvltsz_ );
    mDoFFT( true, cindata, coutdata, wvltsz_ );
    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float re = coutdata.get(idx).real();
	float im = coutdata.get(idx).imag();
	float ph = (re*re+im*im) ? atan2( im, re )  : 0;
	phase.set( idx, degree ? 180*ph/M_PI : ph );
    }

    unwrapPhase( wvltsz_, 1, phase.arr() );
}


//TODO put this in algo :
void WaveletAttrib::unwrapPhase( int nrsamples, float w, float* phase )
{
    if ( w == 0 )
	{ pFreeFnErrMsg("wrapping parameter is zero",
			"WaveletAttrib::unwrapPhase"); return; }

    mAllocVarLenArr( float, dphase, nrsamples );
    mAllocVarLenArr( float, temp, nrsamples );

    float pibyw = M_PI/w;

    temp[0] = phase[0];
    dphase[0] = 0;
    for ( int idx=1; idx<nrsamples; idx++ )
    {
	dphase[idx] = fabs( phase[idx] - phase[idx-1] );
	if ( fabs( dphase[idx] - dphase[idx-1] ) >= pibyw )
	    dphase[idx] = dphase[idx-1];

	temp[idx] = temp[idx-1] + dphase[idx];
    }

    for ( int idx=1; idx<nrsamples; idx++ )
	phase[idx] = temp[idx];
}


void WaveletAttrib::muteZeroFrequency( Array1DImpl<float>& vals )
{
    const int arraysz = vals.info().getSize(0);
    Array1DImpl<float_complex> cvals( arraysz ), tmparr( arraysz );

    for ( int idx=0; idx<arraysz; idx++ )
	cvals.set( idx, vals.get(idx) );

    mDoFFT( true, cvals, tmparr, arraysz );
    tmparr.set( 0, 0 );
    mDoFFT( false, tmparr, cvals,  arraysz );

    for ( int idx=0; idx<arraysz; idx++ )
	vals.set( idx, cvals.get( idx ).real() );
}


void WaveletAttrib::getFrequency( Array1DImpl<float>& padfreq, int padfac )
{
    const int zpadsz = padfac*wvltsz_;
    padfreq.setSize( zpadsz );

    Array1DImpl<float_complex> czeropaddedarr( zpadsz ), cfreqarr( zpadsz );

    for ( int idx=0; idx<zpadsz; idx++ )
	czeropaddedarr.set( idx, 0 );

    int padshift = (int)( padfac / 2 )*wvltsz_;
    for ( int idx=0; idx<wvltsz_; idx++ )
	czeropaddedarr.set( padfac>1 ? idx+padshift : idx, wvltarr_->get(idx));

    mDoFFT( true, czeropaddedarr, cfreqarr, zpadsz );

    for ( int idx=0; idx<zpadsz; idx++ )
	padfreq.set( idx, abs( cfreqarr.get(idx) ) );
}


void WaveletAttrib::applyFreqWindow( const ArrayNDWindow& window, int padfac,
				     Array1DImpl<float>& timedata )
{
    int padsz = padfac*wvltsz_;
    Array1DImpl<float_complex> ctimearr( padsz ), cfreqarr( padsz );

    int padshift = (int)( padfac / 2 )*wvltsz_;
    for ( int idx=0; idx<padsz; idx++ )
	ctimearr.set( idx, 0 );

    for ( int idx=0; idx<wvltsz_; idx++ )
	ctimearr.set( idx + padshift, timedata.get(idx) );
    
    mDoFFT( true, ctimearr, cfreqarr, padsz );

    bool isoddpadsz = ( padsz%2 !=0 );

    for ( int idx=0; idx<padsz/2; idx++ )
    {
	float_complex val = cfreqarr.get(idx);
	float_complex revval = cfreqarr.get( padsz-idx-1 );
	cfreqarr.set( idx, window.getValues()[idx]*val );
	cfreqarr.set( padsz-idx-1, window.getValues()[idx]*revval );
    }

    if ( isoddpadsz ) cfreqarr.set( (int)(padsz/2)+1, 0 );

    mDoFFT( false, cfreqarr, ctimearr, padsz );

    for ( int idx=0; idx<wvltsz_; idx++ )
	timedata.set( idx, ctimearr.get( idx + padshift ).real() );
} 
