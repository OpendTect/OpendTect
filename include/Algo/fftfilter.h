#ifndef fftfilter_h
#define fftfilter_h

/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          6-10-2009
RCS:           $Id: fftfilter.h,v 1.7 2010/08/11 16:55:33 cvsyuancheng Exp $
________________________________________________________________________

*/


#include <complex>

namespace Fourier { class CC; }
class ArrayNDWindow;

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
			{ delete timewindow_; timewindow_=new Window(samp,sz); }

//optional cut-off the frequency around a window(nicer output)
    void		setHighFreqBorderWindow( float* samp, int sz )
			{delete hfreqwindow_; hfreqwindow_=new Window(samp,sz);}
    void		setLowFreqBorderWindow( float* samp, int sz )
			{delete lfreqwindow_; lfreqwindow_=new Window(samp,sz);}

protected:

    mStruct Window
    {
			Window(float* win,int sz)
			    : win_(win)
			    , size_(sz)		{}

			int size_;
			float* win_;
    };

    Fourier::CC*	fft_; 
    Window*		timewindow_;
    Window*		hfreqwindow_;
    Window*		lfreqwindow_;

    void		initFilter(const Array1DImpl<float>&,
				   Array1DImpl<float_complex>&);
};

#endif
