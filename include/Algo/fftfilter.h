#ifndef fftfilter_h
#define fftfilter_h

/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          6-10-2009
RCS:           $Id: fftfilter.h,v 1.1 2009-10-07 09:53:13 cvsbruno Exp $
________________________________________________________________________

*/


#include <complex>

class FFT;
class ArrayNDWindow;
class HilbertTransform;

template <class T> class Array1DImpl;
typedef std::complex<float> float_complex;

mClass FFTFilter
{

public:
			FFTFilter();
			~FFTFilter();	   

    void  		FFTFreqFilter(float,float,bool,
	    			const Array1DImpl<float>&,Array1DImpl<float>&);

    void 		FFTFreqFilter(float,float,bool,
				const Array1DImpl<float_complex>&,
				Array1DImpl<float_complex>&);

    void   		FFTBandPassFilter(float,float,float,
				const Array1DImpl<float>&,Array1DImpl<float>&); 

    void   		FFTBandPassFilter(float,float,float,
				const Array1DImpl<float_complex>&,
				Array1DImpl<float_complex>&);

    void		setFreqBorderWindow( float* win, int sz )
			{ freqwindow_ = win; freqwindowsize_=sz; }

protected:

    float 		avg_;
    HilbertTransform*	hilbert_; 
    FFT*		fft_; 
    float*		freqwindow_;
    int			freqwindowsize_;

    
    void		initFilter(const Array1DImpl<float>&,
				   Array1DImpl<float_complex>&);
};

#endif
