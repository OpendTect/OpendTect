#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "executor.h"
#include "seisstor.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

class Scaler;
class SeisCubeCopier;
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
				const IOPar* iniopar=nullptr,
				const uiString& msg=uiStrings::sProcessing(),
				int compnr=-1);
			SeisSingleTraceProc(const SeisStoreAccess::Setup& inpsu,
					    const SeisStoreAccess::Setup& outsu,
					    const char* nm="Trace processor",
				const uiString& msg=uiStrings::sProcessing());

    virtual		~SeisSingleTraceProc();

    bool		isOK() const	    { return errmsg_.isEmpty(); }
    uiString		errMsg() const	    { return errmsg_; }

    void		skipCurTrc()		{ skipcurtrc_ = true; }
			//!< will also be checked after processing CB

    const SeisTrcReader* reader(int idx=0) const
			{ return rdrs_.size()>idx ? rdrs_[idx] : nullptr; }
			//!< Only available during execution
    const SeisTrcWriter* writer() const		 { return wrr_; }
			//!< Only available during execution
    SeisTrc&		getTrace()		 { return *worktrc_; }
			//!< Only available during execution
    const SeisTrc&	getInputTrace()		 { return intrc_; }
			//!< Only available during execution

    void		setTracesPerStep( int n ) { trcsperstep_ = n; }
			//!< default is 10

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;
    od_int64		nrDone() const override;
    od_int64		totalNr() const override;

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

    SeisStoreAccess::Setup inpsetup_;
    SeisStoreAccess::Setup outsetup_;
    ObjectSet<SeisTrcReader> rdrs_;
    SeisTrcWriter*	wrr_ = nullptr;
    SeisTrc&		intrc_;
    SeisTrc*		worktrc_;
    SeisResampler*	resampler_ = nullptr;
    BufferString	execnm_;
    uiString		initmsg_;
    uiString		curmsg_;
    uiString		errmsg_;
    bool		allszsfound_ = true;
    bool		skipcurtrc_;
    int			nrwr_ = 0;
    int			nrskipped_ = 0;
    int			totnr_ = -1;
    MultiID&		wrrkey_;
    int			trcsperstep_ = 10;
    int			currdridx_ = -1;
    Scaler*		scaler_ = nullptr;
    bool		skipnull_ = false;
    bool		fillnull_ = false;
    BinID		fillbid_;
    TrcKeySampling	fillhs_;
    SeisTrc*		filltrc_ = nullptr;
    bool		extendtrctosi_ = false;

    bool		goImpl(od_ostream*,bool,bool,int) override;
    int			nextStep() override;

    bool		setInput();
    bool		addReader();
    bool		nextReader();
    virtual void	wrapUp();

    int			getNextTrc();
    int			getFillTrc();
    bool		prepareTrc();
    bool		writeTrc();
    void		prepareNullFilling();

    bool		is3D() const;
    bool		is2D() const;
    bool		isPS() const;

    friend class SeisCubeCopier;

public:

    mDeprecated("Use a single IOObj")
			SeisSingleTraceProc(ObjectSet<IOObj>,const IOObj&,
				const char* nm="Trace processor",
				ObjectSet<IOPar>* iniopars=nullptr,
				const uiString& msg=uiStrings::sProcessing(),
				int compnr=-1);
    mDeprecated("Provide input IOObj")
			SeisSingleTraceProc(const IOObj& out,const char* nm,
					    const uiString& msg);

    mDeprecated("Use setup object in the constructor")
    bool		setInput(const IOObj&,const IOObj&,const char*,
				 const IOPar*,const uiString&);
			//!< Must be done before any step

    mDeprecated("IOPar not used")
    bool		addReader(const IOObj&,const IOPar* iop);
			//!< Must be done before any step
};
