#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2001
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "executor.h"
#include "uistrings.h"

class CubeHorSubSel;
class Scaler;
class SeisTrc;
class SeisResampler;
namespace Seis { class Provider; class Storer; }
namespace Survey { class HorSubSelIterator; }


/*!\brief Single trace processing executor

When a trace info is read, the selection notifier is triggered. You can then
use skipCurTrc(). When the trace is read, the processing notifier is triggered.
You can set your own trace as output trace, otherwise the input trace will be
taken.

*/

mExpClass(Seis) SeisSingleTraceProc : public Executor
{ mODTextTranslationClass(SeisSingleTraceProc);
public:

    mUseType( Seis,	Provider );
    mUseType( Seis,	Storer );

			SeisSingleTraceProc(const IOObj& in,const IOObj& out,
				const char* nm="Trace processor",
				const IOPar* iniopar=0,
				const uiString& msg=uiStrings::sProcessing(),
				int compnr=-1, bool forceFPdata=false);
			SeisSingleTraceProc(ObjectSet<IOObj>,const IOObj&,
				const char* nm="Trace processor",
				ObjectSet<IOPar>* iniopars=0,
				const uiString& msg=uiStrings::sProcessing(),
				int compnr=-1, bool forceFPdata=false);
			SeisSingleTraceProc(const IOObj& out,const char* nm,
					    const uiString& msg);

			// Copiers. Provider(s) and Storers become mine
			SeisSingleTraceProc(Provider*,Storer*);
			SeisSingleTraceProc(const ObjectSet<Provider>&,Storer*);

    virtual		~SeisSingleTraceProc();

    bool		addReader(const IOObj&,const IOPar* iop=0);
			//!< Must be done before any step
    void		setInput(const IOObj&,const IOObj&,const char*,
				 const IOPar*,const uiString&);
			//!< Must be done before any step

    void		skipCurTrc()		{ skipcurtrc_ = true; }
			//!< will also be checked after processing CB

    const Provider*	provider(int idx=0) const
			{ return provs_.validIdx(idx) ? provs_.get(idx) : 0; }
    const Storer&	storer() const		 { return storer_; }
    SeisTrc&		getTrace()		 { return *worktrc_; }
    const SeisTrc&	getInputTrace()		 { return intrc_; }

    void		setTracesPerStep( int n ) { trcsperstep_ = n; }
			//!< default is 10

    uiString		message() const;
    uiString		nrDoneText() const;
    virtual od_int64	nrDone() const;
    virtual od_int64	totalNr() const;
    virtual int		nextStep();

    int			nrSkipped() const	{ return nrskipped_; }
    int			nrWritten() const	{ return nrwr_; }
    void		setScaler(Scaler*);
			//!< Scaler becomes mine.
    void		setResampler(SeisResampler*);
    void		skipNullTraces( bool yn=true )	{ skipnull_ = yn; }
    void		fillNullTraces( bool yn=true )	{ fillnull_ = yn; }

    void		setExtTrcToSI( bool yn )	{ extendtrctosi_ = yn; }
    void		setProcPars(const IOPar&);
			//!< Sets all above proc pars from IOPar

    Notifier<SeisSingleTraceProc> traceselected_;
    Notifier<SeisSingleTraceProc> proctobedone_;

    const Scaler*	scaler() const		{ return scaler_; }

protected:

    ObjectSet<Provider>	provs_;
    Storer&		storer_;
    SeisTrc&		intrc_;
    SeisTrc*		worktrc_;
    SeisResampler*	resampler_;
    uiString		curmsg_;
    bool		allszsfound_;
    bool		skipcurtrc_;
    int			nrwr_;
    int			nrskipped_;
    od_int64		totnr_;
    DBKey&		wrrkey_;
    int			trcsperstep_;
    int			curprovidx_;
    Scaler*		scaler_;
    bool		skipnull_;
    bool		is3d_;
    bool		fillnull_;
    CubeHorSubSel*	fillchss_;
    Survey::HorSubSelIterator* filliter_;
    SeisTrc*		filltrc_;
    bool		extendtrctosi_;
    int			compnr_;
    bool		forcefpdata_=false;

    void		addProv(Provider*,bool,const IOPar* iop=0);
    bool		nextReader();
    virtual int		wrapUp();

    int			getNextTrc();
    int			getFillTrc();
    bool		prepareTrc();
    bool		writeTrc();
    bool		prepareNullFilling();

};
