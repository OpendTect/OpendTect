#ifndef waveletextractor_h
#define waveletextractor_h
/*+
________________________________________________________________________
           
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          July 2009
 RCS:           $Id$ 
 ________________________________________________________________________
                 
-*/   

#include "seismod.h"
#include "executor.h"

namespace Fourier { class CC; }
class IOObj;
namespace Seis { class SelData; }
class SeisTrc;
class SeisTrcReader;
class Wavelet;
template <class T> class Array1DImpl;

mExpClass(Seis) WaveletExtractor : public Executor
{
public:
				WaveletExtractor(const IOObj&,int wvltsize);
				~WaveletExtractor();

    void			setSelData(const Seis::SelData&); // 3D
    void			setSelData(const ObjectSet<Seis::SelData>&);//2D
    void			setPhase(int phase);
    void			setTaperParamVal(float paramval);
    const Wavelet&		getWavelet() const;

protected:

    void			initWavelet(const IOObj&);
    void			init2D();
    void			init3D();
    bool			getSignalInfo(const SeisTrc&,
	    				      int& start,int& signalsz) const;
    bool			getNextLine(); //2D
    bool			processTrace(const SeisTrc&,
	    				     int start, int signalsz);
    void			normalisation(Array1DImpl<float>&);
    bool			finish(int nrusedtrcs);
    bool			doWaveletIFFT();
    bool			rotateWavelet();
    bool			taperWavelet();

    int				nextStep();
    od_int64			totalNr() const	{ return totalnr_ ; }
    od_int64			nrDone() const	{ return nrdone_; }
    const char*			nrDoneText() const;
    const char*			message() const;

    Wavelet&			wvlt_;
    const IOObj&		iobj_;
    const Seis::SelData*	sd_;
    ObjectSet<Seis::SelData>    sdset_;
    SeisTrcReader*		seisrdr_;
    Fourier::CC*		fft_;
    int				lineidx_;
    float			paramval_;
    int				wvltsize_;
    int				phase_;
    int				nrusedtrcs_;
    int				nrdone_;
    bool			isbetweenhor_;
    od_int64			totalnr_;
    BufferString		msg_;
};

#endif

