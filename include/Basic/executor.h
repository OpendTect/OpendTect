#ifndef executor_h
#define executor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		11-7-1996
 RCS:		$Id$
________________________________________________________________________

-*/

#include "task.h"
#include "namedobj.h"
#include "progressmeter.h"

#include <iosfwd>

template <class T> class ObjectSet;

/*!\brief specification to enable chunkwise execution a process.

Interface enabling separation of the control of execution of any process from
what actually is going on. The work is done by calling the doStep() method
until either ErrorOccurred or Finished is returned. To enable logging and/or
communication with the user, two types of info can be made available (the
methods will be called before the step is executed). Firstly, a message.
Secondly, info on the progress.
It is common that Executors are combined to a new Executor object. This is
the most common reason why totalNr() can change.

If doStep returns -1 (Failure) the error message should be in message().

The execute() utility executes the process while logging message() etc. to
a stream. Useful in batch situations.

*/

mClass Executor : public SequentialTask
{
public:
			Executor( const char* nm )
			: SequentialTask(nm)
			, prestep(this), poststep(this)	{}
    virtual		~Executor()			{}

    virtual int		doStep();

    virtual bool	execute(std::ostream*,bool isfirst=true,
	    			bool islast=true,int delaybetwnstepsinms=0);
    virtual bool	execute()	{ return execute(0); }

    Notifier<Executor>	prestep;
    Notifier<Executor>	poststep; //!< Only when MoreToDo will be returned.

};


/*!\brief Executor consisting of other executors.

Executors may be added on the fly while processing. Depending on the
parallel flag, the executors are executed in the order in which they were added
or in parallel (but still single-threaded).

*/


mClass ExecutorGroup : public Executor
{
public:
    			ExecutorGroup( const char* nm, bool parallel=false,
				       bool ownsexecs=true );
    virtual		~ExecutorGroup();
    virtual void	add( Executor* );
    			/*!< You will become mine if ownsexecs_ is true!! */

    virtual const char*	message() const;
    virtual od_int64	totalNr() const;
    virtual od_int64	nrDone() const;
    virtual const char*	nrDoneText() const;
    
    int			nrExecutors() { return executors_.size(); }
    Executor*		getExecutor(int idx) { return executors_[idx]; }

    void		setNrDoneText(const char* txt) { nrdonetext_ = txt; }
    			//!< If set, will use this and the counted nrdone

protected:

    virtual int		nextStep();
    virtual bool	goToNextExecutor();
    void		findNextSumStop();

    int			sumstart_;
    int			sumstop_;
    const bool		parallel_;
    int			currentexec_;
    BufferString	nrdonetext_;
    ObjectSet<Executor>& executors_;
    TypeSet<int>	executorres_;
    bool		ownsexecs_;

};


mClass TextTaskRunner : public TaskRunner
{
public:
			TextTaskRunner( std::ostream& strm )
			    : TaskRunner()
			    , strm_(strm)	{}

    bool		execute( Task& t )
			{
			    mDynamicCastGet(Executor*,exec,&t)
			    if ( exec )
				execres_ = exec->execute( &strm_ );
			    else
			    {
				TextStreamProgressMeter progressmeter(strm_);
				t.setProgressMeter( &progressmeter );
				execres_ = t.execute();
			    }
			    
			    return execres_;
			}

protected:
    std::ostream&	strm_;
};


#endif
