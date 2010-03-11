#ifndef seisbayesclass_h
#define seisbayesclass_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2010
 RCS:		$Id: seisbayesclass.h,v 1.7 2010-03-11 14:12:43 cvsbert Exp $
________________________________________________________________________

*/

#include "executor.h"
#include "bufstringset.h"
#include "enums.h"
class IOPar;
class SeisTrc;
class SeisTrcBuf;
class ProbDenFunc;
class SeisTrcReader;
class SeisTrcWriter;


/*!\brief Bayesian inversion/classification for seismic data using PDFs.

  The IOPar must contain the input/output, in IOObj IDs:
  * mGetSeisBayesPDFIDKey(nr) - the input ProbDenFunc's
  * mGetSeisBayesSeisInpIDKey(nr) - for each dimension of the PDF
				    in the order of the first PDF
  * mGetSeisBayesSeisOutIDKey(nr) - outputs in a specific sequence.

  The outputs are all optional, but have a specific number:
  * 0 .. N-1 = the Probability output for PDF 0 .. N-1
  * N = the Classification
  * N+1 = the Classification confidence

  */

mClass SeisBayesClass : public Executor
{
public:

    enum NormPol		{ None, PerBin, Joint, PerPDF };
    				DeclareEnumUtils(NormPol)

    				SeisBayesClass(const IOPar&);
    				~SeisBayesClass();

    static const char*		sKeyPDFID();
    static const char*		sKeySeisInpID();
    static const char*		sKeySeisOutID();
    static const char*		sKeyNormPol();
    static const char*		sKeyPreScale();

    int				nextStep();
    const char*			message() const;
    const char*			nrDoneText() const;
    od_int64			nrDone() const;
    od_int64			totalNr() const;

protected:

    bool			is2d_;
    ObjectSet<ProbDenFunc>	inppdfs_;
    ObjectSet<ProbDenFunc>	pdfs_;
    ObjectSet<SeisTrcReader>	rdrs_;
    ObjectSet<SeisTrcReader>	aprdrs_;
    ObjectSet<SeisTrcWriter>	wrrs_;
    SeisTrcBuf&			inptrcs_;
    SeisTrcBuf&			aptrcs_;
    SeisTrcBuf&			outtrcs_;
    const IOPar&		pars_;
    ObjectSet< TypeSet<int> >	pdfxtbls_;
    NormPol			normpol_;
    TypeSet<float>		prescales_;

    const int			nrdims_;
    od_int64			nrdone_;
    od_int64			totalnr_;
    BufferString		msg_;
    BufferStringSet		pdfnames_;
    bool			needclass_;
    int				initstep_;

    bool			getPDFs();
    bool			scalePDFs();
    bool			getReaders();
    bool			getWriters();

    int				readInpTrcs();
    int				createOutput();
    int				closeDown();

    void			calcProbs(int);
    void			calcClass();
    void			cleanUp();
    void			prepOutTrc(SeisTrc&,bool) const;
    void			getClass(const TypeSet<float>&,int&,
	    				 float&) const;

};

#define mGetSeisBayesKey(ky,nr) \
    IOPar::compKey(SeisBayesClass::sKey##ky(),nr)
#define mGetSeisBayesIDKey(ky,nr) mGetSeisBayesKey(ky##ID,nr)

#define mGetSeisBayesPreScaleKey(nr) mGetSeisBayesKey(PreScale,nr)
#define mGetSeisBayesPDFIDKey(nr) mGetSeisBayesIDKey(PDF,nr)
#define mGetSeisBayesSeisInpIDKey(nr) mGetSeisBayesIDKey(SeisInp,nr)
#define mGetSeisBayesSeisOutIDKey(nr) mGetSeisBayesIDKey(SeisOut,nr)


#endif
