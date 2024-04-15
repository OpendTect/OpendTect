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
    int			nriter_				= 10;
    float		maxvel_				= 0.f;
    uiString		errmsg_;

    int			nrdone_				= 0;
    mutable int		totnr_				= 0;

    Fourier::CC*	fft_				= nullptr;
    int			szx_				= 0;
    int			szy_				= 0;
    int			szz_				= 0;
    float		max_				= 0.f;

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

    Array3DImpl<float_complex>*		trcarr_		= nullptr;
    TrcKeySampling			hs_;

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


    IOObj*		inioobj_			= nullptr;
    IOObj*		outioobj_			= nullptr;
    TrcKeyZSampling	tkzs_;

    BinID		curbid_;
    BinID		prevbid_;
    int			nriter_;

    float		maxvel_				= mUdf(float);
    bool		reusetrcs_			= false;
    int			inlstep_			= 0;
    int			crlstep_			= 0;

    uiString		errmsg_;

    SeisScaler*		sc_				= nullptr;

    SeisTrcBuf&		seisbuf_;
    TrcKeySampling	seisbuftks_;

    SeisTrcWriter*	wrr_				= nullptr;
    SeisTrcBuf		tmpseisbuf_;

    SeisInterpol	interpol_;
    TrcKeySamplingIterator hsit_;

    bool		read_				= false;
    int			nrdone_				= 0;
    mutable int		totnr_				= 0;
    bool		nearesttrace_			= true;

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

    float		avgmaxval_			= 0.f;
    float		avgminval_			= 0.f;
};
