#ifndef seisscanner_h
#define seisscanner_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Feb 2004
 RCS:		$Id: seisscanner.h,v 1.4 2004-03-02 17:40:37 bert Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "samplingdata.h"
#include "executor.h"
class IOPar;
class IOObj;
class SeisTrc;
class BinIDSampler;
class SeisTrcReader;

#define mMaxNrDistribVals 50000


class SeisScanner : public Executor
{
public:

			SeisScanner(const IOObj&);
			~SeisScanner();

    const char*		message() const;
    int			totalNr() const;
    int			nrDone() const;
    const char*		nrDoneText() const;

    bool		getSurvInfo(BinIDSampler&,StepInterval<double>&,
	    			    Coord crd[3]) const;
    			//!< Z range will be exclusive start/end null samples
    void		report(IOPar&) const;

    const float*	distribVals() const	{ return distribvals; }
    int			nrDistribVals() const	{ return nrdistribvals; }
    Interval<float>	zRange() const
    			{ return Interval<float>( sampling.start,
					 sampling.atIndex(nrsamples-1) ); }
    			//!< the range found in the cube

    static const char*	defaultUserInfoFile(const char* translnm=0);
    void		launchBrowser(const char* fnm=0) const;
    			//!< If nulll or empty fnm uses default file name
    			//!< for the current translator

protected:

    int			nextStep();

    bool		first_trace;
    bool		first_position;
    int			chnksz;
    mutable int		totalnr;
    int			expectedcrls;
    int			nrcrlsthisline;
    SeisTrc&		trc;
    SeisTrcReader&	reader;
    mutable BufferString curmsg;

    SamplingData<float>	sampling;
    unsigned int	nrsamples;
    unsigned int	nrvalidtraces;
    unsigned int	nrnulltraces;
    unsigned int	nrtrcsperposn;
    unsigned int	nrlines;
    unsigned int	nrdistribvals;
    float		distribvals[mMaxNrDistribVals];
    unsigned int	tracesthisposition;
    Interval<int>	nonnullsamplerg;

    BinID		longestinlstart, longestinlstop;
    BinID		curlinestart, curlinestop;
    BinID		mininlbinid, maxinlbinid;
    Coord		longestinlstartcoord, longestinlstopcoord;
    Coord		curlinestartcoord, curlinestopcoord;
    Coord		mininlbinidcoord, maxinlbinidcoord;

    StepInterval<int>	inlrg, crlrg;
    Interval<double>	xrg, yrg;
    Interval<float>	valrg;

    BinID		prevbid;
    BinID		nonstdnrtrcsbid;
    BinID		invalidsamplebid;
    int			invalidsamplenr;
    bool		inlgapsfound, crlgapsfound;
    int			firstinlgapbefore;
    BinID		firstcrlgapbefore;
    bool		varcrlstart, varcrlend;

    void		init();
    void		wrapUp();
    void		handleFirstTrc();
    void		handleTrc();
    void		handleBinIDChange();
    bool		doValueWork();

    const char*		getClipRgStr(float) const;

};


#endif
