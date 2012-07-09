	/*/+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
-*/

static const char* rcsID mUnusedVar = "$Id: task.cc,v 1.35 2012-07-09 20:15:08 cvskris Exp $";

#include "task.h"

#include "threadwork.h"
#include "thread.h"
#include "varlenarray.h"
#include "progressmeter.h"
#include "ptrman.h"


Task::Task( const char* nm )
    : NamedObject( nm )
    , workcontrolcondvar_( 0 )
    , control_( Task::Run )
{}


Task::~Task()
{ delete workcontrolcondvar_; }


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


void TaskGroup::addTask( Task* t )
{ tasks_ += t; }


void TaskGroup::setProgressMeter( ProgressMeter* p )
{
    for ( int idx=0; idx<tasks_.size(); idx++ )
	tasks_[idx]->setProgressMeter( p );
}


void TaskGroup::enableNrDoneCounting( bool yn )
{
    for ( int idx=0; idx<tasks_.size(); idx++ )
	tasks_[idx]->enableNrDoneCounting( yn );
}


od_int64 TaskGroup::nrDone() const
{
    lock_.lock();
    const od_int64 res = tasks_[curtask_]->nrDone();
    lock_.unLock();
    return res;
}


od_int64 TaskGroup::totalNr() const
{
    lock_.lock();
    return tasks_[curtask_]->totalNr();
    lock_.unLock();
}


const char* TaskGroup::message() const
{
    lock_.lock();
    return tasks_[curtask_]->message();
    lock_.unLock();
}


const char* TaskGroup::nrDoneText() const
{
    lock_.lock();
    return tasks_[curtask_]->nrDoneText();
    lock_.unLock();
}


bool TaskGroup::execute()
{
    lock_.lock();
    for ( curtask_=0; curtask_<tasks_.size(); curtask_++ )
    {
	lock_.unLock();
	if ( !tasks_[curtask_]->execute() )
	    return false;

	lock_.lock();
    }

    lock_.unLock();

    return true;
}


void TaskGroup::enableWorkControl( bool yn )
{
    for ( int idx=0; idx<tasks_.size(); idx++ )
	tasks_[idx]->enableWorkControl( yn );
}


void TaskGroup::controlWork( Task::Control t )
{
    lock_.lock();
    tasks_[curtask_]->controlWork( t );
    lock_.unLock();
}


Task::Control TaskGroup::getState() const
{
    lock_.lock();
    Task::Control res = tasks_[curtask_]->getState();
    lock_.unLock();
    return res;
}


#define mUpdateProgressMeter \
	progressmeter_->setNrDone( nrDone() ); \
	progressmeter_->setTotalNr( totalNr() ); \
	progressmeter_->setNrDoneText( nrDoneText() ); \
	progressmeter_->setMessage( message() )

void SequentialTask::setProgressMeter( ProgressMeter* pm )
{
    progressmeter_ = pm;
    if ( progressmeter_ )
    {
	mUpdateProgressMeter;
    }
}


int SequentialTask::doStep()
{
    if ( progressmeter_ ) progressmeter_->setStarted();

    const int res = nextStep();
    if ( progressmeter_ )
    {
	mUpdateProgressMeter;
	
	if ( res<1 )
	    progressmeter_->setFinished();
    }

    return res;
}



bool SequentialTask::execute()
{
    control_ = Task::Run;

    do
    {
	int res = doStep();
	if ( !res )     return true;
	if ( res < 0 )  break;
    } while ( shouldContinue() );

    return false;
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
    : Task( nm )
    , progressmeter_( 0 )
    , nrdone_( -1 )
    , totalnrcache_( -1 )
{ }


ParallelTask::~ParallelTask()
{}


void ParallelTask::setProgressMeter( ProgressMeter* pm )
{
    progressmeter_ = pm;
    if ( progressmeter_ )
    {
	progressmeter_->setName( name() );
	progressmeter_->setMessage( message() );
	enableNrDoneCounting( true );
    }
}


void ParallelTask::reportNrDone( int nr )
{ addToNrDone( nr ); }


void ParallelTask::addToNrDone( int nr )
{
    if ( !nrdone_.setIfEqual( nr, -1 ) )
	nrdone_ += nr;

    if ( progressmeter_ )
    {
	progressmeter_->setTotalNr( totalnrcache_ );
	progressmeter_->setNrDoneText( nrDoneText() );
	progressmeter_->setMessage( message() );
	progressmeter_->setNrDone( nrDone() );
    }
}


od_int64 ParallelTask::nrDone() const
{
    return nrdone_;
}


bool ParallelTask::execute( bool parallel )
{
    Threads::WorkManager& twm = Threads::WorkManager::twm();
    if ( parallel && twm.isWorkThread() && !twm.nrFreeThreads() )
	parallel = false;

    totalnrcache_ = totalNr();
    const od_int64 nriterations = nrIterations();
    if ( totalnrcache_<=0 )
	return true;

    if ( progressmeter_ )
    {
	progressmeter_->setName( name() );
	progressmeter_->setMessage( message() );
	progressmeter_->setTotalNr( totalnrcache_ );
	progressmeter_->setStarted();
    }

    nrdone_ = -1;
    control_ = Task::Run;

    const int minthreadsize = minThreadSize();
    int maxnrthreads = parallel
	? mMIN( nriterations/minthreadsize, maxNrThreads() )
	: 1;

    if ( maxnrthreads<1 )
	maxnrthreads = 1;

    if ( Threads::getNrProcessors()==1 || maxnrthreads==1 )
    {
	if ( !doPrepare( 1 ) ) return false;
	bool res = doFinish( doWork( 0, nriterations-1, 0 ) );
	if ( progressmeter_ ) progressmeter_->setFinished();
	return res;
    }

    //Don't take all threads, as we may want to have spare ones.
    const int nrthreads = mMIN(Threads::getNrProcessors(),maxnrthreads);
    const od_int64 size = nriterations;
    if ( !size ) return true;

    ArrPtrMan<ParallelTaskRunner> runners = new ParallelTaskRunner[nrthreads];
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

	const int stop = start + threadsize-1;
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
    if ( progressmeter_ ) progressmeter_->setFinished();
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

    const od_int64 idealnrperthread = mNINT((float) totalnr/nrthreads);
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

