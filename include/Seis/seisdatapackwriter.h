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

namespace PosInfo { class LineCollData; }
class RegularSeisDataPack;
class ArrRegSubSelIterator;
class Scaler;
class SeisTrc;
namespace Seis { class SelData; class Storer; }
namespace Survey { class HorSubSel; class HorSubSelIterator; }


mExpClass(Seis) SeisDataPackWriter : public Executor
{ mODTextTranslationClass(SeisDataPackWriter);
public:

    mUseType( PosInfo,	LineCollData );
    mUseType( Survey,	HorSubSel );
    mUseType( Survey,	HorSubSelIterator );

			SeisDataPackWriter(const DBKey&,
			       const RegularSeisDataPack&,
			       const TypeSet<int>& components=TypeSet<int>());
			~SeisDataPackWriter();

    void		setSelection(const HorSubSel&,
				     const Interval<int>* =0);
    const RegularSeisDataPack& dataPack() const	{ return *dp_; }
    void		setNextDataPack(const RegularSeisDataPack&);

    void		setComponentScaler(const Scaler&,int compidx);

    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }
    uiString		message() const		{ return msg_; }
    uiString		nrDoneText() const	{ return tr("Traces written"); }
    int			nextStep();

    HorSubSel&		horSubSel() const	{ return *hss_; }

private:

    virtual bool	goImpl(od_ostream*,bool,bool,int);
    bool		setTrc();
    void		adjustSteeringScaler(int compidx);

    TypeSet<int>		compidxs_;
    ObjectSet<Scaler>		compscalers_; //Same size as compidxs_
    DBKey			outid_;
    ConstRefMan<RegularSeisDataPack>	dp_;

    od_int64			nrdone_;
    od_int64			totalnr_;
    HorSubSel*			hss_;
    HorSubSelIterator*		iterator_	= nullptr;
    const LineCollData*		lcd_		= nullptr;
    Seis::Storer*		storer_		= nullptr;
    Seis::SelData*		seldata_	= nullptr;
    SeisTrc*			trc_		= nullptr;
    uiString			msg_;
    Interval<int>		cubezrgidx_;

    void			getPosInfo();

};
