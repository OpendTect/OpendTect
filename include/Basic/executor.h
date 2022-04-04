#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		11-7-1996
________________________________________________________________________

-*/

#include "basicmod.h"
#include "task.h"
#include "namedobj.h"
#include "progressmeter.h"
#include "od_iosfwd.h"

/*!
\brief Specification to enable chunkwise execution of a process.

  Interface enabling separation of the control of execution of any process from
  what actually is going on. The work is done by calling the doStep() method
  until either ErrorOccurred or Finished is returned. To enable logging and/or
  communication with the user, two types of info can be made available (the
  methods will be called before the step is executed). Firstly, a message.
  Secondly, info on the progress.
  It is common that Executors are combined to a new Executor object. This is
  the most common reason why totalNr() can change.

  If doStep returns -1 (Failure) the error message should be in uiMessage().

  The execute() utility executes the process while logging uiMessage() etc. to
  a stream. Useful in batch situations.
*/

mExpClass(Basic) Executor : public SequentialTask
{
public:
			Executor( const char* nm )
			    : SequentialTask(nm)
			    , prestep(this), poststep(this)	{}
    virtual		~Executor()			{}

    int			doStep() override;

    inline bool		go( od_ostream* s=0, bool isfirst=true,
			    bool islast=true, int delaybtwnstepsinms=0 )
			{ return goImpl(s,isfirst,islast,delaybtwnstepsinms); }
    inline bool		go( od_ostream& s, bool isfirst=true,
			    bool islast=true, int delaybtwnstepsinms=0 )
			{ return goImpl(&s,isfirst,islast,delaybtwnstepsinms); }

    Notifier<Executor>	prestep;
    Notifier<Executor>	poststep; //!< Only when MoreToDo will be returned.

			// Being a Task requires:
    bool		execute() override	{ return go(); }

protected:

    virtual bool	goImpl(od_ostream*,bool,bool,int);

};


/*!
\brief Executor consisting of other executors.

  Executors may be added on the fly while processing. Depending on the
  parallel flag, the executors are executed in the order in which they were
  added or in parallel (but still single-threaded).
*/

mExpClass(Basic) ExecutorGroup : public Executor
{
public:
			ExecutorGroup( const char* nm, bool parallel=false,
				       bool ownsexecs=true );
    virtual		~ExecutorGroup();
    virtual void	add( Executor* );
			/*!< You will become mine if ownsexecs_ is true!! */

    uiString		uiMessage() const override;
    od_int64		totalNr() const override;
    od_int64		nrDone() const override;
    uiString		uiNrDoneText() const override;

    int			nrExecutors() { return executors_.size(); }
    Executor*		getExecutor(int idx) { return executors_[idx]; }

    void		setNrDoneText(const uiString& txt)
			{ nrdonetext_ = txt; }
			//!< If set, will use this and the counted nrdone

protected:

    int			nextStep() override;
    virtual bool	goToNextExecutor();
    void		findNextSumStop();

    int			sumstart_;
    int			sumstop_;
    const bool		parallel_;
    int			currentexec_;
    uiString		nrdonetext_;
    ObjectSet<Executor>& executors_;
    TypeSet<int>	executorres_;
    bool		ownsexecs_;

};


/*!
\brief TaskRunner to show progress of a Task in text format.
*/

mExpClass(Basic) TextTaskRunner : public TaskRunner
{
public:
			TextTaskRunner( od_ostream& strm )
			    : TaskRunner()
			    , strm_(strm)	{}

    bool		execute(Task&) override;

protected:

    od_ostream&		strm_;

};


