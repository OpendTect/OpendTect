#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "executor.h"
#include "uistring.h"

namespace Fourier { class CC; }
class IOObj;
namespace Seis { class SelData; }
class SeisTrc;
class SeisTrcReader;
class Wavelet;
template <class T> class Array1DImpl;

mExpClass(Seis) WaveletExtractor : public Executor
{ mODTextTranslationClass(WaveletExtractor);
public:
				WaveletExtractor(const IOObj&,int wvltsize);
				~WaveletExtractor();

    void			setSelData(const Seis::SelData&); // 3D
    void			setSelData(const ObjectSet<Seis::SelData>&);//2D
    void			setPhase(double phase);
				// phase expected in radians
    void			setTaperParamVal(float paramval);
    const Wavelet&		getWavelet() const;

protected:

    void			initWavelet(const IOObj&);
    void			init2D();
    void			init3D();
    bool			getSignalInfo(const SeisTrc&,
					      int& start,int& signalsz) const;
    bool			getSignalInfoBetweenZ(const SeisTrc&,
					      int& start,int& signalsz) const;
    bool			getSignalInfoFull(const SeisTrc&,
					      int& start,int& signalsz) const;
    bool			getNextLine(); //2D
    bool			processTrace(const SeisTrc&,
					     int start, int signalsz);
    mDeprecated("Use normalization")
    void			normalisation( Array1DImpl<float>& arr )
				{ normalization( arr ); }
    void			normalization(Array1DImpl<float>&);
    bool			finish(int nrusedtrcs);
    bool			doWaveletIFFT();
    bool			rotateWavelet();
    bool			taperWavelet();

    static void			normalization(Array1D<float>&,int wvltsz);
    static bool			doWaveletIFFT(Fourier::CC&,
					      Wavelet&,int wvltsz);
    static bool			rotateWavelet(Wavelet&,int wvltsz,double phase);
    static bool			taperWavelet(Wavelet&,int wvltsz,float taper);

    int				nextStep() override;
    od_int64			totalNr() const override { return totalnr_ ; }
    od_int64			nrDone() const override  { return nrdone_; }
    uiString			uiNrDoneText() const override;
    uiString			uiMessage() const override;

    Wavelet&			wvlt_;
    const IOObj&		iobj_;
    const Seis::SelData*	sd_		= nullptr;
    ObjectSet<Seis::SelData>    sdset_;
    SeisTrcReader*		seisrdr_	= nullptr;
    Fourier::CC*		fft_;
    int				lineidx_	= -1;
    float			paramval_	= mUdf(float);
    int				wvltsize_;
    double			phase_		= mUdf(double);
    int				nrusedtrcs_;
    int				nrdone_;
    bool			isbetweenhor_;
    od_int64			totalnr_;
    uiString			msg_;

public:

    static uiRetVal		processTrace(Array1D<float>&,Fourier::CC&,
					     int wvltsz,float taperval,
					     Wavelet&);
    static bool			finalize(Fourier::CC&,Wavelet&,
					     int wvltsz,double phase,
					     float taperval);
};
