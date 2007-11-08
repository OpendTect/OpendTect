#ifndef task_h
#define task_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril/K.Tingdahl
 Date:		13-10-1999
 RCS:		$Id: task.h,v 1.3 2007-11-08 17:07:13 cvsyuancheng Exp $
________________________________________________________________________

-*/

namespace Threads { class ThreadWorkManager; class Mutex; class ConditionVar; }

/*!The generalization of something (e.g. a computation) that needs to be
   done in multiple steps. */


class Task
{
public:
    
    virtual		~Task() 			{}

    virtual void	enableNrDoneCounting(bool=true)	{}
    			//!<Must be called before execute()
    virtual int		nrDone() const			{ return -1; }
    virtual int		totalNr() const			{ return -1; }
    virtual const char*	message() const			{ return "Working"; }
    virtual const char*	nrDoneText() const		{ return "Nr Done"; }

    virtual bool	execute()			= 0;

    enum Control	{ Run, Pause, Stop };
    virtual void	controlWork(Control);
    virtual Control	getState() const;
    virtual void	enableWorkContol(bool=true);
    			//!<Must be called before execute()


protected:
    virtual bool			shouldContinue();
    					//!<\returns wether we should continue
    					Task();
    Threads::ConditionVar*		workcontrolcondvar_;
    Task::Control			control_;
};


/*!The generalization of something (e.g. a computation) where the steps must
   be done in sequence, i.e. not parallely. */


class SequentialTask : public Task
{
public:
    
    virtual	~SequentialTask() 	{}
    virtual int	doStep()		{ return nextStep(); }
    		/*!<\retval cMoreToDo()		Not finished. Call me again.
		    \retval cFinished()		Nothing more to do.
		    \retval cErrorOccurred()	Something went wrong.
		    \note if function returns a value greater than cMoreToDo(),
			  it should be interpreted as cMoreToDo(). */

    static int	cMoreToDo()		{ return 1; }
    static int	cFinished()		{ return 0; }
    static int	cErrorOccurred()	{ return -1; }

    bool	execute();

protected:

    virtual int	nextStep()		= 0;
    		/*!<\retval cMoreToDo()		Not finished. Call me again.
		    \retval cFinished()		Nothing more to do.
		    \retval cErrorOccurred()	Something went wrong.
		    \note if function returns a value greater than cMoreToDo(),
			  it should be interpreted as cMoreToDo(). */
};

class ParallelTaskRunner;

/*!
Generalization of a task that can be run in parallel. Any task that has
a fixed number of computations that are independent (i.e. they don't need to
be done in a certain order) can inherit ParallelTask and be executed in
parallel by calling the ParallelTask::execute().

Example of usage:

\code
    float result[N];
    for ( int idx=0; idx<N; idx++ )
    	result[idx] = input1[idx]* function( idx, other, variables );
\endcode

could be made parallel by adding the class:

\code

class CalcClass : public ParallelTask
{
public:
    int		totalNr() const { return N; }
    int		doWork( int start, int stop, int threadid )
    		{
		    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
		    {
		    	result[idx] = input1[idx] *
				      function( idx, other, variables );
			reportNrDone( 1 );
		    }

		    return true;
		}
};

\endcode
and in use that instead of the for-loop:
\code
    CalcClass myclass( N, my, parameters );
    myclass.exectute();
\endcode
		
*/


class ParallelTask : public Task
{
public:
    virtual		~ParallelTask();

    virtual int		minThreadSize() const			{ return 10; }
    			/*!<\returns the minimum number of computations that
			     effectively can be run in a separate thread.
			     A small number will give a large overhead for when
			     each step is quick and totalNr is small. */

    virtual int		totalNr() const					= 0;
    			/*!<\returns the number of times the process should be
			    run. */
    bool		execute();
    			/*!<Runs the process the desired number of times. \note
			    that the function has static threads (normally the
			    same number as there are processors on the machine),
			    and these static threads will be shared by all
			    instances of ParallelTask::execute. */

    void		enableNrDoneCounting(bool yn);
    			/*<!It is not guaranteed that it's implemented by
			    the class. If not, nrDone() will return -1. */

    int			nrDone() const;
    			//!<May be -1, i.e. class does not report nrdone.
    
protected:
    			ParallelTask();
    int			calculateThreadSize(int totalnr,int nrthreads,
	    				    int idx) const;

    void		reportNrDone( int nrdone=1 );
    			/*!<Call this from within your thread to say
			    that you have done something. */

    static Threads::ThreadWorkManager&	twm();
private:

    virtual bool	doWork( int start, int stop, int threadid )	= 0;
    			/*!<The functions that does the job. The function
			    will be called with all intervals from 0 to 
			    ParallelTask::totalNr()-1. The function must
			    be designed to be able to run in parallel.
			    \param threadid gives an identifier (between 0 and
			    	   nr of threads -1) that is unique to each call
				   to doWork. */
    virtual bool	doPrepare(int nrthreads)		{ return true; }
    			/*!<Called before any doWork is called. */
    virtual bool	doFinish(bool success)			{ return true; }
    			/*!<Called after all doWork have finished.
			    \param success indicates whether all doWork returned
			           true. */

    friend class			ParallelTaskRunner;
    int					nrdone_;
    Threads::Mutex*			nrdonemutex_;
};


class TaskRunner
{
public:
    			TaskRunner(Task* t)
			    : task_( t )	{}
    virtual 		~TaskRunner()		{}

    virtual void	setTask(Task& t)	{ task_ = &t; }
    virtual bool	execute()		{ return task_->execute(); }

protected:

    Task*		task_;
};

#endif
