#ifndef fftfilter_h
#define fftfilter_h

/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          6-10-2009
RCS:           $Id$
________________________________________________________________________

*/


#include "algomod.h"
#include "arrayndalgo.h"
#include "enums.h"

#include <complex>

namespace Fourier { class CC; }

template <class T> class Array1DImpl;
typedef std::complex<float> float_complex;


/*!
\brief Classical FFT filter, use set to set up data step, min and max
frequency and type of the filter (minfreq not required for highpass, maxfreq
not required for lowpass)
*/ 

mExpClass(Algo) FFTFilter
{

public:
			FFTFilter(int sz, float step);
			~FFTFilter();	   

			enum Type		{ LowPass, HighPass, BandPass };
			DeclareEnumUtils(Type)

    void  		setLowPass(float cutf3,float cutf4);
    void  		setHighPass(float cutf1,float cutf2);
    void  		setBandPass(float cutf1,float cutf2,
	    			    float cutf3,float cutf4);
    			// The following will auto taper 5% of the filter size
    void  		setLowPass(float cutf4);
    void  		setHighPass(float cutf1);
    void  		setBandPass(float cutf1,float cutf4);

			//will taper the array before apply
    			//do not use for strictly positive/negative signal
    bool		setTimeTaperWindow(int sz, BufferString wintype,
	    				   float var=0.95);

    bool		apply(Array1DImpl<float>&);
    bool		apply(Array1DImpl<float_complex>&,bool dopreproc=true);

    Type		getFilterType() const;
    bool		isLowPass() const;
    bool		isHighPass() const;


protected:

    int			fftsz_;
    int			sz_;
    float		df_;
    float		step_;
    float		cutfreq1_;
    float		cutfreq2_;
    float		cutfreq3_;
    float		cutfreq4_;

    Fourier::CC*	fft_; 
    ArrayNDWindow*	timewindow_;
    ArrayNDWindow*	freqwindow_;
    Array1DImpl<float>*	trendreal_;
    Array1DImpl<float>*	trendimag_;
    BoolTypeSet		isudfreal_;
    BoolTypeSet		isudfimag_;

    void  		buildFreqTaperWin();

    			// will store the position of undef points
    bool		interpUdf(Array1DImpl<float>&,bool isimag=false);
    bool		interpUdf(Array1DImpl<float_complex>&);
    void		restoreUdf(Array1DImpl<float>&,bool isimag=false) const;
    void		restoreUdf(Array1DImpl<float_complex>&) const;
    			// will store the removed trend
    bool		deTrend(Array1DImpl<float>& outp,bool isimag=false);
    bool		deTrend(Array1DImpl<float_complex>&);
    bool		restoreTrend(Array1DImpl<float>& outp,
	    			     bool isimag=false) const;
    bool		restoreTrend(Array1DImpl<float_complex>&) const;
    void		reSize(const Array1DImpl<float_complex>& inp,
	    		       Array1DImpl<float_complex>& outp) const;
    void 		reSize(const Array1DImpl<float>& inp,
	    		       Array1DImpl<float>& outp) const;
    void		restoreSize(const Array1DImpl<float_complex>& inp,
	    			    Array1DImpl<float_complex>& outp) const;
    void		restoreSize(const Array1DImpl<float>& inp,
	    			    Array1DImpl<float>& outp) const;
};

#endif


