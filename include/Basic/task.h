#ifndef task_h
#define task_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril/K.Tingdahl
 Date:		13-10-1999
 RCS:		$Id: task.h,v 1.39 2012-08-03 13:00:15 cvskris Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "namedobj.h"
#include "objectset.h"
#include "thread.h"


namespace Threads { class ThreadWorkManager; }

class ProgressMeter;



/* generalization of something (e.g. a computation) that needs to be
   done in multiple steps. */

mClass(Basic) Task : public NamedObject
{
public:
    virtual		~Task();

    virtual void	setProgressMeter(ProgressMeter*)	{}
    			//!<Must be called before execute()
    virtual void	enableNrDoneCounting( bool yn )		{}
    			/*<!It is not guaranteed that it's implemented by
			    the class. If not, nrDone() will return -1.
			    must be called before execute(). */

    virtual od_int64	nrDone() const			{ return -1; }
    			/*!<\note nrDone is only used for displaying progress
			          and will be compared to totalNr to show
				  user how large part of the task that is
				  finished. */
    			
    virtual od_int64	totalNr() const			{ return -1; }
    			/*!\note totalNr is only used for displaying
			         progress. */
    virtual const char*	message() const			{ return "Working"; }
    virtual const char*	nrDoneText() const		{ return "Nr Done"; }

    virtual bool	execute()			= 0;

    virtual void	enableWorkControl(bool=true);
    			//!<Must be called before execute()
    enum Control	{ Run, Pause, Stop };
    virtual void	controlWork(Control);
    virtual Control	getState() const;

protected:
					Task(const char* nm=0);
    virtual bool			shouldContinue();
    					//!<\returns wether we should continue
    Threads::ConditionVar*		workcontrolcondvar_;
    Task::Control			control_;
};


/*! A collection of tasks, that behave as a single task. */
mClass(Basic) TaskGroup : public Task
{
public:
    			~TaskGroup() { deepErase( tasks_ ); }
    void		addTask( Task* );
    			//Becomes mine

    void		setProgressMeter(ProgressMeter*);
    virtual void	enableNrDoneCounting( bool yn );
    virtual od_int64	nrDone() const;
    virtual od_int64	totalNr() const;

    virtual const char*	message() const;
    virtual const char*	nrDoneText() const;

    virtual bool	execute();

    void		enableWorkControl(bool=true);
    virtual void	controlWork(Control);
    virtual Control	getState() const;

protected:

    ObjectSet<Task>		tasks_;
    int				curtask_;

    mutable Threads::Mutex	lock_;
};



/*!The generalization of something (e.g. a computation) where the steps must
   be done in sequence, i.e. not parallely. */


