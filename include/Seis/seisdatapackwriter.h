#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "trckeysampling.h"
#include "executor.h"
#include "dbkey.h"

namespace PosInfo { class CubeData; }
class RegularSeisDataPack;
class Scaler;
class SeisTrc;
namespace Seis { class SelData; class Storer; }


mExpClass(Seis) SeisDataPackWriter : public Executor
{ mODTextTranslationClass(SeisDataPackWriter);
public:
			SeisDataPackWriter(const DBKey&,
			       const RegularSeisDataPack&,
			       const TypeSet<int>& components=TypeSet<int>());
			~SeisDataPackWriter();

    void		setSelection(const TrcKeySampling&,
				     const Interval<int>* =0);
    const RegularSeisDataPack* dataPack() const	{ return dp_; }
    void		setNextDataPack(const RegularSeisDataPack&);

    void		setComponentScaler(const Scaler&,int compidx);

    od_int64		nrDone() const;
    od_int64		totalNr() const;
    uiString		message() const		{ return msg_; }
    uiString		nrDoneText() const	{ return tr("Traces written"); }
    int			nextStep();

    TrcKeySampling	hSampling() const	{ return tks_; }

private:

    virtual bool	goImpl(od_ostream*,bool,bool,int);
    bool		setTrc();
    void		adjustSteeringScaler(int compidx);

    TypeSet<int>		compidxs_;
    ObjectSet<Scaler>		compscalers_; //Same size as compidxs_
    DBKey			outid_;
    ConstRefMan<RegularSeisDataPack>	dp_;

    int				nrdone_;
    int				totalnr_;
    TrcKeySamplingIterator	iterator_;
    const PosInfo::CubeData*	posinfo_;
    Seis::Storer*		storer_		= nullptr;
    Seis::SelData*		seldata_	= nullptr;
    SeisTrc*			trc_		= nullptr;
    uiString			msg_;

    TrcKeySampling		tks_;
    Interval<int>		cubezrgidx_;
    bool			is2d_;

    void			getPosInfo();
};
