#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/K.Tingdahl
 Date:		13-10-1999
________________________________________________________________________

-*/

#include "taskrunner.h"
#include "namedobj.h"
#include "objectset.h"
#include "threadlock.h"
#include "uistring.h"

class ProgressMeter;
namespace Threads { class ConditionVar; }


/*!\brief Generalization of something (e.g. a computation) that needs to be
	done in multiple steps. */

mExpClass(Basic) Task : public NamedCallBacker
{ mODTextTranslationClass(Task);
public:

    virtual		~Task();

    virtual void	setProgressMeter(ProgressMeter*)	{}
			//!<Must be called before execute()

    virtual uiString	message() const		{ return stdMessage(); }
    virtual uiString	nrDoneText() const	{ return stdNrDoneText(); }
    virtual od_int64	nrDone() const		{ return -1; }
				/*!<\note only used for displaying progress. */
    virtual od_int64	totalNr() const		{ return -1; }
				/*!\note only used for displaying progress. */
    virtual uiRetVal	errorWithDetails() const { return uiRetVal(message()); }

    virtual bool	execute()		= 0;

    enum Control	{ Run, Pause, Stop };
    virtual void	enableWorkControl(bool=true);
			//!<Must be called before execute()
    bool		workControlEnabled() const;
    virtual void	controlWork(Control);
    virtual Control	getState() const;

    static uiString	stdMessage()		{ return tr("Working");}
    static uiString	stdNrDoneText()		{ return tr("Nr Done");}
    static uiString	sPositionsDone()	{ return tr("Positions done"); }
    static uiString	sTracesDone()		{ return tr("Traces done"); }

protected:

				Task(const char* nm=nullptr);
    virtual bool		shouldContinue();
					//!<\returns wether we should continue
    Control			control_;
    Threads::ConditionVar*	workcontrolcondvar_;

};



/*!\brief Helper class for all task implementations which need a progress meter.
*/

mExpClass(Basic) ReportingTask : public Task
{
public:
    virtual		~ReportingTask();

    void		getProgress(const ReportingTask&);

protected:
			ReportingTask(const char* nm=nullptr);

    virtual void	setProgressMeter(ProgressMeter*);
    ProgressMeter*	progressMeter() const	{ return progressmeter_; }

    void		reportProgressStarted();
    void		updateReportedName();
    void		updateProgressMeter(bool forced=false,
					    od_int64* totalnr=0);
    void		incrementProgress();
    void		resetProgress();
    void		reportProgressFinished();

private:

    ProgressMeter*	progressmeter_;
    int			lastupdate_;
};



/*!Helper class that facilitates a task that has multiple sub-tasks that
   are either run in parallel or in sequence.

   The class takes care of progress reporting as well as work
   control.
*/

mExpClass(Basic) TaskGroupController : public ReportingTask
{ mODTextTranslationClass(TaskGroupController);
public:

    od_int64		nrDone() const;
			//!<Percentage
    od_int64		totalNr() const	{ return 100; }

    uiString		nrDoneText() const
			{ return tr("Percentage done"); }

    void		enableWorkControl(bool=true);
    void		controlWork(Control);
			//!<Relays to controlled tasks

    int			nrTasks() const { return controlledtasks_.size();}
    const Task*		getTask(int idx) const { return controlledtasks_[idx]; }

protected:
			TaskGroupController()
			    : ReportingTask()
			{}

    void		controlTask(Task*);
			//!<Does not take over memory management
    void		setEmpty();

private:

    ObjectSet<Task>	controlledtasks_;
    TypeSet<float>	nrdoneweights_;

};



/*!\brief A collection of tasks, that behave as a single task.  */

mExpClass(Basic) TaskGroup : public TaskGroupController
{
public:
			TaskGroup();
    virtual		~TaskGroup() { deepErase( tasks_ ); }

    void		addTask( Task* );
			//Becomes mine

    void		setParallel(bool)			{}
    void		showCumulativeCount( bool yn )
			{ showcumulativecount_ = yn; }

    void		setEmpty();
    void		getTasks(TaskGroup&);

    od_int64		nrDone() const;
    od_int64		totalNr() const;

    uiString		message() const;
    uiString		nrDoneText() const;

    virtual bool	execute();

protected:

    ObjectSet<Task>	tasks_;
    int			curtask_;
    bool		showcumulativecount_;

    mutable Threads::Lock lock_;

};





/*!\brief The generalization of something (e.g. a computation) where the steps
  must be done in sequence, i.e. not in parallel.
*/

mExpClass(Basic) SequentialTask : public ReportingTask
{
public:
			SequentialTask(const char* nm=nullptr);
    virtual		~SequentialTask()			{}

    virtual int		doStep();
		/*!<\retval MoreToDo()		Not finished. Call me again.
		    \retval Finished()		Nothing more to do.
		    \retval ErrorOccurred()	Something went wrong.
		    \note if function returns a value greater than cMoreToDo(),
		     it should be interpreted as cMoreToDo(). */

    static int		ErrorOccurred()			{ return -1; }
    static int		Finished()			{ return 0; }
    static int		MoreToDo()			{ return 1; }
    static int		WarningAvailable()		{ return 2; }

    bool		execute();

protected:

    virtual int		nextStep()				= 0;
			/*!<\retval MoreToDo()	Not finished. Call me again.
			 \retval Finished()		Nothing more to do.
			 \retval ErrorOccurred()	Something went wrong.
			 \note if function returns a value greater than
			  cMoreToDo(),it should be interpreted as cMoreToDo().*/


};
