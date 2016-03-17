#ifndef seissingtrcproc_h
#define seissingtrcproc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Oct 2001
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "executor.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

class IOObj;
class Scaler;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;
class SeisResampler;


/*!\brief Single trace processing executor

When a trace info is read, the selection notifier is triggered. You can then
use skipCurTrc(). When the trace is read, the processing notifier is triggered.
You can set your own trace as output trace, otherwise the input trace will be
taken.

*/

mExpClass(Seis) SeisSingleTraceProc : public Executor
{ mODTextTranslationClass(SeisSingleTraceProc);
public:

			SeisSingleTraceProc(const IOObj& in,const IOObj& out,
				const char* nm="Trace processor",
				const IOPar* iniopar=0,
				const uiString& msg=uiStrings::sProcessing(),
				int compnr=-1);
			SeisSingleTraceProc(ObjectSet<IOObj>,const IOObj&,
				const char* nm="Trace processor",
				ObjectSet<IOPar>* iniopars=0,
				const uiString& msg=uiStrings::sProcessing(),
				int compnr=-1);
			SeisSingleTraceProc(const IOObj& out,const char* nm,
					    const uiString& msg);
    virtual		~SeisSingleTraceProc();

    bool		addReader(const IOObj&,const IOPar* iop=0);
			//!< Must be done before any step
    void		setInput(const IOObj&,const IOObj&,const char*,
				 const IOPar*,const uiString&);
			//!< Must be done before any step

    void		skipCurTrc()		{ skipcurtrc_ = true; }
			//!< will also be checked after processing CB

    const SeisTrcReader* reader(int idx=0) const
			{ return rdrs_.size()>idx ? rdrs_[idx] : 0; }
    const SeisTrcWriter& writer() const		 { return wrr_; }
    SeisTrc&		getTrace()		 { return *worktrc_; }
    const SeisTrc&	getInputTrace()		 { return intrc_; }

    void		setTracesPerStep( int n ) { trcsperstep_ = n; }
			//!< default is 10

    uiString		uiMessage() const;
    uiString		uiNrDoneText() const;
    virtual od_int64	nrDone() const;
    virtual od_int64	totalNr() const;
    virtual int		nextStep();

    int			nrSkipped() const	{ return nrskipped_; }
    int			nrWritten() const	{ return nrwr_; }
    void		setTotalNrIfUnknown( int nr )
			{ if ( totnr_ < 0 ) totnr_ = nr; }
    void		setScaler(Scaler*);
			//!< Scaler becomes mine.
    void		setResampler(SeisResampler*);
    void		skipNullTraces( bool yn=true )	{ skipnull_ = yn; }
    void		fillNullTraces( bool yn=true )	{ fillnull_ = yn; }

    void		setExtTrcToSI( bool yn )	{ extendtrctosi_ = yn; }
    void		setProcPars(const IOPar&,bool is2d);
			//!< Sets all above proc pars from IOPar

    Notifier<SeisSingleTraceProc> traceselected_;
    Notifier<SeisSingleTraceProc> proctobedone_;

    const Scaler*	scaler() const		{ return scaler_; }

protected:

    ObjectSet<SeisTrcReader> rdrs_;
    SeisTrcWriter&	wrr_;
    SeisTrc&		intrc_;
    SeisTrc*		worktrc_;
    SeisResampler*	resampler_;
    uiString		curmsg_;
    bool		allszsfound_;
    bool		skipcurtrc_;
    int			nrwr_;
    int			nrskipped_;
    int			totnr_;
    MultiID&		wrrkey_;
    int			trcsperstep_;
    int			currdridx_;
    Scaler*		scaler_;
    bool		skipnull_;
    bool		is3d_;
    bool		fillnull_;
    BinID		fillbid_;
    TrcKeySampling	fillhs_;
    SeisTrc*		filltrc_;
    bool		extendtrctosi_;
    int			compnr_;

    bool		nextReader();
    virtual void	wrapUp();

    int			getNextTrc();
    int			getFillTrc();
    bool		prepareTrc();
    bool		writeTrc();
    void		prepareNullFilling();
};


#endif
