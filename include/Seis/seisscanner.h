#ifndef seisscanner_h
#define seisscanner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Feb 2004
 RCS:		$Id: seisscanner.h 37589 2014-12-17 09:12:16Z bart.degroot@dgbes.com $
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "dataclipper.h"
#include "executor.h"
#include "position.h"
#include "samplingdata.h"
#include "statruncalc.h"
#include "uistring.h"

class IOObj;
class PosGeomDetector;
class SeisTrc;
class SeisTrcReader;
class TrcKeyZSampling;

namespace Pos { class Provider; }
namespace PosInfo { class Detector; }


#define mSeisScanMaxNrDistribVals 50000

mExpClass(Seis) SeisScanner : public Executor
{ mODTextTranslationClass(SeisScanner)
public:

			SeisScanner(const IOObj&,Seis::GeomType,
				    int max_nr_trcs=-1);
			~SeisScanner();

    uiString		uiMessage() const;
    od_int64		totalNr() const;
    od_int64		nrDone() const;
    uiString		uiNrDoneText() const;
    int			nextStep();

    void		report(IOPar&) const;
    bool		getSurvInfo(TrcKeyZSampling&,Coord crd[3]) const;
			//!< Z range will be exclusive start/end null samples

    Interval<float>	zRange() const		{ return Interval<float>
			( sampling_.start, sampling_.atIndex(nrsamples_-1) ); }
    Interval<float>	valRange() const	{ return valrg_; }
    unsigned int	nrNullTraces() const	{ return nrnulltraces_; }

    void		launchBrowser(const IOPar&,const char* fnm) const;
			//!< Passed IOPar will be the start of the display

protected:

    SeisTrc&		trc_;
    SeisTrcReader&	rdr_;
    mutable uiString	curmsg_;
    Seis::GeomType	geom_;
    int			maxnrtrcs_;
    int			totalnr_;
    PosInfo::Detector&	dtctor_;

    SamplingData<float>	sampling_;
    int			nrsamples_;
    int			nrnulltraces_;
    Interval<int>	nonnullsamplerg_;
    Interval<float>	valrg_;
    BinID		invalidsamplebid_;
    int			invalidsamplenr_;
    DataClipSampler	clipsampler_;

    void		wrapUp();
    bool		doValueWork();
    bool		addTrc();

    const char*		getClipRgStr(float) const;
};

mExpClass(Seis) SeisStatsCalc : public Executor
{ mODTextTranslationClass(SeisStatsCalc)
public:
			SeisStatsCalc(const IOObj&,const Stats::CalcSetup&,
				      const Pos::Provider* =0,
				      const TypeSet<int>* components=0);
			~SeisStatsCalc();

    const Stats::RunCalc<float>&	getStats(int) const;

    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const	{ return tr("Traces read"); }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }
    int			nextStep();

protected:

    ObjectSet<Stats::RunCalc<float> >	stats_;

    IOObj*		ioobj_;
    SeisTrcReader&	rdr_;
    TypeSet<int>	components_;
    Pos::Provider*	prov_;

    int			queueid_;

    od_int64		totalnr_;
    od_int64		nrdone_;
    uiString		msg_;
};

#endif
