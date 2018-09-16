/*/+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
-*/


#include "paralleltask.h"
#include "executor.h"

#include "threadwork.h"
#include "thread.h"
#include "varlenarray.h"
#include "progressmeterimpl.h"
#include "ptrman.h"
#include "timefun.h"
#include "uistrings.h"
#include "od_ostream.h"

#include <limits.h>


Task::Task( const char* nm )
    : NamedCallBacker( nm )
    , workcontrolcondvar_( 0 )
    , control_( Task::Run )
{
}


Task::~Task()
{
    delete workcontrolcondvar_;
}


void Task::enableWorkControl( bool yn )
{
    const bool isenabled = workcontrolcondvar_;
    if ( isenabled==yn )
	return;

    if ( yn )
	workcontrolcondvar_ = new Threads::ConditionVar;
    else
    {
	delete workcontrolcondvar_;
	workcontrolcondvar_ = 0;
    }
}


bool Task::workControlEnabled() const
{
    return workcontrolcondvar_;
}


void Task::controlWork( Task::Control c )
{
    if ( !workcontrolcondvar_ )
	return;

    workcontrolcondvar_->lock();
    if ( control_!=c )
    {
	control_ = c;
	workcontrolcondvar_->signal(true);
    }

    workcontrolcondvar_->unLock();
}


Task::Control Task::getState() const
{
    if ( !workcontrolcondvar_ )
	return Task::Run;

    workcontrolcondvar_->lock();
    Task::Control res = control_;
    workcontrolcondvar_->unLock();
    return res;
}


bool Task::shouldContinue()
{
    if ( !workcontrolcondvar_ )
	return true;

    workcontrolcondvar_->lock();
    if ( control_==Task::Run )
    {
	workcontrolcondvar_->unLock();
	return true;
    }
    else if ( control_==Task::Stop )
    {
	workcontrolcondvar_->unLock();
	return false;
    }

    while ( control_==Task::Pause )
	workcontrolcondvar_->wait();

    const bool shouldcont = control_==Task::Run;

    workcontrolcondvar_->unLock();
    return shouldcont;
}


void TaskGroupController::controlTask( Task*  t )
{
    if ( workcontrolcondvar_ ) t->enableWorkControl( true );

    controlledtasks_ += t;
    nrdoneweights_ += 1;
}


void TaskGroupController::setEmpty()
{
    controlledtasks_.setEmpty();
    nrdoneweights_.setEmpty();
}


od_int64 TaskGroupController::nrDone() const
{
    const int nrtasks = controlledtasks_.size();

    float progress = 0;
    float weightsum = 0;
    for ( int idx=0; idx<nrtasks; idx++ )
    {
	const od_int64 taskprogress = controlledtasks_[idx]->nrDone();
	const od_int64 tasktotal = controlledtasks_[idx]->totalNr();

	if ( taskprogress>=0 && tasktotal>0 )
	{
	    progress += nrdoneweights_[idx] * taskprogress / tasktotal;
	    weightsum += nrdoneweights_[idx];
	}
    }

    if ( !weightsum )
	return Task::nrDone();

    return (int) (100*progress/weightsum+0.5f);
}


void TaskGroupController::enableWorkControl( bool yn )
{
    Task::enableWorkControl( yn );

    for ( int idx=0; idx<controlledtasks_.size(); idx++ )
	controlledtasks_[idx]->enableWorkControl( yn );
}


void TaskGroupController::controlWork( Task::Control t )
{
    for ( int idx=0; idx<controlledtasks_.size(); idx++ )
	controlledtasks_[idx]->controlWork( t );
}



TaskGroup::TaskGroup()
    : TaskGroupController()
    , curtask_(-1)
    , showcumulativecount_(false)
    , lock_(true)
{
}


void TaskGroup::addTask( Task* t )
{
    tasks_ += t;
    controlTask( t );
}


void TaskGroup::setEmpty()
{
    deepErase( tasks_ );
    TaskGroupController::setEmpty();
}


void TaskGroup::getTasks( TaskGroup& oth )
{
    for ( int idx=0; idx<tasks_.size(); idx++ )
	oth.addTask( tasks_[idx] );

    tasks_.setEmpty();
    TaskGroupController::setEmpty();
}


od_int64 TaskGroup::nrDone() const
{
    Threads::Locker lckr( lock_ );
    if ( !showcumulativecount_ )
	return tasks_.validIdx(curtask_) ? tasks_[curtask_]->nrDone() : 0;

    od_int64 res = 0;
    for ( int idx=0; idx<tasks_.size(); idx++ )
	res += tasks_[idx]->nrDone();

    return res;
}


od_int64 TaskGroup::totalNr() const
{
    Threads::Locker lckr( lock_ );
    if ( !showcumulativecount_ )
	return tasks_.validIdx(curtask_) ? tasks_[curtask_]->totalNr() : 0;

    od_int64 res = 0;
    for ( int idx=0; idx<tasks_.size(); idx++ )
	res += tasks_[idx]->totalNr();

    return res;
}


