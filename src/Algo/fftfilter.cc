/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          October 2009
RCS:           $Id: fftfilter.cc,v 1.2 2009-10-07 11:53:46 cvshelene Exp $
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
    , freqwindowsize_(0)  
    , freqwindow_(0)	      	      
{
    hilbert_ = new HilbertTransform();
    fft_ = new FFT();
} 

    
FFTFilter::~FFTFilter()
{
    delete fft_;
    delete hilbert_;
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

    //avg_ = computeAvg( ctimeinput ).real();
    //removeBias( &ctimeinput );

    mDoTransform( fft_, true, ctimeinput, cfreqoutput, arraysize );
}


void FFTFilter::FFTFreqFilter( float df, float cutfreq, bool islowpass,
			   const Array1DImpl<float_complex>& input,
			   Array1DImpl<float_complex>& output )
{
    const int arraysize = input.info().getTotalSz();
    const int bordersz = freqwindowsize_/2;

    const float infthreshold = cutfreq - bordersz*df/2;
    const float supthreshold = cutfreq + bordersz*df/2;

    Array1DImpl<float> filterborders( bordersz );
    for ( int idx=0; idx<bordersz; idx++ )
    {
	filterborders.set( idx, 1 );
	if ( freqwindow_ )
	    filterborders.set( idx, 
		    freqwindow_[idx+bordersz]*filterborders.get( idx ) );
    }

    Array1DImpl<float> freqarr ( arraysize );
    for ( int idx=0; idx<arraysize; idx++ )
	freqarr.set( idx, idx*df );

    int idarray = 0;
    for ( int idx=0 ; idx<arraysize/2 ; idx++ )
    {
	float_complex outpval = 0;
	float_complex revoutpval = 0;
	const float freq = freqarr.get(idx);
	const int revidx = arraysize-idx-1;
        	
	if ( (islowpass && freq < infthreshold) 
		|| (!islowpass && freq > supthreshold) )
	{
	    outpval = input.get( idx );
	    revoutpval = input.get( revidx );
	}
	else if ( freq < supthreshold && freq > infthreshold )
	{
	    const int idxborder = islowpass ? idarray : bordersz-idarray-1; 
	    outpval = input.get( idx )*filterborders.get( idxborder );
	    revoutpval = input.get( revidx )*
				filterborders.get( bordersz - idxborder -1 );
	    idarray++;
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
