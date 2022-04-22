#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2015
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

protected:

    const IOObj&	inioobj_;
    IOObj*		outioobj_;
    SeisTrcWriter*	wrr_;
    CBVSSeisTrcTranslator* tr_;

    od_int64		nrdone_;
    mutable od_int64	totnr_;
    uiString		errmsg_;
    const char*		fullusrexp_;

    Interpol		interpol_;

	mStruct(Seis)	PosData
	{
			    PosData()
				: hsit_(0)
				, tkzs_(false) {}

	    BinID		curbid_;
	    TrcKeyZSampling tkzs_;
	    TrcKeySamplingIterator* hsit_;
	};

    PosData		data_, olddata_;

    int			padfac_;
    int			sz_;
    int			newsz_;
    int			szz_;

    Fourier::CC*	fft_;
    ObjectSet<SeisTrc>	trcsset_;
    Array3DImpl<float_complex>* arr_;
    Array3DImpl<float_complex>* fftarr_;
    ArrayNDWindow*	taper_;

    bool		createTranslators(const char*);
    bool		createWriter();

    bool		findSquareTracesAroundCurbid(ObjectSet<SeisTrc>&) const;
    void		getCubeInfo(TypeSet<Coord>&,TypeSet<BinID>&) const;
    float		getInlXlnDist(const Pos::IdxPair2Coord&,bool,int) const;
    SeisTrc*		readTrc(const BinID&) const;
    void		sincInterpol(ObjectSet<SeisTrc>&) const;
};

