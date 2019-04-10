#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2004
 RCS:		$Id: seisscanner.h 37589 2014-12-17 09:12:16Z bart.degroot@dgbes.com $
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "executor.h"
#include "statruncalc.h"

namespace Pos { class Provider; }
namespace Seis { class Provider; }


mExpClass(Seis) SeisStatsCalc : public Executor
{ mODTextTranslationClass(SeisStatsCalc)
public:
			SeisStatsCalc(const IOObj&,const Stats::CalcSetup&,
				      const Pos::Provider* =0,
				      const TypeSet<int>* components=0);
			~SeisStatsCalc();

    const Stats::RunCalc<float>&	getStats(int) const;

    uiString		message() const	{ return msg_; }
    uiString		nrDoneText() const	{ return tr("Traces read"); }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }
    int			nextStep();

protected:

    ObjectSet<Stats::RunCalc<float> >	stats_;

    IOObj*		ioobj_;
    Seis::Provider*	seisprov_;
    TypeSet<int>	components_;
    Pos::Provider*	posprov_;

    int			queueid_;

    od_int64		totalnr_;
    od_int64		nrdone_;
    uiString		msg_;
};
