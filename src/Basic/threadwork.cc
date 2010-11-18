/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: threadwork.cc,v 1.31 2010-11-18 17:24:50 cvskris Exp $";

#include "threadwork.h"
#include "task.h"
#include "thread.h"
#include "errh.h"
#include "sighndl.h"
#include <signal.h>


namespace Threads
{

Threads::WorkManager& WorkManager::twm()
{
    static Threads::WorkManager twm_( Threads::getNrProcessors()*2 );
    return twm_;
}



/*!\brief
is the worker that actually does the job and is the link between the manager
and the tasks to be performed.
*/

class WorkThread : public CallBacker
{
public:
    			WorkThread( WorkManager& );
    			~WorkThread();

    			//Interface from manager
    void		assignTask(SequentialTask&, CallBack* finishedcb,
	    			   int queueid, bool manage );

    int			getRetVal();
    			/*!< Do only call when task is finished,
			     i.e. from the cb or
			     Threads::WorkManager::imFinished()
			*/

    void		cancelWork( const SequentialTask* );
    			//!< If working on this task, cancel it and continue.
    			//!< If a nullpointer is given, it will cancel
    			//!< regardless of which task we are working on
    const SequentialTask* getTask() const { return task_; }

protected:

    void		doWork(CallBacker*);
    void 		exitWork(CallBacker*);

    WorkManager&	manager_;

    ConditionVar&	controlcond_;	//Dont change this order!
    int			retval_;	//Lock before reading or writing

    bool		exitflag_;	//Set only from destructor
    bool		cancelflag_;	//Cancel current work and continue
    SequentialTask*	task_;		
    CallBack*		finishedcb_;
    int			queueid_;
    bool		manage_;

    Thread*		thread_;

private:
    long		spacefiller_[24];
};

}; // Namespace


Threads::WorkThread::WorkThread( WorkManager& man )
    : manager_( man )
    , controlcond_( *new Threads::ConditionVar )
    , thread_( 0 )
    , queueid_( -1 )
    , manage_( false )
    , exitflag_( false )
    , cancelflag_( false )
    , task_( 0 )
{
    controlcond_.lock();
    thread_ = new Thread( mCB( this, WorkThread, doWork));
    controlcond_.unLock();

    SignalHandling::startNotify( SignalHandling::Kill,
	    			 mCB( this, WorkThread, exitWork ));
}


Threads::WorkThread::~WorkThread()
{
    SignalHandling::stopNotify( SignalHandling::Kill,
				 mCB( this, WorkThread, exitWork ));

    if ( thread_ )
    {
	controlcond_.lock();
	exitflag_ = true;
	controlcond_.signal(false);
	controlcond_.unLock();

	thread_->stop();
	delete thread_;
	thread_ = 0;
    }

    delete &controlcond_;
}


void Threads::WorkThread::doWork( CallBacker* )
{
#ifndef __win__

    sigset_t newset;
    sigemptyset(&newset);
    sigaddset(&newset,SIGINT);
    sigaddset(&newset,SIGQUIT);
    sigaddset(&newset,SIGKILL);
    pthread_sigmask(SIG_BLOCK, &newset, 0 );

#endif

    controlcond_.lock();
    while ( true )
    {
	while ( !task_ && !exitflag_ )
	    controlcond_.wait();

	if ( exitflag_ )
	{
	    controlcond_.unLock();
	    return;
	}

	while ( task_ && !exitflag_ )
	{
	    controlcond_.unLock(); //Allow someone to set the exitflag
	    retval_ = cancelflag_ ? 0 : task_->doStep();
	    controlcond_.lock();

	    if ( retval_<1 )
	    {
		if ( finishedcb_ ) finishedcb_->doCall( this );
		manager_.workloadcond_.lock();
		bool isidle = false;

		if ( manage_ )
		    delete task_;
		task_ = 0;

		const int idx =
		    manager_.reportFinishedAndAskForMore( this, queueid_ );
		if ( idx==-1 )
		{
		    queueid_ = -1;
		    finishedcb_ = 0;
		    manage_ = false;
		    isidle = true;
		}
		else
		{
		    task_ = manager_.workload_.remove( idx );
		    finishedcb_ = manager_.callbacks_.remove( idx );
		    queueid_ = manager_.workqueueid_[idx];
		    manager_.workqueueid_.remove( idx );
		    manage_ = manager_.isowner_[idx];
		    manager_.isowner_.remove( idx );
		}

		manager_.workloadcond_.unLock();
		if ( isidle )
		    manager_.isidle.trigger( &manager_ );
	    }
	}
    }

    controlcond_.unLock();
}


void Threads::WorkThread::cancelWork( const SequentialTask* canceltask )
{
    Threads::MutexLocker lock( controlcond_ );
    if ( !canceltask || canceltask==task_ )
	cancelflag_ = true;
}


