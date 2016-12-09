#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "trckeyzsampling.h"
#include "executor.h"
#include "dbkey.h"

namespace PosInfo { class CubeData; }
class RegularSeisDataPack;
class Scaler;
class SeisTrcWriter;
class SeisTrc;


mExpClass(Seis) SeisDataPackWriter : public Executor
{ mODTextTranslationClass(SeisDataPackWriter);
public:
			SeisDataPackWriter(const DBKey&,
			       const RegularSeisDataPack&,
			       const TypeSet<int>& components=TypeSet<int>());
			~SeisDataPackWriter();

    void		setSelection(const TrcKeySampling&,
				     const Interval<int>&);
    const RegularSeisDataPack* dataPack() const	{ return dp_; }
    void		setNextDataPack(const RegularSeisDataPack&);

    void		setComponentScaler(const Scaler&,int compidx);

    od_int64		nrDone() const;
    od_int64		totalNr() const;
    uiString		message() const;
    uiString		nrDoneText() const
			{ return tr("Traces written:"); }
    int			nextStep();

    TrcKeySampling	hSampling() const	{ return tks_; }
    Interval<int>	zSampling() const       { return zrg_; }

private:

    void		setCubeIdxRange();
    bool		setTrc();

    TypeSet<int>		compidxs_;
    ObjectSet<Scaler>		compscalers_; //Same size as compidxs_
    DBKey			mid_;
    ConstRefMan<RegularSeisDataPack>	dp_;

    int				nrdone_;
    int				totalnr_;
    TrcKeySamplingIterator	iterator_;
    const PosInfo::CubeData*	posinfo_;
    SeisTrcWriter*		writer_;
    SeisTrc*			trc_;

    TrcKeySampling		tks_;
    Interval<int>		zrg_;
    StepInterval<int>		cubezrgidx_;
    bool			is2d_;

    void			getPosInfo();
};
