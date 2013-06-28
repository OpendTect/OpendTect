/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "threadwork.h"
#include "task.h"
#include "thread.h"
#include "errh.h"
#include "sighndl.h"
#include <signal.h>


namespace Threads
{

const Threads::WorkManager* thetwm = 0;

static void shutdownTWM()
{
   WorkManager::twm().shutdown();
}


Threads::WorkManager& WorkManager::twm()
{
    static PtrMan<Threads::WorkManager> twm_= 0;
    if ( !twm_ )
    {
	Threads::WorkManager* newtwm =
	    new Threads::WorkManager( Threads::getNrProcessors()*2 );
	if ( !twm_.setIfNull( newtwm ) )
	{
	    delete newtwm;
	}
	else
	{
	    thetwm = newtwm;
	    NotifyExitProgram( &shutdownTWM );
	}
    }

    return *twm_;
}


class SimpleWorker : public CallBacker
{
public:
    			SimpleWorker() : retval_( false )	{}
    virtual		~SimpleWorker()				{}

    void		runWork(Work& w,CallBack* cb)
    			{
			    retval_ = w.doRun();
			    if ( cb ) cb->doCall( this );
			}

    bool		getRetVal() const 	{ return retval_; }
    			/*!< Do only call when task is finished,
			     i.e. from the cb or
			     Threads::WorkManager::imFinished()
			*/
protected:
    bool		retval_;
};



/*!\brief
is the worker that actually does the job and is the link between the manager
and the tasks to be performed.
*/

class WorkThread : public SimpleWorker
{
public:
    			WorkThread( WorkManager& );
    			~WorkThread();

    			//Interface from manager
    void		assignTask(const ::Threads::Work&,
	    			   const CallBack& finishedcb, int queueid );


    void		cancelWork( const ::Threads::Work* );
    			//!< If working on this task, cancel it and continue.
    			//!< If a nullpointer is given, it will cancel
    			//!< regardless of which task we are working on
    const ::Threads::Work* getTask() const { return &task_; }

    const void*		threadID() const { return thread_->threadID(); }

protected:

    void		doWork(CallBacker*);
    void 		exitWork(CallBacker*);

    WorkManager&	manager_;

    ConditionVar&	controlcond_;	//Dont change this order!

    bool		exitflag_;	//Set only from destructor
    bool		cancelflag_;	//Cancel current work and continue
    ::Threads::Work	task_;		
    CallBack		finishedcb_;
    int			queueid_;

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
    , exitflag_( false )
    , cancelflag_( false )
{
    spacefiller_[0] = 0; //to avoid warning of unused
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

	thread_->waitForFinish();
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
	while ( !task_.isOK() && !exitflag_ )
	    controlcond_.wait();

	if ( exitflag_ )
	{
	    controlcond_.unLock();
	    return;
	}

	while ( task_.isOK() && !exitflag_ )
	{
	    controlcond_.unLock(); //Allow someone to set the exitflag
	    bool retval = cancelflag_ ? false : task_.doRun();
	    controlcond_.lock();
	    retval_ = retval;

	    finishedcb_.doCall( this );
	    manager_.workloadcond_.lock();
	    bool isidle = false;

	    task_ = 0;

	    const int idx =
		manager_.reportFinishedAndAskForMore( this, queueid_ );
	    if ( idx==-1 )
	    {
		queueid_ = -1;
		finishedcb_ = CallBack(0,0);
		isidle = true;
	    }
	    else
	    {
		task_ = manager_.workload_[idx];
		manager_.workload_.removeSingle( idx );
		finishedcb_ = manager_.callbacks_[idx];
		manager_.callbacks_.removeSingle( idx );
		queueid_ = manager_.workqueueid_[idx];
		manager_.workqueueid_.removeSingle( idx );
	    }

	    manager_.workloadcond_.unLock();
	    if ( isidle )
		manager_.isidle.trigger( &manager_ );
	}
    }