void Threads::WorkThread::exitWork(CallBacker*)
{
    controlcond_.lock();
    exitflag_ = true;
    controlcond_.signal( false );
    controlcond_.unLock();

    thread_->stop();
    delete thread_;
    thread_ = 0;
}


void Threads::WorkThread::assignTask(SequentialTask& newtask, CallBack* cb,
       				     int queueid, bool manage )
{
    controlcond_.lock();
    if ( task_ )
    {
	pErrMsg( "Trying to set existing task");
	controlcond_.unLock();
	return;
    }

    task_ = &newtask;
    finishedcb_ = cb;
    queueid_ = queueid;
    manage_ = manage;

    controlcond_.signal(false);
    controlcond_.unLock();
    return;
}


int Threads::WorkThread::getRetVal()
{
    return retval_;
}


Threads::WorkManager::WorkManager( int nrthreads )
    : workloadcond_( *new ConditionVar )
    , isidle( this )
    , freeid_( cDefaultQueueID() )
{
    callbacks_.allowNull(true);

    addQueue( MultiThread );

    if ( nrthreads == -1 )
	nrthreads = Threads::getNrProcessors();

    for ( int idx=0; idx<nrthreads; idx++ )
    {
	WorkThread* wt = new WorkThread( *this );
	threads_ += wt;
	freethreads_ += wt;
    }
}


Threads::WorkManager::~WorkManager()
{
    while ( queueids_.size() )
	removeQueue( queueids_[0], false );

    deepErase( threads_ );
    deepErase( workload_ );
    delete &workloadcond_;
}


int Threads::WorkManager::addQueue( QueueType type )
{
    Threads::MutexLocker lock( workloadcond_ );

    const int id = freeid_;
    freeid_++;
    queueids_ += id;
    queuetypes_ += type;
    queueworkload_ += 0;
    queueisclosing_ += false;

    return id;
}


void Threads::WorkManager::executeQueue( int queueid )
{
    Threads::MutexLocker lock( workloadcond_ );
    const int queueidx = queueids_.indexOf( queueid );
    if ( queueidx==-1 )
	return;

    if ( queuetypes_[queueidx]!=Manual )
    {
	pErrMsg("Only manual queues can be executed" );
	return;
    }

    while ( true )
    {
	SequentialTask* task = 0;
	CallBack* cb = 0;
	bool manage = false;
	for ( int idx=0; idx<workload_.size(); idx++ )
	{
	    if ( workqueueid_[idx]==queueid )
	    {
		task = workload_.remove( idx );
		cb = callbacks_.remove( idx );
		workqueueid_.remove( idx );

		manage = isowner_[idx];
		isowner_.remove( idx );
		break;
	    }
	}

	if ( !task )
	    break;

	workqueueid_[queueidx]++;
	lock.unLock();
	task->execute();
	if ( cb ) cb->doCall( 0 );
	if ( manage ) delete task;
	lock.lock();
    }
}


void Threads::WorkManager::removeQueue( int queueid, bool finishall )
{
    Threads::MutexLocker lock(workloadcond_);
    const int queueidx = queueids_.indexOf( queueid );

    if ( !finishall )
    {
	for ( int idx=workqueueid_.size()-1; idx>=0; idx-- )
	{
	    if ( workqueueid_[idx]==queueid )
	    {
		SequentialTask* task = workload_.remove( idx );
		if ( isowner_[idx] ) delete task;

		isowner_.remove( idx );
		workqueueid_.remove( idx );
		callbacks_.remove( idx );
	    }
	}
    }

    //Wait for all threads to exit
    queueisclosing_[queueidx] = true;
    while ( queueworkload_[queueidx] && queueSizeNoLock( queueid ) )
	workloadcond_.wait();

    queueworkload_.remove( queueidx );
    queuetypes_.remove( queueidx );
    queueids_.remove( queueidx );
    queueisclosing_.remove( queueidx );
}


int Threads::WorkManager::queueSize( int queueid ) const
{
    Threads::MutexLocker lock(workloadcond_);
    return queueSizeNoLock( queueid );
}


int Threads::WorkManager::queueSizeNoLock( int queueid ) const
{
    int res = 0;
    for ( int idx=workqueueid_.size()-1; idx>=0; idx-- )
    {
	if ( workqueueid_[idx]==queueid )
	    res++;
    }

    return res;
}


