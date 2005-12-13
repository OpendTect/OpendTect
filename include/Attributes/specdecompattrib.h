#ifndef specdecompattrib_h
#define specdecompattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          Jan 2004
 RCS:           $Id: specdecompattrib.h,v 1.6 2005-12-13 10:03:41 cvshelene Exp $
________________________________________________________________________
-*/

#include "attribprovider.h"
#include "arrayndutils.h"
#include "wavelettrans.h"
#include "fft.h"


/*!\brief Spectral Decomposition Attribute

SpecDecomp gate=[-12,12] window=[Box]|Hamming|Hanning|Barlett|Blackman|CosTaper5

Calculates the frequency spectrum of a trace
Input:
0       Real data
1       Imag data

Output:
0       Spectrum 0 freq
1       freq 1
2       freq 2
|       etc
|       etc
N

*/

class ArrayNDWindow;

namespace Attrib
{

class DataHolder;

class SpecDecomp : public Provider
{
public:
    static void		initClass();
			SpecDecomp( Desc& );

    static const char*	attribName()		{ return "SpecDecomp"; }
    static const char*  transformTypeStr()	{ return "transformtype"; }
    static const char*	windowStr()		{ return "window"; }
    static const char*	gateStr()		{ return "gate"; }
    static const char*	deltafreqStr()		{ return "deltafreq"; }
    static const char*	dwtwaveletStr()		{ return "dwtwavelet"; }
    static const char*	cwtwaveletStr()		{ return "cwtwavelet"; }
    static const char*	transTypeNamesStr(int);

protected:
    			~SpecDecomp();
    static Provider*	createInstance( Desc& );
    static void		updateDesc( Desc& );

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
	    			    int t0,int nrsamples) const;
    bool		calcDFT(const DataHolder&,int t0,int nrsamples) const;
    bool		calcDWT(const DataHolder&,int t0,int nrsamples) const;
    bool		calcCWT(const DataHolder&,int t0,int nrsamples) const;

    const Interval<float>*	reqZMargin(int input, int output) const;

    int				transformtype;
    ArrayNDWindow::WindowType	windowtype;
    Interval<float>		gate;
    float                       deltafreq;
    WaveletTransform::WaveletType	dwtwavelet;
    CWT::WaveletType		cwtwavelet;

    Interval<int>               samplegate;

    ArrayNDWindow*              window;
    FFT                         fft_;
    CWT                         cwt_;
    int                         scalelen;

    float                       df;
    int                         fftsz;
    int                         sz;

    bool			fftisinit;

    int				realidx_;
    int				imagidx_;

    Array1DImpl<float_complex>*     timedomain;
    Array1DImpl<float_complex>*     freqdomain;
    Array1DImpl<float_complex>*     signal;

    const DataHolder*		    redata;
    const DataHolder*               imdata;
};

}; // namespace Attrib


#endif