    controlcond_.unLock();
}


void Threads::WorkThread::cancelWork( const ::Threads::Work* canceltask )
{
    Threads::MutexLocker lock( controlcond_ );
    if ( !canceltask || task_==(*canceltask) )
	cancelflag_ = true;
}


void Threads::WorkThread::exitWork(CallBacker*)
{
    controlcond_.lock();
    exitflag_ = true;
    controlcond_.signal( false );
    controlcond_.unLock();

    thread_->waitForFinish();
    delete thread_;
    thread_ = 0;
}


void Threads::WorkThread::assignTask(const ::Threads::Work& newtask,
				     const CallBack& cb, int queueid )
{
    controlcond_.lock();
    if ( task_.isOK() )
    {
	pErrMsg( "Trying to set existing task");
	::Threads::Work taskcopy = newtask;
	taskcopy.destroy();
	controlcond_.unLock();
	return;
    }

    task_ = newtask;
    finishedcb_ = cb;
    queueid_ = queueid;

    controlcond_.signal(false);
    controlcond_.unLock();
    return;
}


Threads::WorkManager::WorkManager( int nrthreads )
    : workloadcond_( *new ConditionVar )
    , isidle( this )
    , isShuttingDown( this )
    , freeid_( cDefaultQueueID() )
{
    addQueue( MultiThread, "Default queue" );

    if ( nrthreads == -1 )
	nrthreads = Threads::getNrProcessors();

    workloadcond_.lock();

    for ( int idx=0; idx<nrthreads; idx++ )
    {
	WorkThread* wt = new WorkThread( *this );
	threads_ += wt;
	threadids_ += wt->threadID();
	freethreads_ += wt;
    }
    
    workloadcond_.unLock();
}


Threads::WorkManager::~WorkManager()
{
    if ( this==thetwm && queueids_.size() )
    {
	pErrMsg("Default queue is not empty. "
	        "Please call twm().shutdown() before exiting main program,"
		"or exit with ExitProgram()");
    }

    shutdown();

    delete &workloadcond_;
}


void Threads::WorkManager::shutdown()
{
    isShuttingDown.trigger();

    if ( queueids_.size()>1 )
    {
	BufferString msg("All queues are not removed. Remaining queues: ");
	for ( int idx=1; idx<queueids_.size(); idx++ )
	{
	    if ( idx>1 )
		msg.add( ", " );
	    msg.add( queuenames_[idx] );
	}
	
	pErrMsg( msg.buf() );
    }

    while ( queueids_.size() )
	removeQueue( queueids_[0], false );

    deepErase( threads_ );
}


int Threads::WorkManager::addQueue( QueueType type, const char* nm )
{
    Threads::MutexLocker lock( workloadcond_ );

    const int id = freeid_;
    freeid_++;
    queueids_ += id;
    queuetypes_ += type;
    queueworkload_ += 0;
    queuenames_ += nm;
    queueisclosing_ += false;

    return id;
}


bool Threads::WorkManager::executeQueue( int queueid )
{
    Threads::MutexLocker lock( workloadcond_ );
    int queueidx = queueids_.indexOf( queueid );
    if ( queueidx==-1 )
	return false;

    if ( queuetypes_[queueidx]!=Manual )
    {
	pErrMsg("Only manual queues can be executed" );
	return false;
    }

    bool success = true;
    while ( true )
    {
	::Threads::Work task;
	CallBack cb(0,0);
	for ( int idx=0; idx<workload_.size(); idx++ )
	{
	    if ( workqueueid_[idx]==queueid )
	    {
		task = workload_[idx];
		workload_.removeSingle( idx );

		cb = callbacks_[idx];
		callbacks_.removeSingle( idx );
		workqueueid_.removeSingle( idx );
		break;
	    }
	}

	if ( !task.isOK() )
	    break;

	queueworkload_[queueidx]++;
	lock.unLock();

	if ( !task.doRun() )
	    success = false;

	cb.doCall( 0 );

	lock.lock();
	queueidx = queueids_.indexOf( queueid );
	if ( queueidx==-1 )
	    break;

	reduceWorkload( queueidx );
    }

    return success;
}


inline void Threads::WorkManager::reduceWorkload( int queueidx )
{
    if ( !queueworkload_.validIdx(queueidx) )
    {
	pErrMsg("Invalid index found" );
	workloadcond_.signal( true );
	return;
    }

    queueworkload_[queueidx]--;
    if ( !queueworkload_[queueidx] )
	workloadcond_.signal( true );
}


void Threads::WorkManager::emptyQueue( int queueid, bool finishall )
{
    Threads::MutexLocker lock(workloadcond_);
    int queueidx = queueids_.indexOf( queueid );

    if ( finishall )
    {
	//Wait for all threads to exit
	while ( queueworkload_[queueidx] || queueSizeNoLock( queueid ) )
	{
	    workloadcond_.wait();
	    queueidx = queueids_.indexOf( queueid );
	}
    }
    else
    {
	for ( int idx=workqueueid_.size()-1; idx>=0; idx-- )
	{
	    if ( workqueueid_[idx]==queueid )
	    {
		::Threads::Work& task = workload_[idx];
		task.destroy();
		workload_.removeSingle( idx );
		
		workqueueid_.removeSingle( idx );
		callbacks_.removeSingle( idx );
	    }
	}

	if ( queueisclosing_[queueidx] && !queueSizeNoLock(queueid) )
	    workloadcond_.signal( true );
    }
}


void Threads::WorkManager::removeQueue( int queueid, bool finishall )
{
    workloadcond_.lock();
    int queueidx = queueids_.indexOf( queueid );
    queueisclosing_[queueidx] = true;
    workloadcond_.unLock();

    emptyQueue( queueid, finishall );

    Threads::MutexLocker lock(workloadcond_);

    while ( queueworkload_[queueidx] )
    {
	workloadcond_.wait();
	queueidx = queueids_.indexOf( queueid );
    }

    queueidx = queueids_.indexOf( queueid );
    queueworkload_.removeSingle( queueidx );
    queuetypes_.removeSingle( queueidx );
    queueids_.removeSingle( queueidx );
    queueisclosing_.removeSingle( queueidx );
    queuenames_.removeSingle( queueidx );
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

#define mRunTask \
    ::Threads::Work taskcopy = newtask; \
    SimpleWorker sw; \
    sw.runWork( taskcopy, cb )

void Threads::WorkManager::addWork( const ::Threads::Work& newtask,
	CallBack* cb, int queueid, bool firstinline,
	bool ignoreduplicates )
       					  
{
    if ( queueid<0 )
	queueid = cDefaultQueueID();
    
    const int nrthreads = threads_.size();
    if ( !nrthreads )
    {
	mRunTask;
	return;
    }

    const CallBack thecb( cb ? *cb : CallBack(0,0) );

    Threads::MutexLocker lock(workloadcond_);
    int queueidx = queueids_.indexOf( queueid );
    if ( queueidx==-1 || queueisclosing_[queueidx] )
    {
	pErrMsg("Queue does not exist or is closing. Task rejected." );
	::Threads::Work taskcopy = newtask;
	taskcopy.destroy();
	return;
    }

    if ( ignoreduplicates && workload_.isPresent( newtask ) )
	return;
 
    const int nrfreethreads = freethreads_.size();
    if ( queuetypes_[queueidx]!=Manual )
    {
	if ( queuetypes_[queueidx]==MultiThread ||
	     !queueworkload_[queueidx] )
	{
	    if ( nrfreethreads )
	    {
		WorkThread* thread =
		    freethreads_.removeSingle( nrfreethreads-1 );
		queueworkload_[queueidx]++;

		lock.unLock();
		thread->assignTask( newtask, thecb, queueid );
		return;
	    }
	    else if ( isWorkThread() )
	    {
		queueworkload_[queueidx]++;
		lock.unLock();

		mRunTask;

		lock.lock();
		queueidx = queueids_.indexOf( queueid );
		queueworkload_[queueidx]--;
		return;
	    }
	}
    }

    if ( firstinline )
    {
	workqueueid_.insert( 0, queueid );
	workload_.insert( 0, newtask );
	callbacks_.insert( 0, thecb );
    }
    else
    {
	workqueueid_ += queueid;
	workload_ += newtask;
	callbacks_ += thecb;
    }
}


bool Threads::WorkManager::removeWork( const ::Threads::Work& task )
{
    workloadcond_.lock();

    const int idx = workload_.indexOf( task );
    if ( idx==-1 )
    {
	workloadcond_.unLock();
	return false;
    }

    workload_[idx].destroy();

    workqueueid_.removeSingle( idx );
    workload_.removeSingle( idx );
    callbacks_.removeSingle( idx );

    workloadcond_.unLock();
    return true;
}


const ::Threads::Work* Threads::WorkManager::getWork(CallBacker* cb) const
{
    if ( !cb ) return 0;

    //Why not go through workload?
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
			Threads::SimpleWorker* worker =
				    dynamic_cast<Threads::SimpleWorker*>( cb );
			rescond_.lock();
			if ( error_ || !worker->getRetVal() )
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


bool Threads::WorkManager::addWork( TypeSet<Threads::Work>& work,
				    int queueid, bool firstinline )
{
    return executeWork( work.arr(), work.size(), queueid, firstinline );
}


bool Threads::WorkManager::executeWork( Threads::Work* workload,
				    int workloadsize,
				    int queueid, bool firstinline )
{

    if ( !workloadsize )
	return true;

    const int nrthreads = threads_.size();
    bool res = true;
    if ( nrthreads==1 )
    {
	for ( int idx=0; idx<workloadsize; idx++ )
	{
	    if ( workload[idx].doRun() )
		continue;

	    res = false;
	    break;
	}
    }
    else
    {
	WorkResultManager resultman( workloadsize-1 );

	CallBack cb( mCB( &resultman, WorkResultManager, imFinished ));

	for ( int idx=1; idx<workloadsize; idx++ )
	    addWork( workload[idx], &cb, queueid, firstinline );

	res = workload[0].doRun();

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
    reduceWorkload( queueids_.indexOf(oldqueueid) );

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


int Threads::WorkManager::nrFreeThreads() const
{
    workloadcond_.lock();
    const int res = freethreads_.size();
    workloadcond_.unLock();
    return res;
}


bool Threads::WorkManager::isWorkThread() const
{
    return threadids_.isPresent( Threads::currentThread() );
}


void ::Threads::Work::destroy()
{
    if ( tf_ && takeover_ )
	delete obj_;

    obj_ = 0;
}


bool ::Threads::Work::operator==(const ::Threads::Work& t ) const
{
    return obj_==t.obj_ && cbf_==t.cbf_ && tf_==t.tf_ && stf_==t.stf_;
}