void Threads::WorkManager::addWork( SequentialTask* newtask, CallBack* cb,
					  int queueid, bool firstinline,
       					  bool manage )
{
    const int nrthreads = threads_.size();
    if ( !nrthreads )
    {
	while ( true )
	{
	    newtask->execute();
	    if ( cb ) cb->doCall( 0 );
	    if ( manage ) delete newtask;
	    return;
	}
    }

    Threads::MutexLocker lock(workloadcond_);
    int const queueidx = queueids_.indexOf( queueid );
    if ( queueidx==-1 || queueisclosing_[queueidx] )
    {
	pErrMsg("Queue does not exist or is closing. Task rejected." );
	if ( manage ) delete newtask;
	return;
    }

    const int nrfreethreads = freethreads_.size();
    if ( nrfreethreads )
    {
	if ( queuetypes_[queueidx]==MultiThread || !queueworkload_[queueidx] )
	{
	    const int threadidx = nrfreethreads-1;
	    WorkThread* thread = freethreads_.remove( nrfreethreads-1 );
	    queueworkload_[queueidx]++;
	    thread->assignTask( *newtask, cb, queueid, manage );
	    return;
	}
    }

    if ( firstinline )
    {
	workqueueid_.insert( 0, queueid );
	workload_.insertAt( newtask, 0 );
	callbacks_.insertAt( cb, 0 );
	isowner_.insert( 0, manage );
    }
    else
    {
	workqueueid_ += queueid;
	workload_ += newtask;
	callbacks_ += cb;
	isowner_ += manage;
    }
}


bool Threads::WorkManager::removeWork( const SequentialTask* task )
{
    workloadcond_.lock();

    const int idx = workload_.indexOf( task );
    if ( idx==-1 )
    {
	workloadcond_.unLock();
	for ( int idy=0; idy<threads_.size(); idy++ )
	    threads_[idy]->cancelWork( task );
	return false;
    }

    if ( isowner_[idx] )
	delete workload_[idx];

    workqueueid_.remove( idx );
    workload_.remove( idx );
    callbacks_.remove( idx );
    isowner_.remove( idx );

    workloadcond_.unLock();
    return true;
}


const SequentialTask* Threads::WorkManager::getWork(CallBacker* cb) const
{
    if ( !cb ) return 0;

    for ( int idx=0; idx<threads_.size(); idx++ )
    {
	if ( threads_[idx]==cb )
	    return threads_[idx]->getTask();
    }

    return 0;
}


class WorkResultManager : public CallBacker
{
public:
		    WorkResultManager( int nrtasks )
			: nrtasks_( nrtasks )
			, nrfinished_( 0 )
			, error_( false )
		    {}

    bool	    isFinished() const
		    { return nrfinished_==nrtasks_; }

    bool	    hasErrors() const
		    { return error_; }

    void	    imFinished(CallBacker* cb )
		    {
			Threads::WorkThread* worker =
				    dynamic_cast<Threads::WorkThread*>( cb );
			rescond_.lock();
			if ( error_ || worker->getRetVal()==-1 )
			    error_ = true;

			nrfinished_++;
			if ( nrfinished_==nrtasks_ ) rescond_.signal( false );
			rescond_.unLock();
		    }

    Threads::ConditionVar	rescond_;

protected:
    int				nrtasks_;
    int				nrfinished_;
    bool			error_;
};


bool Threads::WorkManager::addWork( ObjectSet<SequentialTask>& work,
       					  bool firstinline )
{
    if ( work.isEmpty() )
	return true;

    const int nrwork = work.size();
    const int nrthreads = threads_.size();
    bool res = true;
    if ( nrthreads==1 )
    {
	for ( int idx=0; idx<nrwork; idx++ )
	{
	    if ( work[idx]->execute() )
		continue;

	    res = false;
	    break;
	}
    }
    else
    {
	WorkResultManager resultman( nrwork-1 );

	CallBack cb( mCB( &resultman, WorkResultManager, imFinished ));

	for ( int idx=1; idx<nrwork; idx++ )
	    addWork( work[idx], &cb, cDefaultQueueID(), firstinline, false );

	res = work[0]->execute();

	resultman.rescond_.lock();
	while ( !resultman.isFinished() )
	    resultman.rescond_.wait();
	resultman.rescond_.unLock();

	if ( res ) res = !resultman.hasErrors();
    }

    return res;
}


int Threads::WorkManager::reportFinishedAndAskForMore(WorkThread* caller,
							    int oldqueueid )
{
    const int oldqueueidx = queueids_.indexOf(oldqueueid);
    queueworkload_[oldqueueidx]--;
    if ( queueisclosing_[oldqueueidx] && !queueworkload_[oldqueueidx] )
	workloadcond_.signal( true );

    int sz = workload_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	const int newqueueid = workqueueid_[idx];
	const int newqueueidx = queueids_.indexOf( newqueueid );
	if ( queuetypes_[newqueueidx]==Manual )
	    continue;

	if ( queuetypes_[newqueueidx]==SingleThread &&
	     queueworkload_[newqueueidx] )
	    continue;

	queueworkload_[newqueueidx]++;
	return idx;
    }

    freethreads_ += caller;
    return -1;
}
