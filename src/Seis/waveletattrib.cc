/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Nov 2009
________________________________________________________________________

-*/

#include "waveletattrib.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "fftfilter.h"
#include "fourier.h"
#include "hilberttransform.h"
#include "phase.h"
#include "wavelet.h"
#include "windowfunction.h"


WaveletAttrib::WaveletAttrib( const Wavelet& wvlt )
	: wvltarr_(0)
{
    setNewWavelet( wvlt );
}


WaveletAttrib::~WaveletAttrib()
{ delete wvltarr_; }


void WaveletAttrib::setNewWavelet( const Wavelet& wvlt )
{
    wvltsz_ = wvlt.size();
    delete wvltarr_; wvltarr_ = new Array1DImpl<float>( wvltsz_ );
    OD::memCopy( wvltarr_->getData(), wvlt.samples(), wvltsz_*sizeof(float) );
    centersample_ = wvlt.centerSample();
    sr_ = wvlt.sampleRate();
}


static bool doFFT( bool isfwd, const Array1D<float_complex>& input,
		   Array1D<float_complex>& output, int sz )
{
    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault();
    fft->setInputInfo( Array1DInfoImpl(sz) );
    fft->setDir( isfwd );
    fft->setNormalization( !isfwd );
    fft->setInput( input.getData() );
    fft->setOutput( output.getData() );
    return fft->run( true );
}


void WaveletAttrib::getHilbert( Array1DImpl<float>& hilb ) const
{
    HilbertTransform hilbert;
    hilbert.setCalcRange( 0, 0 );
    hilbert.setInputInfo( Array1DInfoImpl(wvltsz_) );
    hilbert.setDir( true );
    hilbert.init();
    hilbert.transform( *wvltarr_, hilb );
}


void WaveletAttrib::getWaveletArrForPhase( Array1DImpl<float>& cindata ) const
{
    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	int idy = idx;
	idy += idx < wvltsz_ - centersample_ ? centersample_
					     : centersample_ - wvltsz_;
	cindata.set( idx, wvltarr_->get( idy ) );
    }
}


void WaveletAttrib::getPhase( Array1DImpl<float>& phase, bool indegrees ) const
{
    Array1DImpl<float> cindata( wvltsz_ );
    getWaveletArrForPhase( cindata );
    Phase phasecomputer( cindata );
    phasecomputer.setUnitDeg( indegrees );
    if ( !phasecomputer.calculate() )
	return;

    phase = phasecomputer.getPhase();
}


float WaveletAttrib::getAvgPhase( bool indegrees ) const
{
    Array1DImpl<float> cindata( wvltsz_ );
    getWaveletArrForPhase( cindata );
    Phase phasecomputer( cindata );
    phasecomputer.setUnitDeg( indegrees );
    if ( !phasecomputer.calculate() )
	return mUdf(float);

    return phasecomputer.getAvgPhase();
}


void WaveletAttrib::getPhaseRotated( float* out, float phase ) const
{
    if ( mIsUdf(phase) )
	return;

    Array1DImpl<float> hilbert( wvltsz_ );
    getHilbert( hilbert );

    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	out[idx] = wvltarr_->get(idx) * cos( phase ) -
		   hilbert.get(idx) * sin( phase );
    }
}


void WaveletAttrib::getCosTapered( float* out, float taperval ) const
{
    if ( mIsUdf(taperval) )
	return;

    ArrayNDWindow taper( Array1DInfoImpl(wvltsz_), false,
			 CosTaperWindow::sName(), taperval );
    if ( !taper.isOK() )
	return;

    Array1DImpl<float> arrout( wvltsz_ );
    if ( !taper.apply(wvltarr_,&arrout) )
	return;

    OD::memCopy( out, arrout.arr(), wvltsz_*sizeof(float) );
}


bool WaveletAttrib::getFreqFiltered( float* out, float f1, float f2,
				     float f3, float f4 ) const
{
    if ( mIsUdf(f2) && mIsUdf(f3) )
	return false;

    FFTFilter filter( wvltsz_, sr_ );
    if ( mIsUdf(f2) )
    {
	if ( mIsUdf(f4) )
	    filter.setLowPass( f3 );
	else
	    filter.setLowPass( f3, f4 );
    }
    else if ( mIsUdf(f3) )
    {
	if ( mIsUdf(f1) )
	    filter.setHighPass( f2 );
	else
	    filter.setHighPass( f1, f2 );
    }
    else
    {
	if ( mIsUdf(f1) || mIsUdf(f4) )
	    filter.setBandPass( f2, f3 );
	else
	    filter.setBandPass( f1, f2, f3, f4 );
    }

    Array1DImpl<float> arrout( *wvltarr_ );
    if ( !filter.apply(arrout) )
	return false;

    OD::memCopy( out, arrout.arr(), wvltsz_*sizeof(float) );

    return true;
}


void WaveletAttrib::muteZeroFrequency( Array1DImpl<float>& vals )
{
    const int arraysz = vals.info().getSize(0);
    Array1DImpl<float_complex> cvals( arraysz ), tmparr( arraysz );

    for ( int idx=0; idx<arraysz; idx++ )
	cvals.set( idx, vals.get(idx) );

    doFFT( true, cvals, tmparr, arraysz );
    tmparr.set( 0, 0 );
    doFFT( false, tmparr, cvals,  arraysz );

    for ( int idx=0; idx<arraysz; idx++ )
	vals.set( idx, cvals.get( idx ).real() );
}


void WaveletAttrib::transform( Array1D<float_complex>& fftwvlt, int sz )
{
    const int tarsz = mMAX(wvltsz_,sz);

    fftwvlt.setSize( tarsz );
    Array1DImpl<float_complex> wvlt( tarsz );
    wvlt.setAll( 0 );

    const int shift = (tarsz - wvltsz_)/2;
    for ( int idx=0; idx<wvltsz_; idx++ )
        wvlt.set( idx+shift, wvltarr_->get(idx) );

    doFFT( true, wvlt, fftwvlt, tarsz );
}


void WaveletAttrib::transformBack( const Array1D<float_complex>& fftwvlt,
                                   Array1D<float>& wvlt )
{
    const int arraysz = fftwvlt.info().getSize( 0 );
    Array1DImpl<float_complex> cwvlt( arraysz );
    doFFT( false, fftwvlt, cwvlt,  arraysz );

    const int sz2 = arraysz / 2;
    for ( int idx=0; idx<sz2; idx++ )
	wvlt.set( idx+sz2, cwvlt.get(idx).real() );

    for ( int idx=sz2; idx<arraysz; idx++ )
	wvlt.set( idx-sz2, cwvlt.get(idx).real() );
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

    doFFT( true, czeropaddedarr, cfreqarr, zpadsz );

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

    doFFT( true, ctimearr, cfreqarr, padsz );

    bool isoddpadsz = ( padsz%2 !=0 );

    for ( int idx=0; idx<padsz/2; idx++ )
    {
	float_complex val = cfreqarr.get(idx);
	float_complex revval = cfreqarr.get( padsz-idx-1 );
	cfreqarr.set( idx, window.getValues()[idx]*val );
	cfreqarr.set( padsz-idx-1, window.getValues()[idx]*revval );
    }

    if ( isoddpadsz ) cfreqarr.set( (int)(padsz/2)+1, 0 );

    doFFT( false, cfreqarr, ctimearr, padsz );

    for ( int idx=0; idx<wvltsz_; idx++ )
	timedata.set( idx, ctimearr.get( idx + padshift ).real() );
}
