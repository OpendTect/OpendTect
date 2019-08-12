#pragma once
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          July 2009
 ________________________________________________________________________

-*/

#include "seiscommon.h"
#include "executor.h"
#include "wavelet.h"
#include "uistring.h"

namespace Fourier { class CC; }
namespace Seis { class Provider; class SelData; }
class SeisTrc;
class Wavelet;
template <class T> class Array1DImpl;

mExpClass(Seis) WaveletExtractor : public Executor
{ mODTextTranslationClass(WaveletExtractor);
public:
				WaveletExtractor(const IOObj&,int wvltsize);
				~WaveletExtractor();

    void			setSelData(const Seis::SelData&);
    void			setPhase(int phase);
    void			setTaperParamVal(float paramval);
    void			setRelZWindow( Interval<float> zw )
					{ relzwin_ = zw; }
					// when extracting a table of positions

    RefMan<Wavelet>		getWavelet() const;

protected:

    void			initWavelet(const IOObj&);
    bool			getSignalInfo(const SeisTrc&,
					      int& start,int& signalsz) const;
    bool			processTrace(const SeisTrc&,
					     int start, int signalsz);
    void			doNormalisation(Array1DImpl<float>&);
    bool			finish(int nrusedtrcs);
    bool			doWaveletIFFT(float*);
    bool			rotateWavelet(float*);
    bool			taperWavelet(float*);

    int				nextStep();
    od_int64			totalNr() const		    { return totalnr_; }
    od_int64			nrDone() const		    { return nrdone_; }
    uiString			nrDoneText() const;
    uiString			message() const;

    RefMan<Wavelet>		wvlt_;
    const IOObj&		iobj_;
    Interval<float>		relzwin_;
    const Seis::SelData*	sd_;
    Seis::Provider*		seisprov_;
    Fourier::CC*		fft_;
    int				lineidx_;
    float			paramval_;
    int				wvltsize_;
    int				phase_;
    int				nrusedtrcs_;
    int				nrdone_;
    bool			isbetweenhor_;
    od_int64			totalnr_;
    uiString			msg_;

};
