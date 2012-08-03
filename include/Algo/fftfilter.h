#ifndef fftfilter_h
#define fftfilter_h

/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          6-10-2009
RCS:           $Id: fftfilter.h,v 1.11 2012-08-03 13:00:03 cvskris Exp $
________________________________________________________________________

*/


#include "algomod.h"
#include "algomod.h"
#include <complex>
#include "enums.h"

namespace Fourier { class CC; }
class ArrayNDWindow;

template <class T> class Array1DImpl;
typedef std::complex<float> float_complex;

/*! brief classical FFT filter, use set to set up data step, min and max frequency and type of the filter (minfreq not required for highpass, maxfreq not required for lowpass) !*/ 

mClass(Algo) FFTFilter
{

public:
			FFTFilter();
			~FFTFilter();	   

			enum Type		{ LowPass, HighPass, BandPass };
			DeclareEnumUtils(Type)

    void  		setLowPass(float df,float cutf,bool zeropad); 
    void  		setHighPass(float df,float cutf,bool zeropad); 
    void  		setBandPass(float df,float cutf1,float cutf2,bool pad); 
    void  		set(float df,float cutf1,float cutf2,Type,bool zeropad); 
    void		apply(const float*,float*,int sz) const;
    void		apply(const float_complex*,float_complex*,int sz) const;

    void		apply(const Array1DImpl<float>&,
	    			 Array1DImpl<float>&) const;
    void		apply(const Array1DImpl<float_complex>&,
	    			 Array1DImpl<float_complex>&) const;

			//will taper the array before apply
    void		setTaperWindow( float* samp, int sz )
			{ delete timewindow_; timewindow_=new Window(samp,sz); }

			//optional cut-off the frequency with a window
    void		setFreqBorderWindow(float* win,int sz,bool forlowpass);

    int			getFFTFastSize(int nrsamps) const;

protected:

    mStruct(Algo) Window
    {
			Window(float* win,int sz)
			    : win_(win)
			    , size_(sz)		{}

			int size_;
			float* win_;
    };

    float		df_;
    Type		type_;
    float		cutfreq1_;
    float		cutfreq2_;
    bool		iszeropadd_;

    Fourier::CC*	fft_; 
    Window*		timewindow_;
    Window*		hfreqwindow_;
    Window*		lfreqwindow_;

    void 		FFTFreqFilter(float,float,bool,
			    const float_complex*,float_complex*,int sz) const;
    void   		FFTBandPassFilter(float,float,float,
			    const float_complex*,float_complex*,int sz) const;
};

#endif


