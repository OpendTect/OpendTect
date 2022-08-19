#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "trckeyzsampling.h"
#include "executor.h"
#include "multiid.h"

namespace PosInfo { class CubeData; }
class RegularSeisDataPack;
class Scaler;
class SeisTrcWriter;
class SeisTrc;
namespace VolProc { class ChainOutput; }


mExpClass(Seis) SeisDataPackWriter : public Executor
{ mODTextTranslationClass(SeisDataPackWriter);
public:
			SeisDataPackWriter(const MultiID&,
			       const RegularSeisDataPack&,
			       const TypeSet<int>& components=TypeSet<int>());
			~SeisDataPackWriter();

    void		setSelection(const TrcKeySampling&,
				     const Interval<int>&);
    const RegularSeisDataPack* dataPack() const	{ return dp_; }
    void		setNextDataPack(const RegularSeisDataPack&);

    void		setComponentScaler(const Scaler&,int compidx);

    od_int64		nrDone() const override;
    od_int64		totalNr() const override;
    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override
			{ return tr("Traces written:"); }
    int			nextStep() override;

    TrcKeySampling	hSampling() const	{ return tks_; }
    Interval<int>	zSampling() const       { return zrg_; }

private:

    void			setCubeIdxRange();
    bool			setTrc();
    void			adjustSteeringScaler(int compidx);

    TypeSet<int>		compidxs_;
    MultiID			mid_;
    const RegularSeisDataPack*	dp_;

    int				nrdone_;
    int				totalnr_;
    TrcKeySamplingIterator	iterator_;
    const PosInfo::CubeData*	posinfo_;
    SeisTrcWriter*		writer_;
    SeisTrc*			trc_;

    TrcKeySampling		tks_;
    Interval<int>		zrg_;
    StepInterval<int>		cubezrgidx_;
    ObjectSet<Scaler>		compscalers_;
    bool			is2d_;

    void			getPosInfo();
    void			obtainDP();
    void			releaseDP();

    friend class VolProc::ChainOutput;

};
