#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Feb 2004
________________________________________________________________________

-*/

#include "seismod.h"
#include "position.h"
#include "samplingdata.h"
#include "executor.h"
#include "seistype.h"
#include "dataclipper.h"
#include "uistring.h"
class IOObj;
class SeisTrc;
class TrcKeyZSampling;
class SeisTrcReader;
class PosGeomDetector;
namespace PosInfo { class Detector; }

#define mSeisScanMaxNrDistribVals 50000


mExpClass(Seis) SeisScanner : public Executor
{ mODTextTranslationClass(SeisScanner);
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


