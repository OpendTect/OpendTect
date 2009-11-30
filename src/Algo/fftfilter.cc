/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          October 2009
RCS:           $Id: fftfilter.cc,v 1.9 2009-11-30 15:28:26 cvsbruno Exp $
________________________________________________________________________

*/

#include "arrayndimpl.h"
#include "fftfilter.h"
#include "fft.h"
#include "hilberttransform.h"

#include <complex>

FFTFilter::FFTFilter()
    : hilbert_(0)	       	
    , fft_(0)	      
    , window_(0)	      	      
    , hfwindow_(0)	      	      
    , lfwindow_(0)	      	      
{
    hilbert_ = new HilbertTransform();
    fft_ = new FFT();
} 

    
FFTFilter::~FFTFilter()
{
    delete fft_;
    delete hilbert_;
    delete window_;
    delete hfwindow_;
    delete lfwindow_;
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
    const int arraysize = input.info().getSize(0);
    Array1DImpl<float_complex> cfreqinput( arraysize ), 
			       cfreqoutput( arraysize ), 
			       coutvals( arraysize );
    initFilter( input, cfreqinput );

    FFTFreqFilter( df, cutfreq, islowpass, cfreqinput, cfreqoutput );
    mDoTransform( fft_, false, cfreqoutput, coutvals, arraysize );

    for ( int idx=0; idx<arraysize; idx++ )
    {
	float val = coutvals.get(idx).real();
	if ( mIsUdf(val) )
	    val = 0;
	output.set( idx, val );
    }
}


void FFTFilter::FFTBandPassFilter( float df, float minfreq, float maxfreq,
				      const Array1DImpl<float>& input,
				      Array1DImpl<float>& output )
{
    const int arraysize = input.info().getSize(0);
    Array1DImpl<float_complex> cfreqinput( arraysize ), 
			       cfreqoutput( arraysize ), 
			       coutvals( arraysize );
    initFilter( input, cfreqinput );
    
    FFTBandPassFilter( df, minfreq, maxfreq, cfreqinput, cfreqoutput );
    mDoTransform( fft_, false, cfreqoutput, coutvals, arraysize );

    for ( int idx=0; idx<arraysize; idx++ )
    {
	float val = coutvals.get(idx).real();
	if ( mIsUdf(val) )
	    val = 0;
	output.set( idx, val );
    }
}


void FFTFilter::initFilter( const Array1DImpl<float>& timeinput,
			       Array1DImpl<float_complex>& cfreqoutput )
{
    const int arraysize = timeinput.info().getSize(0);
    Array1DImpl<float_complex> ctimeinput( arraysize );
    for ( int idx=0; idx<arraysize; idx++ )
	ctimeinput.set( idx, timeinput.get( idx ) );

    if ( window_ )
    {
	for( int idx=0; idx<window_->size_; idx++ )
	    ctimeinput.set( idx, window_->win_[idx]*ctimeinput.get( idx )  );
    }

    for ( int idx=0; idx<arraysize; idx++ )
	ctimeinput.set( idx, ctimeinput.get( idx) );

    mDoTransform( fft_, true, ctimeinput, cfreqoutput, arraysize );
}


void FFTFilter::FFTFreqFilter( float df, float cutfreq, bool islowpass,
			   const Array1DImpl<float_complex>& input,
			   Array1DImpl<float_complex>& output )
{
    const int arraysize = input.info().getSize(0);
    const Window* window = islowpass ? lfwindow_ : hfwindow_;
    const int bordersz = window ? window->size_ : 0;

    const int poscutfreq = (int)(cutfreq/df);
    int infposthreshold = poscutfreq; 
    int supposthreshold = poscutfreq; 

    if ( !islowpass )
	infposthreshold -= (int)(bordersz/df) - 1;
    else
	supposthreshold += (int)(bordersz/df) - 1;

    float idborder = 0;
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
	else if ( window && idx <=supposthreshold && idx >= infposthreshold )
	{
	    float winval = window->win_[(int)idborder];
	    outpval = input.get( idx )*winval;
	    revoutpval = input.get( revidx )*winval;
	    idborder += df;
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


