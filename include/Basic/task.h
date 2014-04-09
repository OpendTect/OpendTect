#ifndef task_h
#define task_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril/K.Tingdahl
 Date:		13-10-1999
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "namedobj.h"
#include "objectset.h"
#include "threadlock.h"
#include "uistring.h"

class ProgressMeter;
namespace Threads { class ConditionVar; }


/*!\brief Generalization of something (e.g. a computation) that needs to be
	done in multiple steps.  */

mExpClass(Basic) Task : public NamedObject
{
public:
    virtual		~Task();

    virtual void	setProgressMeter(ProgressMeter*)	{}
			//!<Must be called before execute()

    virtual od_int64	nrDone() const			{ return -1; }
			/*!<\note nrDone is only used for displaying progress
			          and will be compared to totalNr to show
				  user how large part of the task that is
				  finished. */

    virtual od_int64	totalNr() const			{ return -1; }
			/*!\note totalNr is only used for displaying
			         progress. */

    virtual uiStringCopy uiMessage() const;
    virtual uiStringCopy uiNrDoneText() const;

    virtual bool	execute()			= 0;

    virtual void	enableWorkControl(bool=true);
			//!<Must be called before execute()
    enum Control	{ Run, Pause, Stop };
    virtual void	controlWork(Control);
    virtual Control	getState() const;

protected:
				Task(const char* nm=0);
    virtual bool		shouldContinue();
					//!<\returns wether we should continue
    Control			control_;
    Threads::ConditionVar*	workcontrolcondvar_;

public:
    virtual const char* message() const 		{ return "Working"; }
    virtual const char* nrDoneText() const		{ return "Nr Done"; }
};


/*!\brief A collection of tasks, that behave as a single task.  */

mExpClass(Basic) TaskGroup : public Task
{
public:
			TaskGroup();
			~TaskGroup() { deepErase( tasks_ ); }
    void		addTask( Task* );
			//Becomes mine

    void		setParallel(bool);

    void		setProgressMeter(ProgressMeter*);
    virtual od_int64	nrDone() const;
    virtual od_int64	totalNr() const;

    uiStringCopy	uiMessage() const;
    uiStringCopy	uiNrDoneText() const;

    virtual bool	execute();

    void		enableWorkControl(bool=true);
    virtual void	controlWork(Control);
    virtual Control	getState() const;

protected:

    ObjectSet<Task>	tasks_;
    int			curtask_;

    mutable Threads::Lock lock_;

};


/*!\brief The generalization of something (e.g. a computation) where the steps
  must be done in sequence, i.e. not in parallel.
*/

mExpClass(Basic) SequentialTask : public Task
{
public:
		SequentialTask(const char* nm=0)
		    : Task(nm), progressmeter_( 0 )	{}

    virtual	~SequentialTask()			{}
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

    ProgressMeter* progressmeter_;
};



/*!\brief Class that can execute a task.

  Can be used as such, be inherited by fancy subclasses with user interface
  and progressbars etc.
*/

mExpClass(Basic) TaskRunner
{
public:
    static bool		execute(TaskRunner* tr, Task& );
			//!<Taskrunner may be zero

			TaskRunner() : execres_(false)	{}
    virtual		~TaskRunner()			{}

    virtual bool	execute(Task& t)
			{ return (execres_ = t.execute()); }
    virtual bool	execResult() const		{ return execres_; }

protected:

    bool		execres_;

};


#endif