uiString TaskGroup::message() const
{
    Threads::Locker locker( lock_ );
    if ( !tasks_.validIdx(curtask_) )
	return uiString::empty();

    return tasks_[curtask_]->message();
}


uiString TaskGroup::nrDoneText() const
{
    Threads::Locker lckr( lock_ );
    return tasks_.validIdx(curtask_) ? tasks_[curtask_]->nrDoneText()
				     : uiString::empty();
}


bool TaskGroup::execute()
{
    Threads::Locker locker( lock_ );
    for ( curtask_=0; curtask_<tasks_.size(); curtask_++ )
    {
	Task* toexec = tasks_[curtask_];
	toexec->setProgressMeter( progressMeter() );

	locker.unlockNow();
	if ( !toexec->execute() )
	    return false;
	locker.reLock();
    }

    return true;
}



ReportingTask::ReportingTask( const char* nm )
    : Task(nm)
    , progressmeter_(0)
    , lastupdate_(Time::getMilliSeconds())
{
}


ReportingTask::~ReportingTask()
{
}


void ReportingTask::setProgressMeter( ProgressMeter* pm )
{
    progressmeter_ = pm;
    updateReportedName();
    updateProgressMeter( true );
}


void ReportingTask::getProgress( const ReportingTask& oth )
{
    setProgressMeter( oth.progressmeter_ );
}


void ReportingTask::reportProgressStarted()
{
    if ( progressmeter_ )
	progressmeter_->setStarted();
}


void ReportingTask::updateReportedName()
{
    if ( progressmeter_ )
	progressmeter_->setName( name() );
}


#define mDefaultTimeLimit 250
void ReportingTask::updateProgressMeter( bool forced, od_int64* totalnrcache )
{
    if ( !progressmeter_ ||
	 (Time::passedSince(lastupdate_) < mDefaultTimeLimit && !forced ) )
	return;

    progressmeter_->setNrDone( nrDone() );
    progressmeter_->setTotalNr( totalnrcache ? *totalnrcache : totalNr() );
    progressmeter_->setNrDoneText( nrDoneText() );
    progressmeter_->setMessage( message() );
    lastupdate_ = Time::getMilliSeconds();
}


void ReportingTask::incrementProgress()
{
    if ( progressmeter_ )
	++(*progressmeter_);
}


void ReportingTask::resetProgress()
{
    if ( progressmeter_ )
	progressmeter_->setNrDone( -1 );
}


void ReportingTask::reportProgressFinished()
{
    updateProgressMeter( true );
    if ( progressmeter_ )
	progressmeter_->setFinished();
}



SequentialTask::SequentialTask( const char* nm )
    : ReportingTask(nm)
{
}


int SequentialTask::doStep()
{
    const int res = nextStep();
    updateProgressMeter();

    return res;
}



bool SequentialTask::execute()
{
    control_ = Task::Run;

    reportProgressStarted();
    bool success = false;
    do
    {
	int res = doStep();
	success = !res;
	if ( success || res < 0 ) break;
    } while ( shouldContinue() );
    reportProgressFinished();

    return success;
}


class ParallelTaskRunner : public CallBacker
{
public:
		ParallelTaskRunner()
		    : task_( 0 )					{}

    void	set( ParallelTask* task, od_int64 start, od_int64 stop,
		     int threadidx )
		{
		    task_ = task;
		    start_ = start;
		    stop_ = stop;
		    threadidx_ = threadidx;
		}

    bool	doRun()
		{
		    if ( !task_->doWork(start_,stop_,threadidx_) )
		    {
			task_->controlWork( Task::Stop );
			return false;
		    }

		    return true;
		}

protected:
    od_int64		start_;
    od_int64		stop_;
    int			threadidx_;
    ParallelTask*	task_;
};


ParallelTask::ParallelTask( const char* nm )
    : ReportingTask( nm )
    , nrdone_( -1 )
    , totalnrcache_( -1 )
{}


ParallelTask::ParallelTask( const ParallelTask& t )
    : ReportingTask( t.name() )
    , nrdone_( -1 )
    , totalnrcache_( -1 )
{}


ParallelTask::~ParallelTask()
{}


void ParallelTask::addToNrDone( od_int64 nr )
{
    if ( nrdone_.load()!=-1 || !nrdone_.setIfValueIs( -1,  nr, 0 ) )
	nrdone_ += nr;

    updateProgressMeter( false, &totalnrcache_ );
}


void ParallelTask::quickAddToNrDone( od_int64 idx )
{
    if ( idx%nrdonebigchunksz_ || idx == 0 )
	return;

    addToNrDone( nrdonebigchunksz_ );
}


void ParallelTask::resetNrDone()
{
    nrdone_ = -1;
    resetProgress();
}


