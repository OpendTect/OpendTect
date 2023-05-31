/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "paralleltask.h"

#include "progressmeter.h"
#include "ptrman.h"
#include "threadwork.h"
#include "thread.h"
#include "timefun.h"
#include "varlenarray.h"
#include "uistrings.h"

#include <limits.h>


Task::Task( const char* nm )
    : NamedCallBacker( nm )
{}


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
	deleteAndNullPtr( workcontrolcondvar_ );
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


mStartAllowDeprecatedSection

uiString Task::uiMessage() const
{
    const char* oldmsg = message();
    if ( oldmsg && *oldmsg )
    {
	pErrMsgOnce("Use uiMessage() in your implementation "
		    "instead of message()");
	return toUiString(oldmsg);
    }

    return tr("Working");
}


uiString Task::uiNrDoneText() const
{
    const char* oldmsg = nrDoneText();
    if ( oldmsg && *oldmsg )
    {
	pErrMsgOnce("Use uiNrDoneText() in your implementation instead of "
		"nrDoneText()");
	return toUiString(oldmsg);
    }

    return uiStdNrDoneText();
}

mStopAllowDeprecatedSection



TaskGroup::TaskGroup()
    : curtask_(-1)
    , lock_(true)
{
}


TaskGroup::~TaskGroup()
{
    deepErase(tasks_);
}



void TaskGroup::addTask( Task* t )
{ tasks_ += t; }


void TaskGroup::setEmpty()
{
    deepErase( tasks_ );
}


void TaskGroup::getTasks( TaskGroup& oth )
{
    for ( int idx=0; idx<tasks_.size(); idx++ )
	oth.addTask( tasks_[idx] );

    tasks_.setEmpty();
}


od_int64 TaskGroup::nrDone() const
{
    Threads::Locker locker( lock_ );
    if ( !cumulativecount_ )
	return tasks_.validIdx(curtask_) ? tasks_[curtask_]->nrDone() : 0;

    od_int64 res = 0;
    for ( int idx=0; idx<tasks_.size(); idx++ )
	res += tasks_[idx]->nrDone();

    return res;
}


od_int64 TaskGroup::totalNr() const
{
    Threads::Locker locker( lock_ );
    if ( !cumulativecount_ )
	return tasks_.validIdx(curtask_) ? tasks_[curtask_]->totalNr() : 0;

    od_int64 res = 0;
    for ( int idx=0; idx<tasks_.size(); idx++ )
	res += tasks_[idx]->totalNr();

    return res;
}


uiString TaskGroup::uiMessage() const
{
    Threads::Locker locker( lock_ );
    if ( !tasks_.validIdx(curtask_) )
	return uiStrings::sProcessing();

    return tasks_[curtask_]->uiMessage();
}


uiString TaskGroup::uiNrDoneText() const
{
    Threads::Locker locker( lock_ );
    return tasks_.validIdx(curtask_)
	? tasks_[curtask_]->uiNrDoneText()
	: uiString::emptyString();
}


bool TaskGroup::execute()
{
    Threads::Locker locker( lock_ );
    for ( curtask_=0; curtask_<tasks_.size(); curtask_++ )
    {
	Task* toexec = tasks_[curtask_];
	toexec->setProgressMeter( progressMeter() );

	locker.unlockNow();
	const bool res = toexec->execute();
	locker.reLock();
	updateProgressMeter( true );
	if ( !res )
	    return false;
    }

    return true;
}


void TaskGroup::enableWorkControl( bool yn )
{
    for ( int idx=0; idx<tasks_.size(); idx++ )
	tasks_[idx]->enableWorkControl( yn );
}


void TaskGroup::controlWork( Task::Control t )
{
    Threads::Locker lock( lock_ );
    if ( tasks_.validIdx(curtask_) ) tasks_[curtask_]->controlWork( t );
}


Task::Control TaskGroup::getState() const
{
    Threads::Locker lock( lock_ );
    return tasks_.validIdx(curtask_) ? tasks_[curtask_]->getState()
				     : Task::Stop;
}

void TaskGroup::setParallel(bool)
{
    pErrMsg("Not implemented.");
}



ReportingTask::ReportingTask( const char* nm )
    : Task(nm)
    , progressUpdated(this)
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
    progressmeter_->setNrDoneText( uiNrDoneText() );
    progressmeter_->setMessage( uiMessage() );
    lastupdate_ = Time::getMilliSeconds();
    progressUpdated.trigger( this );
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


SequentialTask::~SequentialTask()
{
}


bool SequentialTask::doPrepare( od_ostream* )
{
    return true;
}


int SequentialTask::doStep()
{
    const int res = nextStep();
    updateProgressMeter();

    return res;
}


bool SequentialTask::doFinish( bool success, od_ostream* )
{
    return success;
}

bool SequentialTask::execute()
{
    control_ = Task::Run;
    reportProgressStarted();
    mDynamicCastGet(TextStreamProgressMeter*,tspm,progressMeter())
    od_ostream* strm = tspm ? &tspm->stream() : nullptr;
    if ( !doPrepare(strm) )
	return false;

    bool success = false;
    do
    {
	int res = doStep();
	success = !res;
	if ( success || res < 0 ) break;
    } while ( shouldContinue() );

    success = doFinish( success, strm );
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
    if ( parallel && twm.isWorkThread() && twm.nrFreeThreads()==0 )
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
	if ( !doPrepare(1) )
	    return false;

	const bool res = doFinish( doWork(0,nriterations-1,0) );
	if ( nrdone_ != -1 )
	    addToNrDone( nriterations - nrdone_ );

	reportProgressFinished();
	return res;
    }

    if ( maxnrthreads > twm.nrFreeThreads()+1 )
	maxnrthreads = twm.nrFreeThreads() + 1;

    //Don't take all threads, as we may want to have spare ones.
    const int nrthreads = mMIN(Threads::getNrProcessors(),maxnrthreads);
    const od_int64 size = nriterations;
    if ( size==0 )
	return true;

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

    if ( !doPrepare(nrtasks) )
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
    if ( nrdone_ != -1 )
	addToNrDone( nriterations - nrdone_ );

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
    if ( nrthreads==1 )
	return totalnr;

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



bool TaskRunner::execute( TaskRunner* taskrnr, Task& task )
{
    if ( taskrnr )
	return taskrnr->execute( task );

    return task.execute();
}
