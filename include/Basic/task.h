#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "namedobj.h"
#include "ptrman.h"
#include "objectset.h"
#include "threadlock.h"
#include "uistring.h"

class od_ostream;
class ProgressMeter;
namespace Threads { class ConditionVar; }


/*!\brief Generalization of something (e.g. a computation) that needs to be
	done in multiple steps. */

mExpClass(Basic) Task : public NamedCallBacker
{ mODTextTranslationClass(Task)
public:

    virtual		~Task();
			mOD_DisableCopy(Task)

    virtual void	setProgressMeter(ProgressMeter*)	{}
			//!<Must be called before execute()

    virtual uiString	uiMessage() const;
    virtual uiString	uiNrDoneText() const;
    virtual od_int64	nrDone() const			{ return -1; }
			/*!<\note nrDone is only used for displaying progress
				  and will be compared to totalNr to show
				  user how large part of the task that is
				  finished. */

    virtual od_int64	totalNr() const			{ return -1; }
			/*!\note totalNr is only used for displaying
				 progress. */
    virtual double	progressFactor() const			{ return 1.; }
			/*!\note conversion factor used to scale nrDone() and
				 totalNr() when displayed in the GUI  	*/
    virtual uiRetVal	errorWithDetails() const
			{ return uiRetVal(uiMessage()); }

    static uiString	uiStdNrDoneText() { return tr("Nr Done"); }

    virtual bool	execute()		= 0;

    enum Control	{ Run, Pause, Stop };
    virtual void	enableWorkControl(bool=true);
			//!<Must be called before execute()
    bool		workControlEnabled() const;
    virtual void	controlWork(Control);
    virtual Control	getState() const;

protected:

				Task(const char* nm=nullptr);
    virtual bool		shouldContinue();
					//!<\returns wether we should continue
    Control			control_			= Task::Run;
    Threads::ConditionVar*	workcontrolcondvar_		= nullptr;

private:

    mDeprecated("Use uiMessage")
    virtual const char* message() const			{ return nullptr; }
    mDeprecated("Use uiNrDoneText")
    virtual const char* nrDoneText() const		{ return nullptr; }

};



/*!\brief Helper class for all task implementations which need a progress meter.
*/

mExpClass(Basic) ReportingTask : public Task
{
public:
    virtual		~ReportingTask();
			mOD_DisableCopy(ReportingTask)

    void		getProgress(const ReportingTask&);

    void		setSimpleMeter(bool yn, int repperc);
    bool		useSimpleMeter() const;
    int			simpleMeterStep() const;
    static PtrMan<ProgressMeter> getTextProgressMeter(od_ostream&,
						      const IOPar* iop=nullptr);
    static bool		needSimpleLogging(const IOPar&);
    static const char*	sKeySimpleLogging();
    static const char*	sKeySimpleLoggingStep();

    Notifier<ReportingTask>	progressUpdated;

protected:
			ReportingTask(const char* nm=nullptr);

    void		setProgressMeter(ProgressMeter*) override;
    ProgressMeter*	progressMeter() const	{ return progressmeter_; }

    void		reportProgressStarted();
    void		updateReportedName();
    void		updateProgressMeter(bool forced=false,
					    od_int64* totalnr=0);
    void		incrementProgress();
    void		resetProgress();
    void		reportProgressFinished();

private:

    ProgressMeter*	progressmeter_ = nullptr;
    od_int64		lastupdate_;
    bool		simplemeter_ = false;
    int			repperc_ = 5;
};


/*!\brief A collection of tasks, that behave as a single task.  */

mExpClass(Basic) TaskGroup : public ReportingTask
{
public:
			TaskGroup();
    virtual		~TaskGroup();

    void		addTask( Task* );
			//Becomes mine

    void		setParallel(bool);
    void		showCumulativeCount( bool yn )
			{ cumulativecount_ = yn; }
    void		setEmpty();
    void		getTasks(TaskGroup&);

    od_int64		nrDone() const override;
    od_int64		totalNr() const override;

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

    bool		execute() override;

    void		enableWorkControl(bool=true) override;
    void		controlWork(Control) override;
    Control		getState() const override;

protected:

    ObjectSet<Task>	tasks_;
    int			curtask_;
    bool        cumulativecount_ = false;

    mutable Threads::Lock lock_;

};





/*!\brief The generalization of something (e.g. a computation) where the steps
  must be done in sequence, i.e. not in parallel.
*/

mExpClass(Basic) SequentialTask : public ReportingTask
{
public:
		SequentialTask(const char* nm=nullptr);
    virtual	~SequentialTask();

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

    bool	execute() override;

protected:

    virtual int	nextStep()				= 0;
		/*!<\retval MoreToDo()		Not finished. Call me again.
		    \retval Finished()		Nothing more to do.
		    \retval ErrorOccurred()	Something went wrong.
		    \note if function returns a value greater than cMoreToDo(),
			  it should be interpreted as cMoreToDo(). */

    virtual bool	doPrepare(od_ostream* =nullptr);
			/*!<\will be called before the 1st nextStep() call */
    virtual bool	doFinish(bool success,od_ostream* =nullptr);
			/*!<\will be called after the last nextStep() call */
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
			mOD_DisableCopy(TaskRunner)

    virtual bool	execute(Task& t)
			{ return (execres_ = t.execute()); }
    virtual bool	execResult() const		{ return execres_; }

protected:

    bool		execres_;

};
