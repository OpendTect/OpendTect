#ifndef seissingtrcproc_h
#define seissingtrcproc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Oct 2001
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "executor.h"
#include "cubesampling.h"
class IOObj;
class IOPar;
class Scaler;
class SeisTrc;
class MultiID;
class SeisTrcReader;
class SeisTrcWriter;
class SeisResampler;


/*!\brief Single trace processing executor

When a trace info is read, the selection notifier is triggered. You can then
use skipCurTrc(). When the trace is read, the processing notifier is triggered.
You can set your own trace as output trace, otherwise the input trace will be
taken.

*/

mClass(Seis) SeisSingleTraceProc : public Executor
{
public:

			SeisSingleTraceProc(const IOObj* in,const IOObj* out,
					    const char* nm="Trace processor",
					    const IOPar* iniopar=0,
					    const char* msg="Processing");
			SeisSingleTraceProc(ObjectSet<IOObj>,const IOObj*,
					    const char* nm="Trace processor",
					    ObjectSet<IOPar>* iniopars=0,
					    const char* msg="Processing");
    virtual		~SeisSingleTraceProc();

    void		skipCurTrc()		{ skipcurtrc_ = true; }
    			//!< will also be checked after processing CB

    const SeisTrcReader* reader(int idx=0) const { return rdrset_[idx]; }
    const SeisTrcWriter* writer() const		 { return wrr_; }
    SeisTrc&		getTrace()		 { return *worktrc_; }
    const SeisTrc&	getInputTrace()		 { return intrc_; }

    void		setTracesPerStep( int n ) { trcsperstep_ = n; }
    			//!< default is 10

    virtual const char*	message() const;
    virtual const char*	nrDoneText() const;
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

    void		setInput(const IOObj*,const IOObj*,const char*,
				 const IOPar*,const char*);
    void		setExtTrcToSI( bool yn )	{ extendtrctosi_ = yn; }

    Notifier<SeisSingleTraceProc>		traceselected_;
    Notifier<SeisSingleTraceProc>		proctobedone_;

protected:

    ObjectSet<SeisTrcReader> rdrset_;
    SeisTrcWriter*	wrr_;
    SeisTrc&		intrc_;
    SeisTrc*		worktrc_;
    SeisResampler*	resampler_;
    BufferString	msg_;
    BufferString	curmsg_;
    bool		skipcurtrc_;
    int			nrwr_;
    int			nrskipped_;
    int			totnr_;
    MultiID&		wrrkey_;
    int			trcsperstep_;
    int			currentobj_;
    int			nrobjs_;
    Scaler*		scaler_;
    bool		skipnull_;
    bool		is3d_;
    bool		fillnull_;
    BinID		fillbid_;
    HorSampling		fillhs_;
    SeisTrc*		filltrc_;
    bool		extendtrctosi_;

    bool		mkWriter(const IOObj*);
    void		nextObj();
    bool		init(ObjectSet<IOObj>&,ObjectSet<IOPar>&);
    virtual void	wrapUp();

    int			getNextTrc();
    int			getFillTrc();
    bool		prepareTrc();
    bool		writeTrc();
    void		prepareNullFilling();
};


#endif