od_int64 ParallelTask::nrDone() const
{
    return nrdone_;
}

#define cBigChunkSz 100000

bool ParallelTask::executeParallel( bool parallel )
{
    Threads::WorkManager& twm = Threads::WorkManager::twm();
    if ( parallel && twm.isWorkThread() && !twm.nrFreeThreads() )
	parallel = false;

    totalnrcache_ = totalNr();
    const od_int64 nriterations = nrIterations();
    if ( totalnrcache_<=0 )
	return true;

    updateReportedName();
    updateProgressMeter( true, &totalnrcache_ );
    reportProgressStarted();

    nrdone_ = -1;
    nrdonebigchunksz_ = nriterations >= cBigChunkSz ? cBigChunkSz
		      : nriterations >= 100 ? nriterations / 100 : 1;
    control_ = Task::Run;

    const int minthreadsize = minThreadSize();
    int maxnrthreads = parallel
	? mMIN( (int)(nriterations/minthreadsize), maxNrThreads() )
	: 1;

    if ( maxnrthreads<1 )
	maxnrthreads = 1;

    if ( Threads::getNrProcessors()==1 || maxnrthreads==1 )
    {
	if ( !doPrepare( 1 ) ) return false;
	bool res = doFinish( doWork( 0, nriterations-1, 0 ) );
	if ( nrdone_ != -1 ) addToNrDone( nriterations - nrdone_ );
	reportProgressFinished();
	return res;
    }

    //Don't take all threads, as we may want to have spare ones.
    const int nrthreads = mMIN(Threads::getNrProcessors(),maxnrthreads);
    const od_int64 size = nriterations;
    if ( !size ) return true;

    mAllocLargeVarLenArr( ParallelTaskRunner, runners, nrthreads );
    mAllocVarLenArr( Threads::Work, tasks, nrthreads );

    od_int64 start = 0;
    int nrtasks = 0;
    for ( int idx=nrthreads-1; idx>=0; idx-- )
    {
	if ( start>=size )
	    break;

	const od_int64 threadsize = calculateThreadSize( size, nrthreads, idx );
	if ( threadsize==0 )
	    continue;

	const od_int64 stop = start + threadsize-1;
	runners[nrtasks].set( this, start, stop, idx );
	tasks[nrtasks] = mWMT(&runners[idx],ParallelTaskRunner,doRun);

	nrtasks++;
	start = stop+1;
    }

    if ( !doPrepare( nrtasks ) )
	return false;

    bool res;
    if ( nrtasks<2 )
	res = doWork( 0, nriterations-1, 0 );
    else
    {
	if ( stopAllOnFailure() )
	    enableWorkControl( true );

	res = twm.executeWork( tasks, nrtasks );
    }

    res = doFinish( res );
    if ( nrdone_ != -1 ) addToNrDone( nriterations - nrdone_ );
    reportProgressFinished();
    return res;
}


int ParallelTask::maxNrThreads() const
{
    const od_int64 res = nrIterations();
    if ( res>INT_MAX )
        return INT_MAX;

    return (int) res;
}


od_int64 ParallelTask::calculateThreadSize( od_int64 totalnr, int nrthreads,
				            int idx ) const
{
    if ( nrthreads==1 ) return totalnr;

    const od_int64 idealnrperthread =
	mNINT64((float) (totalnr/(od_int64) nrthreads));
    const od_int64 nrperthread = idealnrperthread<minThreadSize()
	?  minThreadSize()
	: idealnrperthread;

    const od_int64 start = idx*nrperthread;
    od_int64 nextstart = start + nrperthread;

    if ( nextstart>totalnr )
	nextstart = totalnr;

    if ( idx==nrthreads-1 )
	nextstart = totalnr;

    if ( nextstart<=start )
	return 0;

    return nextstart-start;
}



bool TaskRunner::execute( TaskRunner* tskr, Task& task )
{
    if ( tskr )
	return tskr->execute( task );

    return task.execute();
}


bool SilentTaskRunner::execute( Task& t )
{
    return (execres_ = t.execute());
}


void SilentTaskRunner::emitErrorMessage( const uiString& msg, bool wrn ) const
{
    ErrMsg( msg );
}


bool LoggedTaskRunner::execute( Task& tsk )
{
    mDynamicCastGet( Executor*, exec, &tsk )

    if ( exec )
	execres_ = exec->go( strm_ );
    else
    {
	TextStreamProgressMeter progressmeter( strm_ );
	tsk.setProgressMeter( &progressmeter );
	execres_ = tsk.execute();
    }

    return execres_;
}


void LoggedTaskRunner::emitErrorMessage( const uiString& msg, bool wrn ) const
{
    strm_ << "\n" << toString(msg) << od_endl;
}


bool TaskRunnerProvider::execute( const TaskRunnerProvider* trprov, Task& task )
{
    return trprov ? trprov->execute( task ) : task.execute();
}
