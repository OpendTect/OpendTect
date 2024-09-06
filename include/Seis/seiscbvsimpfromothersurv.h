#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "arrayndimpl.h"
#include "executor.h"
#include "fourier.h"
#include "position.h"
#include "trckeyzsampling.h"
#include "uistring.h"

class ArrayNDWindow;
class CBVSSeisTrcTranslator;
class IOObj;
class SeisTrc;
class SeisTrcWriter;
class SeisTrcInfo;
namespace Pos { class IdxPair2Coord; }

mExpClass(Seis) SeisImpCBVSFromOtherSurvey : public Executor
{ mODTextTranslationClass(SeisImpCBVSFromOtherSurvey);
public:

    enum Interpol	{ Sinc, Nearest };

			SeisImpCBVSFromOtherSurvey(const IOObj&);
			~SeisImpCBVSFromOtherSurvey();

    uiString		uiMessage() const override
			{ return tr("Importing CBVS"); }

    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiNrDoneText() const override
			{ return tr("Traces handled"); }

    od_int64		totalNr() const override	{ return totnr_; }
    int			nextStep() override;

    uiString		errMsg() const		{ return errmsg_; }
    bool		prepareRead(const char*);
    void		setPars(Interpol&,int,const TrcKeyZSampling&);
    inline void		setOutput( IOObj& obj )	{ outioobj_ = &obj; }

    const TrcKeyZSampling& cubeSampling() const { return data_.tkzs_; }
    bool		hasSameGridAsThisSurvey() const;

protected:

    IOObj*		inioobj_;
    IOObj*		outioobj_			= nullptr;
    SeisTrcWriter*	wrr_				= nullptr;
    CBVSSeisTrcTranslator* tr_				= nullptr;

    od_int64		nrdone_				= 0;
    mutable od_int64	totnr_				= 0;
    uiString		errmsg_;
    const char*		fullusrexp_			= nullptr;

    Interpol		interpol_;

	mStruct(Seis) PosData
	{
					PosData();
					~PosData();

	    BinID			curbid_;
	    TrcKeyZSampling		tkzs_;
	    TrcKeySamplingIterator* 	hsit_;
	};

    PosData		data_;
    PosData		olddata_;

    int			padfac_;
    int			sz_;
    int			newsz_;
    int			szz_;
    int			nrcomponents_;

    Fourier::CC*	fft_				= nullptr;
    ObjectSet<SeisTrc>	trcsset_;
    Array3DImpl<float_complex>* arr_			= nullptr;
    Array3DImpl<float_complex>* fftarr_			= nullptr;
    ArrayNDWindow*	taper_				= nullptr;

    bool		doFinish(bool success,od_ostream* =nullptr) override;

    bool		createTranslators(const char*);
    bool		createWriter();

    bool		findSquareTracesAroundCurbid(ObjectSet<SeisTrc>&) const;
    void		getCubeInfo(TypeSet<Coord>&,TypeSet<BinID>&) const;
    float		getInlXlnDist(const Pos::IdxPair2Coord&,bool,int) const;
    SeisTrc*		readTrc(const BinID&) const;
    void		sincInterpol(ObjectSet<SeisTrc>&) const;
};
