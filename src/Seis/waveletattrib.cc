/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Nov 2009
 RCS:           $Id: waveletattrib.cc,v 1.6 2009-11-25 13:33:06 cvsbruno Exp $
________________________________________________________________________

-*/

#include "waveletattrib.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "fft.h"
#include "hilberttransform.h"
#include "wavelet.h"


WaveletAttrib::WaveletAttrib( const Wavelet& wvlt )
	: wvltarr_(0)
	, wvltsz_(0)  
	, fft_(new FFT(false))
	, hilbert_(new HilbertTransform())
{
    setNewWavelet( wvlt );
}


WaveletAttrib::~WaveletAttrib()
{
    delete wvltarr_;
    delete fft_;
    delete hilbert_;
}


void WaveletAttrib::setNewWavelet( const Wavelet& wvlt )
{
    wvltsz_ = wvlt.size();
    delete wvltarr_; wvltarr_ = new Array1DImpl<float>( wvltsz_ );
    memcpy( wvltarr_->getData(), wvlt.samples(), wvltsz_*sizeof(float) );
}


#define mDoTransform( transfm, isforward, inp, outp,sz )\
{\
    transfm->setInputInfo( Array1DInfoImpl(sz) );\
    transfm->setDir( isforward );\
    transfm->init();\
    transfm->transform(inp,outp);\
}


void WaveletAttrib::getHilbert(Array1DImpl<float>& hilb )
{
    hilbert_->setCalcRange( 0, wvltsz_, 0 );
    mDoTransform( hilbert_, true, *wvltarr_, hilb, wvltsz_ );
} 


void WaveletAttrib::getPhase( Array1DImpl<float>& phase, bool degree )
{
    Array1DImpl<float> hilb( wvltsz_ );
    getHilbert( hilb );
    for ( int idx=0; idx<wvltsz_; idx++ )
    {
	float ph = 0;
	if ( !mIsZero(wvltarr_->get(idx),mDefEps)  )
	    ph = atan2( hilb.get(idx), wvltarr_->get(idx) );
	phase.set( idx, degree ? 180*ph/M_PI : ph );
    }
}


void WaveletAttrib::muteZeroFrequency( Array1DImpl<float>& vals )
{
    const int arraysz = vals.info().getSize(0);
    Array1DImpl<float_complex> cvals( arraysz ), tmparr( arraysz );

    for ( int idx=0; idx<arraysz; idx++ )
	cvals.set( idx, vals.get(idx) );

    mDoTransform( fft_, true, cvals, tmparr, arraysz );
    tmparr.set( 0, 0 );
    mDoTransform( fft_, false, tmparr, cvals,  arraysz );

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

    for ( int idx=0; idx<wvltsz_; idx++ )
	czeropaddedarr.set( padfac>1 ? idx+wvltsz_:idx, wvltarr_->get(idx) );

    mDoTransform( fft_, true, czeropaddedarr, cfreqarr, zpadsz );

    for ( int idx=0; idx<zpadsz; idx++ )
	padfreq.set( idx, abs( cfreqarr.get(idx) ) );
}


void WaveletAttrib::getWvltFromFrequency( const Array1DImpl<float>& freqdata,
       					  Array1DImpl<float>& timedata )
{
    const int timesz = timedata.info().getSize(0);
    const int freqsz = freqdata.info().getSize(0);

    Array1DImpl<float_complex> cfreqarr(freqsz), ctimearr(freqsz);

    for ( int idx=0; idx<freqsz; idx++ )
	cfreqarr.set( idx, freqdata.get(idx) );

    mDoTransform( fft_, false, cfreqarr, ctimearr, timesz );

    for ( int idx=0; idx<timesz/2; idx++ )
	timedata.set( idx+timesz/2, ctimearr.get(timesz-idx-1).real() );

    for ( int idx=0; idx<timesz/2+1; idx++ )
	timedata.set( timesz/2-idx+1, ctimearr.get(idx).real() );
    timedata.set( 0, 0 );
} 
