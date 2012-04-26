/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          October 2009
RCS:           $Id: fftfilter.cc,v 1.14 2012-04-26 14:37:34 cvsbruno Exp $
________________________________________________________________________

*/

#include "arrayndimpl.h"
#include "fftfilter.h"
#include "fourier.h"

#include <complex>



DefineEnumNames(FFTFilter,Type,0,"Filter type")
{ "LowPass", "HighPass", "BandPass", 0 };


FFTFilter::FFTFilter()
    : fft_( Fourier::CC::createDefault() )	    
    , timewindow_(0)	      	      
    , hfreqwindow_(0)	      	      
    , lfreqwindow_(0)	      	      
{
    minfreq_ = maxfreq_ = df_ = mUdf( float );
}

    
FFTFilter::~FFTFilter()
{
    delete fft_;
    delete timewindow_;
    delete hfreqwindow_;
    delete lfreqwindow_;
}


#define mDoFFT(isstraight,inp,outp,sz)\
{\
    fft_->setInputInfo(Array1DInfoImpl(sz));\
    fft_->setDir(isstraight);\
    fft_->setNormalization(!isstraight); \
    fft_->setInput(inp);\
    fft_->setOutput(outp);\
    fft_->run(true);\
}


void FFTFilter::set( float df, float minf, float maxf, Type tp, bool zeropadd )
{
    df_ = df;
    type_ = tp;
    maxfreq_ = maxf;
    minfreq_ = minf;
    iszeropadd_ = zeropadd;
}


void FFTFilter::apply( const float* input, float* output, int sz ) const
{
    mAllocVarLenArr( float_complex, cinput, sz );
    for ( int idx=0; idx<sz; idx++ )
	cinput[idx] = input[idx];

    mAllocVarLenArr( float_complex, coutput, sz );
    apply( cinput, coutput, sz );

    for ( int idx=0; idx<sz; idx++ )
	output[idx] = coutput[idx].real();
}


void FFTFilter::apply(const float_complex* inp,float_complex* outp,int sz) const
{
    mAllocVarLenArr( float_complex, ctimeinput, sz );
    mAllocVarLenArr( float_complex, cfreqinput, sz );
    mAllocVarLenArr( float_complex, cfreqoutput, sz );

    for ( int idx=0; idx<sz; idx++ )
	ctimeinput[idx] = inp[idx];

    if ( timewindow_ )
    {
	for( int idx=0; idx<timewindow_->size_; idx++ )
	    ctimeinput[idx] *= timewindow_->win_[idx];
    }

    mDoFFT( true, ctimeinput, cfreqoutput, sz );

    if ( type_ == BandPass )
	FFTBandPassFilter( df_, minfreq_, maxfreq_, cfreqinput, cfreqoutput,sz);
    else if ( type_ == HighPass )
	FFTFreqFilter( df_,minfreq_,type_==LowPass,cfreqinput,cfreqoutput,sz );

    mDoFFT( false, cfreqoutput, outp, sz );
}


void FFTFilter::FFTFreqFilter( float df, float cutfreq, bool islowpass,
			   const float_complex* input,
			   float_complex* output, int sz ) const
{
    const Window* window = islowpass ? lfreqwindow_ : hfreqwindow_;
    const int bordersz = window ? window->size_ : 0;

    const int poscutfreq = (int)(cutfreq/df);
    int infposthreshold = poscutfreq; 
    int supposthreshold = poscutfreq; 

    if ( !islowpass )
	infposthreshold -= (int)(bordersz/df) - 1;
    else
	supposthreshold += (int)(bordersz/df) - 1;

    float idborder = 0;
    for ( int idx=0 ; idx<sz/2 ; idx++ )
    {
	float_complex outpval = 0;
	float_complex revoutpval = 0;
	const int revidx = sz-idx-1;
        	
	if ( (islowpass && idx < infposthreshold) 
		|| (!islowpass && idx > supposthreshold) )
	{
	    outpval = input[idx];
	    revoutpval = input[revidx];
	}
	else if ( window && idx <=supposthreshold && idx >= infposthreshold )
	{
	    float winval = window->win_[(int)idborder];
	    outpval = input[idx]*winval;
	    revoutpval = input[revidx]*winval;
	    idborder += df;
	}
	output[idx] = outpval;
	output[revidx] = revoutpval;
    }
}


void FFTFilter::FFTBandPassFilter( float df, float minfreq, float maxfreq,
				const float_complex* input,
				float_complex* output, int sz ) const
{
    mAllocVarLenArr( float_complex, intermediateoutput, sz );
    FFTFreqFilter( df, minfreq, false, input, intermediateoutput, sz );
    FFTFreqFilter( df, maxfreq, true, intermediateoutput, output, sz );
}


void FFTFilter::setFreqBorderWindow( float* win, int sz, bool forlowpass )
{
    if ( forlowpass )
    {
	delete hfreqwindow_; hfreqwindow_=new Window(win,sz);
    }
    else
    {
	delete lfreqwindow_; lfreqwindow_=new Window(win,sz);
    }
}
