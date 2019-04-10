#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2015
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "arrayndimpl.h"
#include "executor.h"
#include "fourier.h"
#include "position.h"
#include "trckeyzsampling.h"
#include "uistring.h"

class ArrayNDWindow;
class CBVSSeisTrcTranslator;
class SeisTrc;
class SeisTrcInfo;
namespace Pos { class IdxPair2Coord; }
namespace Seis { class Storer; namespace Blocks { class Reader; } }


mExpClass(Seis) SeisCubeImpFromOtherSurvey : public Executor
{ mODTextTranslationClass(SeisCubeImpFromOtherSurvey);
public:

    enum Interpol	{ Sinc, Nearest };

			SeisCubeImpFromOtherSurvey(const IOObj&);
			~SeisCubeImpFromOtherSurvey();

    uiString		message() const	{ return tr("Importing CBVS"); }
    od_int64		nrDone() const          { return nrdone_; }
    uiString		nrDoneText() const	{ return tr("Traces handled"); }
    od_int64		totalNr() const		{ return totnr_; }
    int			nextStep();

    bool		prepareRead(const char*);
    void		setPars(Interpol&,int,const TrcKeyZSampling&);
    void		setOutput(const IOObj&);

    const TrcKeyZSampling& cubeSampling() const { return data_.tkzs_; }

protected:

    const IOObj&	inioobj_;
    IOObj*		outioobj_;
    Seis::Storer*	storer_;
    CBVSSeisTrcTranslator* cbvstr_;
    Seis::Blocks::Reader* rdr_;

    od_int64		nrdone_			= 0;
    mutable od_int64	totnr_			= -1;
    uiRetVal		uirv_;
    const char*		fullusrexp_;

    Interpol		interpol_;

    mStruct(Seis)	PosData
    {
			PosData()
			    : hsit_(0)
			    , tkzs_(false) {}

	BinID			curbid_;
	TrcKeyZSampling		tkzs_;
	TrcKeySamplingIterator*	hsit_;
    };

    PosData		data_, olddata_;

    int			padfac_;
    int			sz_;
    int			newsz_;
    int			szz_;

    typedef Array3DImpl<float_complex>	CplxArr3D;
    Fourier::CC*	fft_;
    ObjectSet<SeisTrc>	trcsset_;
    CplxArr3D*		arr_;
    CplxArr3D*		fftarr_;
    ArrayNDWindow*	taper_;

    bool		createReader(const char*);
    bool		createWriter();

    bool		findSquareTracesAroundCurbid(ObjectSet<SeisTrc>&) const;
    void		getCubeInfo(TypeSet<Coord>&,TypeSet<BinID>&) const;
    float		getInlXlnDist(const Pos::IdxPair2Coord&,bool,int) const;
    SeisTrc*		readTrc(const BinID&) const;
    void		sincInterpol(ObjectSet<SeisTrc>&) const;
};
