#ifndef seissingtrcproc_h
#define seissingtrcproc_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		Oct 2001
 RCS:		$Id: seissingtrcproc.h,v 1.3 2002-02-05 15:44:52 nanne Exp $
________________________________________________________________________

-*/

#include <executor.h>
class SeisTrcReader;
class SeisTrcWriter;
class SeisTrc;
class IOObj;
class IOPar;
class MultiID;


/*!\brief Single trace processing executor

When a trace info is read, the selection callback is triggered. You can then
use skipCurTrc(). When the trace is read, the processing callback is triggered.
You can set you own trace as output trace, otherwise the input trace will be
taken.

*/

class SeisSingleTraceProc : public Executor
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

    void		setSelectionCB( const CallBack& cb ) { selcb_ = cb; }
    void		setProcessingCB( const CallBack& cb ) { proccb_ = cb; }
    void		skipCurTrc()		{ skipcurtrc_ = true; }
    			//!< will also be checked after processing CB

    const SeisTrcReader* reader(int idx=0) const { return rdrset_[idx]; }
    const SeisTrcWriter* writer() const		 { return wrr_; }
    SeisTrc&		inputTrace()		 { return *intrc_; }

    void		setOuputTrace( const SeisTrc& t ) { outtrc_ = &t; }
    			//!< should be called before first nextStep()
    void		setTracesPerStep( int n ) { trcsperstep_ = n; }
    			//!< default is 10

    virtual const char*	message() const;
    virtual const char*	nrDoneText() const;
    virtual int		nrDone() const;
    virtual int		totalNr() const;
    virtual int		nextStep();

    int			nrSkipped() const	{ return nrskipped_; }
    int			nrWritten() const	{ return nrwr_; }

protected:

    ObjectSet<SeisTrcReader> rdrset_;
    SeisTrcWriter*	wrr_;
    SeisTrc*		intrc_;
    const SeisTrc*	outtrc_;
    CallBack		selcb_;
    CallBack		proccb_;
    BufferString	msg_;
    BufferString	curmsg_;
    bool		skipcurtrc_;
    int			nrwr_;
    int			nrskipped_;
    int			totnr_;
    Executor*		starter_;
    MultiID&		wrrkey_;
    int			trcsperstep_;
    int			currentobj_;
    int			nrobjs_;

    virtual void	wrapUp();
    void		nextObj();
    bool		init(ObjectSet<IOObj>&,ObjectSet<IOPar>&);
};


#endif
