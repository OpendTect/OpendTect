/*+
________________________________________________________________________
           
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          July 2009
 RCS:           $Id: waveletextractor.h,v 1.2 2009-09-23 05:56:05 cvsnageswara Exp $ 
 ________________________________________________________________________
                 
-*/   

#include "executor.h"
#include<complex>

class FFT;
class IOObj;
namespace Seis { class SelData; class TableSelData; }
class SeisTrc;
class SeisTrcReader;
class BufferString;
template <class T> class Array1DImpl;
typedef std::complex<float> float_complex;

mClass WaveletExtractor : public Executor
{
public:
				WaveletExtractor(const IOObj&,
						 const Seis::SelData&,
						 int wvltsize );
				~WaveletExtractor();
    void			setOutputPhase(int phase);
    const float*		getWavelet() const;
    void			setParamVal(float);

protected:

    bool			isBetween2Hors();

    bool			setTraces(const SeisTrc&);
    bool			doStatistics(int,const SeisTrc&);
    void			normalisation(Array1DImpl<float>&);
    bool			finish(int);
    bool			doIFFT(const float*,float*);
    bool			calcWvltPhase(const float*,float*);
    bool			taperedWvlt(const float*,float*);
    bool			wvltIFFT(const Array1DImpl<float_complex>&,
	    				 float*);
    int				nextStep();
    od_int64			totalNr() const;
    od_int64			nrDone() const  { return nrdone_; };
    const char*			nrDoneText() const;
    const char*			message() const;

    BufferString		msg_;
    const Seis::SelData&	sd_;
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
    od_int64			totalnr_;
    float			paramval_;
};

