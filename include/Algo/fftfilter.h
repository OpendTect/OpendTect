#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "arrayndalgo.h"
#include "enums.h"
#include "odcomplex.h"

namespace Fourier { class CC; }

template <class T> class Array1DImpl;


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
			mDeclareEnumUtils(Type)

    void		setLowPass(float cutf3,float cutf4);
    void		setHighPass(float cutf1,float cutf2);
    void		setBandPass(float cutf1,float cutf2,
				    float cutf3,float cutf4);
			// The following will auto taper 5% of the filter size
    void		setLowPass(float cutf4);
    void		setHighPass(float cutf1);
    void		setBandPass(float cutf1,float cutf4);

			//will taper the array before apply
			//do not use for strictly positive/negative signal
    bool		setTimeTaperWindow(int sz, BufferString wintype,
					   float var=0.95);

    bool		apply(Array1DImpl<float>&);
    bool		apply(Array1DImpl<float_complex>&,bool dopreproc=true);

    Array1DImpl<float_complex>* getFreqDomainArr() const
			{ return freqdomain_; }
    void		requestStayInFreqDomain()
			{stayinfreq_ = true; }

    Type		getFilterType() const;
    bool		isLowPass() const;
    bool		isHighPass() const;


protected:

    int			fftsz_;
    int			sz_;
    float		df_;
    float		step_;
    float		cutfreq_[4];

    Fourier::CC*	fft_;
    ArrayNDWindow*	timewindow_;
    ArrayNDWindow*	freqwindow_;
    Array1DImpl<float>*	trendreal_;
    Array1DImpl<float>*	trendimag_;
    BoolTypeSet		isudfreal_;
    BoolTypeSet		isudfimag_;
    Array1DImpl<float_complex>* freqdomain_;
    bool			stayinfreq_;

    void		buildFreqTaperWin();

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
    void		reSize(const Array1DImpl<float>& inp,
			       Array1DImpl<float>& outp) const;
    void		restoreSize(const Array1DImpl<float_complex>& inp,
				    Array1DImpl<float_complex>& outp) const;
    void		restoreSize(const Array1DImpl<float>& inp,
				    Array1DImpl<float>& outp) const;
};