mClass(Basic) SequentialTask : public Task
{
public:
		SequentialTask(const char* nm=0)
		    : Task(nm), progressmeter_( 0 )	{}
    
    virtual	~SequentialTask() 			{}
    void	setProgressMeter(ProgressMeter*);
    virtual int	doStep();
    		/*!<\retval MoreToDo()		Not finished. Call me again.
		    \retval Finished()		Nothing more to do.
		    \retval ErrorOccurred()	Something went wrong.
		    \note if function returns a value greater than cMoreToDo(),
			  it should be interpreted as cMoreToDo(). */

    static int	ErrorOccurred()				{ return -1; }
    static int	Finished()				{ return 0; }
    static int	MoreToDo()				{ return 1; }
    static int	WarningAvailable()			{ return 2; }

    bool	execute();

protected:

    virtual int	nextStep()				= 0;
    		/*!<\retval MoreToDo()		Not finished. Call me again.
		    \retval Finished()		Nothing more to do.
		    \retval ErrorOccurred()	Something went wrong.
		    \note if function returns a value greater than cMoreToDo(),
			  it should be interpreted as cMoreToDo(). */

    ProgressMeter*					progressmeter_;
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
    od_int64	nrIterations() const { return N; }
    int		doWork( od_int64 start, od_int64 stop, int threadid )
    		{
		    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
		    {
		    	result[idx] = input1[idx] *
				      function( idx, other, variables );
			addToNrDone( 1 );
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


mClass(Basic) ParallelTask : public Task
{
public:
    virtual		~ParallelTask();

    bool		execute() { return execute(true); }
    			/*!<Runs the process the desired number of times. \note
			    that the function has static threads (normally the
			    same number as there are processors on the machine),
			    and these static threads will be shared by all
			    instances of ParallelTask::execute. */
    virtual bool	execute(bool parallel);
    			/*!<Runs the process the desired number of times. \note
			    that the function has static threads (normally the
			    same number as there are processors on the machine),
			    and these static threads will be shared by all
			    instances of ParallelTask::execute. */

    void		setProgressMeter(ProgressMeter*);
    od_int64		nrDone() const;
    			//!<May be -1, i.e. class does not report nrdone.
    
    od_int64		totalNr() const	{ return nrIterations(); }

protected:
    virtual od_int64	nrIterations() const				= 0;
    			/*!<\returns the number of times the process should be
			    run. */
    virtual int		maxNrThreads() const;
    virtual int		minThreadSize() const	{ return 1; }
    			/*!<\returns the minimum number of computations that
			     effectively can be run in a separate thread.
			     A small number will give a large overhead for when
			     each step is quick and nrIterations is small. */
    virtual bool 	stopAllOnFailure() const	{ return true; }
    			/*!<If one thread fails, should an attempt be made to
			    stop the others? If true, enableWorkControl will
			    be enabled, and threads should call shouldContinue()
			    regularly. */

			ParallelTask(const char* nm=0);
    od_int64		calculateThreadSize(od_int64 totalnr,int nrthreads,
	    				    int thread) const;

    void		addToNrDone(int increment);
    			/*!<Call this from within your thread to say
			    that you have done something. */


    void		reportNrDone(int nrdone);
    			//Legacy, don't use in new code. Use addToNrDone
private:

    virtual bool	doWork(od_int64 start,od_int64 stop,int threadid) = 0;
    			/*!<The functions that does the job. The function
			    will be called with all intervals from 0 to 
			    ParallelTask::nrIterations()-1. The function must
			    be designed to be able to run in parallel.
			    \param threadid gives an identifier (between 0 and
			    	   nr of threads -1) that is unique to each call
				   to doWork. */
    virtual bool	doPrepare(int nrthreads)	{ return true; }
    			/*!<Called before any doWork is called. */
    virtual bool	doFinish(bool success)		{ return success; }
    			/*!<Called after all doWork have finished.
			    \param success indicates whether all doWork returned
			           true. */

    friend class			ParallelTaskRunner;
    ProgressMeter*			progressmeter_;
    Threads::Atomic<od_int64>		nrdone_;

private:
    od_int64				totalnrcache_;
};


/*!Macros to define a class to exectute your loop in parallel.

  The loop index is 'idx'.

Example:

    The original loop was:

for ( int isamp=0; isamp<outnrsamples; isamp++ )
    trc.set( isamp, storinterp_->get(blockbuf_,isamp), curcomp );

    There are 4 parameters (trc, curcomp, blockbuf_ and storinterp_) to
    pass to the executing object, thus:

mDefParallelCalc4Pars( SEGYSampleInterpreter, trc.size(),
		   SeisTrc&,trc, int,curcomp, unsigned char*,blockbuf,
		   const TraceDataInterpreter*,storinterp)
mDefParallelCalcBody( \* No initializations *\,
	    trc_.set( idx, storinterp_->get(blockbuf_,idx), curcomp_ );
		    , \* No post-operations *\)

SEGYSampleInterpreter interp( trc.size(), trc, curcomp, blockbuf_, storinterp_);
interp.execute();
 
 */

#define mDefParallelCalcNoPars(clss) \
	class clss : public ParallelTask \
	{ \
	public: \
	    od_int64	sz_; \
	    clss( od_int64 _sz_ ) : sz_(_sz_)  		{} \
	    od_int64 nrIterations() const { return sz_; }

#define mDefParallelCalc1Par(clss,T1,v1) \
	class clss : public ParallelTask \
	{ \
	public: \
	    od_int64	sz_; \
	    T1 		v1##_; \
	    clss( od_int64 _sz_, T1 _##v1##_ ) \
		: sz_(_sz_), v1##_(_##v1##_) 		{} \
	    od_int64 nrIterations() const { return sz_; }

#define mDefParallelCalc2Pars(clss,T1,v1,T2,v2) \
	class clss : public ParallelTask \
	{ \
	public: \
	    od_int64	sz_; \
	    T1 v1##_; T2 v2##_; \
	    clss( od_int64 _sz_, T1 _##v1##_, T2 _##v2##_ ) \
		: sz_(_sz_) \
		, v1##_(_##v1##_), v2##_(_##v2##_) 			{} \
	    od_int64 nrIterations() const { return sz_; }

#define mDefParallelCalc3Pars(clss,T1,v1,T2,v2,T3,v3) \
	class clss : public ParallelTask \
	{ \
	public: \
	    od_int64	sz_; \
	    T1 v1##_; T2 v2##_; T3 v3##_; \
	    clss( od_int64 _sz_, \
		    T1 _##v1##_, T2 _##v2##_, T3 _##v3##_ ) \
		: sz_(_sz_) \
		, v1##_(_##v1##_), v2##_(_##v2##_) , v3##_(_##v3##_)	{} \
	    od_int64 nrIterations() const { return sz_; }

#define mDefParallelCalc4Pars(clss,T1,v1,T2,v2,T3,v3,T4,v4) \
	class clss : public ParallelTask \
	{ \
	public: \
	    od_int64	sz_; \
	    T1 v1##_; T2 v2##_; T3 v3##_; T4 v4##_; \
	    clss( od_int64 _sz_, \
		    T1 _##v1##_, T2 _##v2##_, T3 _##v3##_, T4 _##v4##_ ) \
		: sz_(_sz_) \
		, v1##_(_##v1##_), v2##_(_##v2##_) \
		, v3##_(_##v3##_), v4##_(_##v4##_)		{} \
	    od_int64 nrIterations() const { return sz_; }

#define mDefParallelCalcBody(preop,impl,postop) \
	    bool doWork( od_int64 start, od_int64 stop, int ) \
	    { \
		preop; \
		for ( int idx=start; idx<=stop; idx++ ) \
		    { impl; } \
		postop; \
		return true; \
	    } \
	};



/*!Class that can execute a task. Can be used as such, be inherited by
   fancy subclasses with user interface and progressbars etc. */

mClass(Basic) TaskRunner
{
public:

			TaskRunner() : execres_(false)	{}
    virtual 		~TaskRunner()			{}

    virtual bool	execute(Task& t)
    			{ return (execres_ = t.execute()); }
    virtual bool	execResult() const		{ return execres_; }

protected:

    bool		execres_;

};

#endif

