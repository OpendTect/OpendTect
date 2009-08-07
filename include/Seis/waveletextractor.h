/*+
________________________________________________________________________
           
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          July 2009
 RCS:           $Id: waveletextractor.h,v 1.1 2009-08-07 12:34:55 cvsnageswara Exp $ 
 ________________________________________________________________________
                 
-*/   

#include "executor.h"

class FFT;
class IOObj;
namespace Seis { class SelData; class TableSelData; }
class SeisTrc;
class SeisTrcReader;
class BufferString;
template <class T> class Array1DImpl;

mClass WaveletExtract : public Executor
{
public:
				WaveletExtract(const IOObj*,
					       const Seis::SelData*,
					       const int wvltsize);
				~WaveletExtract();
    void			setOutputPhase(const int phase);
    const float*		getWavelet() const;

protected:

    bool			isBetween2Hors(const Seis::TableSelData&);

    bool			setTraces(const SeisTrc&,
	    				  const Seis::TableSelData*) ;
    bool			doStatistics(int,const SeisTrc&);
    void			normalisation(Array1DImpl<float>&);
    bool			finish(int);
    bool			doIFFT(const float*, float*);
    bool			calcWvltPhase(const float*, float*);
    int				nextStep();
    od_int64			totalNr() const;
    od_int64			nrDone() const  { return nrdone_; };
    const char*			nrDoneText() const;
    const char*			message() const;

    BufferString		msg_;
    const Seis::SelData*	sd_;
    SeisTrcReader*		seisrdr_;
    FFT*			fft_;
    Array1DImpl<float>* 	stackedwvlt_;
    int				wvltsize_;
    int				phase_;
    int				nrgoodtrcs_;
    int				nrdone_;
    int				start_;
    int				stop_;
    bool			isdouble_;
};
