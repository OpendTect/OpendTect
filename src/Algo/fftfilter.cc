/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          October 2009
RCS:           $Id: fftfilter.cc,v 1.3 2009-10-08 14:26:38 cvsbruno Exp $
________________________________________________________________________

*/

#include "arrayndimpl.h"
#include "fftfilter.h"
#include "fft.h"
#include "hilberttransform.h"

#include <complex>

FFTFilter::FFTFilter()
    : avg_(0)
    , hilbert_(0)	       	
    , fft_(0)	      
    , window_(0)	      	      
    , fwindow_(0)	      	      
{
    hilbert_ = new HilbertTransform();
    fft_ = new FFT();
} 

    
FFTFilter::~FFTFilter()
{
    delete fft_;
    delete hilbert_;
    delete window_;
    delete fwindow_;
}


#define mDoTransform(tf,isstraight,inp,outp,sz)\
{\
    tf->setInputInfo(Array1DInfoImpl(sz));\
    tf->setDir(isstraight);\
    tf->init();\
    tf->transform(inp,outp);\
}
void FFTFilter::FFTFreqFilter( float df, float cutfreq, bool islowpass,
				  const Array1DImpl<float>& input,
				  Array1DImpl<float>& output )
{
    const int arraysize = input.info().getTotalSz();
    Array1DImpl<float_complex> cfreqinput( arraysize ), 
			       cfreqoutput( arraysize ), 
			       coutvals( arraysize );
    initFilter( input, cfreqinput );

    FFTFreqFilter( df, cutfreq, islowpass, cfreqinput, cfreqoutput );
    mDoTransform( fft_, false, cfreqoutput, coutvals, arraysize );

    float correctbias = islowpass ? avg_ : 0;
    for ( int idx=0; idx<arraysize; idx++ )
	output.set( idx, coutvals.get(idx).real() + correctbias );
}


void FFTFilter::FFTBandPassFilter( float df, float minfreq, float maxfreq,
				      const Array1DImpl<float>& input,
				      Array1DImpl<float>& output )
{
    const int arraysize = input.info().getTotalSz();
    Array1DImpl<float_complex> cfreqinput( arraysize ), 
			       cfreqoutput( arraysize ), 
			       coutvals( arraysize );
    initFilter( input, cfreqinput );
    
    float correctbias = minfreq ? 0 : avg_ ;
    FFTBandPassFilter( df, minfreq, maxfreq, cfreqinput, cfreqoutput );
    mDoTransform( fft_, false, cfreqoutput, coutvals, arraysize );

    for ( int idx=0; idx<arraysize; idx++ )
	output.set( idx, coutvals.get(idx).real() + correctbias );
}


void FFTFilter::initFilter( const Array1DImpl<float>& timeinput,
			       Array1DImpl<float_complex>& cfreqoutput )
{
    const int arraysize = timeinput.info().getTotalSz();
    Array1DImpl<float_complex> ctimeinput( arraysize );

    hilbert_->setCalcRange( 0, arraysize, 0 );
    mDoTransform( hilbert_, true, timeinput, ctimeinput, arraysize );

    if ( window_ )
    {
	for( int idx=0; idx<window_->size_; idx++ )
	    ctimeinput.set( idx, window_->win_[idx]*ctimeinput.get( idx )  );
    }

    avg_ = 0;
    for ( int idx=0; idx<arraysize; idx++ )
	avg_ += ctimeinput.get(idx).real();
    avg_ /= arraysize;
    for ( int idx=0; idx<arraysize; idx++ )
	ctimeinput.set( idx, ctimeinput.get( idx) - avg_ );

    mDoTransform( fft_, true, ctimeinput, cfreqoutput, arraysize );
}


void FFTFilter::FFTFreqFilter( float df, float cutfreq, bool islowpass,
			   const Array1DImpl<float_complex>& input,
			   Array1DImpl<float_complex>& output )
{
    const int arraysize = input.info().getTotalSz();
    const int bordersz = fwindow_ ? fwindow_->size_ : 0;

    const int poscutfreq = (int)(cutfreq/df);
    const int infposthreshold = poscutfreq - bordersz/2;
    const int supposthreshold = poscutfreq + bordersz/2;

    int idborder = 0;
    for ( int idx=0 ; idx<arraysize/2 ; idx++ )
    {
	float_complex outpval = 0;
	float_complex revoutpval = 0;
	const int revidx = arraysize-idx-1;
        	
	if ( (islowpass && idx < infposthreshold) 
		|| (!islowpass && idx > supposthreshold) )
	{
	    outpval = input.get( idx );
	    revoutpval = input.get( revidx );
	}
	else if ( idx < supposthreshold && idx > infposthreshold )
	{
	    float winval = fwindow_->win_[idborder];
	    float revwinval = fwindow_->win_[bordersz-idborder-1];
	    if ( !islowpass )
	    { float tmp; mSWAP( winval, revwinval, tmp ); }
 
	    outpval = input.get( idx )*winval;
	    revoutpval = input.get( revidx )*revwinval;
	    idborder++;
	}
	output.set( idx,outpval );
	output.set( arraysize-idx-1, revoutpval );
    }
}


void FFTFilter::FFTBandPassFilter( float df, float minfreq, float maxfreq,
				const Array1DImpl<float_complex>& input,
				Array1DImpl<float_complex>& output )
{
    Array1DImpl<float_complex> intermediateoutput( input.info().getTotalSz() );
    FFTFreqFilter( df, minfreq, false, input, intermediateoutput );
    FFTFreqFilter( df, maxfreq, true, intermediateoutput, output );
}
