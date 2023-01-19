#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "executor.h"
#include "trckeyzsampling.h"
#include "arrayndimpl.h"
#include "fourier.h"
#include "binid.h"
#include "seisbuf.h"
#include "uistring.h"

class IOObj;
class Seis2DDataSet;
class SeisScaler;
class SeisTrc;
class SeisTrcWriter;
class SeisTrcBuf;


mExpClass(Seis) SeisInterpol : public Executor
{ mODTextTranslationClass(SeisInterpol)
public:

			SeisInterpol();
			~SeisInterpol();

    void		setInput(const ObjectSet<const SeisTrc>&);
    void		setParams(const TrcKeySampling&,float maxvel);

    void		getOutTrcs(ObjectSet<SeisTrc>&,
					const TrcKeySampling&) const;
    uiString		uiMessage() const override
			{ return  errmsg_.isEmpty() ? tr( "interpolating" )
						    : errmsg_; }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiNrDoneText() const override
						{
					return tr("Number of iterations");
						}
    od_int64		totalNr() const override	{ return nriter_; }
    int			nextStep() override;

protected:

    const ObjectSet<const SeisTrc>* inptrcs_;
    int			nriter_;
    float		maxvel_;
    uiString		errmsg_;

    int			nrdone_;
    mutable int		totnr_;

    Fourier::CC*	fft_;
    int			szx_;
    int			szy_;
    int			szz_;
    float		max_;

    mStruct(Seis) TrcPosTrl
    {
		    TrcPosTrl(int x,int y, int trc)
			: idx_(x)
			, idy_(y)
			, trcpos_(trc)
			{}
	int		idx_;
	int		idy_;
	int		trcpos_;

	bool operator	== ( const TrcPosTrl& tr ) const
			    { return tr.trcpos_ == trcpos_; }
    };
    TypeSet<TrcPosTrl>			posidxs_;

    Array3DImpl<float_complex>*		trcarr_;
    TrcKeySampling				hs_;

    void		clear();
    void		doWork(bool,int);
    bool		doPrepare(od_ostream* =nullptr) override;
    void		setUpData();
    void		setFinalTrcs();

    const BinID		convertToBID(int,int) const;
    void		convertToPos(const BinID&,int&,int&) const;
    int			getTrcInSet(const BinID&) const;
};



mExpClass(Seis) Seis2DTo3D : public Executor
{ mODTextTranslationClass(Seis2DTo3D)
public:

			Seis2DTo3D();
			~Seis2DTo3D();


    uiString		uiMessage() const override
			{ return errmsg_.isEmpty() ? tr("interpolating")
						   : errmsg_; }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiNrDoneText() const override	{ return tr("Done"); }
    od_int64		totalNr() const override;
    int			nextStep() override;

    bool		init(const IOPar&);
    bool		useNearestOnly() const	{ return nearesttrace_; }

    bool		finishWrite()		{ return writeTmpTrcs(); }

    static const char*	sKeyInput();
    static const char*	sKeyIsNearest();
    static const char*	sKeyStepout();
    static const char*	sKeyReUse();
    static const char*	sKeyMaxVel();
    static const char*	sKeyCreaterType();
    static BufferString getCreatorFormat() { return "Normal"; }

protected:
    bool		usePar(const IOPar&);
    bool		setIO(const IOPar&);
    bool		checkParameters();
    void		doWorkNearest();
    bool		doWorkFFT();


    IOObj*		inioobj_;
    IOObj*		outioobj_;
    TrcKeyZSampling	tkzs_;

    BinID		curbid_;
    BinID		prevbid_;
    int			nriter_;

    float		maxvel_;
    bool		reusetrcs_;
    int			inlstep_;
    int			crlstep_;

    uiString		errmsg_;

    SeisScaler*		sc_;

    SeisTrcBuf&		seisbuf_;
    TrcKeySampling	seisbuftks_;

    SeisTrcWriter*	wrr_;
    SeisTrcBuf		tmpseisbuf_;

    SeisInterpol	interpol_;
    TrcKeySamplingIterator hsit_;

    bool		read_;
    int			nrdone_;
    mutable int		totnr_;
    bool		nearesttrace_;

    bool		writeTmpTrcs();
    bool		read();
};


mExpClass(Seis) SeisScaler
{ mODTextTranslationClass(SeisScaler);
public:
			SeisScaler();
			SeisScaler(const SeisTrcBuf&);
			~SeisScaler();

    void		scaleTrace(SeisTrc&);

protected:

    float		avgmaxval_ = 0.f;
    float		avgminval_ = 0.f;
};
