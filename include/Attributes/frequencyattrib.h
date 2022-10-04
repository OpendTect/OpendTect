#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"
#include "bufstringset.h"
#include "fourier.h"
#include "mathfunc.h"
#include "valseries.h"
#include "valseriesinterpol.h"

#include <complex>


class ArrayNDWindow;
template<class T> class Array1DImpl;

namespace Attrib
{

class DataHolder;

/*!
\brief %Frequency Attribute
  
  Calculates a number of attributes (see below) from the frequency domain
  in a gate. The gate can be windowed with the window specified in
  the window parameter prior to the Fourier Transform. If normalize is enabled,
  the frequency spectra is normalized with regard to its area. This will make
  it possible to attribare the attribs from areas with high and low energy.

  Dumptofile dumps the spectrogram at all locations to a file. This feature is
  only for experimental dGB use - don't present it in any manuals. If used wrong
  (i.e. with volume output) you will end up with a file of several Gb. The
  file is stored as /tmp/frequency.dump

<pre>
  %Frequency gate=[-4,4] [normalize=No] [window=CosTaper5] [dumptofile=No]

  Input:
  0       Real data
  1       Imag data

  %Output:
  0       Dominant frequency (DFQ)
  1       Average frequency  (AFQ)
  2       Median frequency (MFQ)
  3       Average frequency Squared (AFS)
  4       Maximum spectral amplitude (MSA)
  5       Spectral Area beyond dominant frequency (SADF)
  6       %Frequency Slope Fall (FSF)
  7       Absorption Quality Factor (AQF)
</pre>
*/

mExpClass(Attributes) Frequency : public Provider
{
public:
    static void			initClass();
				Frequency(Desc&);

    static const char*		attribName()		{ return "Frequency"; }
    static const char*		gateStr()		{ return "gate"; }
    static const char*		normalizeStr()		{ return "normalize"; }
    static const char*		windowStr()		{ return "window"; }
    static const char*		paramvalStr()		{ return "paramval"; }
    static const char*		dumptofileStr()		{ return "dumptofile"; }
    static const char*		smoothspectrumStr()	{ return "smoothspect";}

    void			prepPriorToBoundsCalc() override;

protected:
				~Frequency();
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);
    static void			updateDefaults(Desc&);

    bool			checkInpAndParsAtStart() override;
    bool			allowParallelComputation() const override
				{ return false; }
    bool			getInputOutput(int input,
					   TypeSet<int>& res) const override;
    bool			getInputData(const BinID&,int idx) override;
    bool			computeData(const DataHolder&,const BinID& rel,
					    int z0,int nrsamples,
					    int threadid) const override;

    const Interval<float>*	reqZMargin(int input,int output) const override;

    Interval<float>		gate_;
    Interval<int>		samplegate_;
    bool			dumptofile_;
    ArrayNDWindow*		window_;
    BufferString		windowtype_;
    float			df_;

    bool			normalize_;
    float			variable_;

    const DataHolder*		redata_;
    const DataHolder*		imdata_;
    int				realidx_;
    int				imagidx_;
    bool			smoothspectrum_;

    BufferStringSet		dumpset_;
    bool			fftisinit_;
    int				fftsz_;
    Fourier::CC*		fft_;

    Array1DImpl<float_complex>* signal_;
    Array1DImpl<float_complex>* timedomain_;
    Array1DImpl<float_complex>* freqdomain_;

    mExpClass(Attributes) FreqFunc : public FloatMathFunction
    {
    public:
			FreqFunc(const ValueSeries<float>& func, int sz)
			    : func_( func )
			    , sz_(sz)			{}
			~FreqFunc()			{}

    float		getValue( float x ) const override
			{
				ValueSeriesInterpolator<float> interp(sz_);
				return interp.value(func_,x);
			}
    virtual float	getValue( const float* p ) const
			{ return getValue(*p); }

    protected:

	const ValueSeries<float>& func_;
	int sz_;

    };
};

} // namespace Attrib
