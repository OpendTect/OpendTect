#ifndef fftfilter_h
#define fftfilter_h

/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          6-10-2009
RCS:           $Id: fftfilter.h,v 1.2 2009-10-08 14:26:38 cvsbruno Exp $
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

//optional taper in time-domain array before computation 
//only if non complex numbers as input
    void		setTaperWindow( float* samp, int sz )
			{ delete window_; window_ = new Window(samp,sz/2); }

//optional cut-off the frequency around a window(nicer output)
    void		setFreqBorderWindow( float* samp, int sz )
			{ delete fwindow_; fwindow_ = new Window(samp,sz/2); }

protected:

    mStruct Window
    {
			Window(float* win,int sz)
			    : win_(win)
			    , size_(sz)		{}

			int size_;
			float* win_;
    };

    float 		avg_;
    HilbertTransform*	hilbert_; 
    FFT*		fft_; 
    Window*		window_;
    Window*		fwindow_;

    void		initFilter(const Array1DImpl<float>&,
				   Array1DImpl<float_complex>&);
};

#endif
