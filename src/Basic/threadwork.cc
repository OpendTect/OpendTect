/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: threadwork.cc,v 1.9 2002-12-30 12:00:39 kristofer Exp $";

#include "threadwork.h"
#include "basictask.h"
#include "thread.h"
#include "errh.h"
#include "sighndl.h"
#include <signal.h>


namespace Threads
{

/*!\brief
is the worker that actually does the job and is the link between the manager
and the tasks to be performed.
*/

class WorkThread : public CallBacker
{
public:
    enum		Status { Idle, Running, Finished, Stopped };

    			WorkThread( ThreadWorkManager& );
    			~WorkThread();

    			//Interface from manager
    bool		assignTask(BasicTask*, CallBack* cb = 0);
    			/*!< becomes mine */

    Status		getStatus();
    int			getRetVal();
    BasicTask*		getTask();
    void		cancelWork( const BasicTask* );
    			//!< If working on this task, cancel it and continue.
    			//!< If a nullpointer is given, it will cancel
    			//!< regardless of which task we are working on

protected:

    void		doWork(CallBacker*);
    void 		exitWork(CallBacker*);
    ThreadWorkManager&	manager;

    ConditionVar&	controlcond;	//Dont change this order!
    Status		status;		//These are protected by the condvar
    int			retval;		//Lock before reading or writing

    bool		exitflag;	//Set only from destructor
    bool		cancelflag;	//Cancel current work and continue
    BasicTask*		task;		
    CallBack*		cb;

    Thread*		thread;
};

}; // Namespace


Threads::WorkThread::WorkThread( ThreadWorkManager& man )
    : manager( man )
    , controlcond( *new Threads::ConditionVar )
    , thread( 0 )
    , exitflag( false )
    , cancelflag( false )
    , task( 0 )
    , status( Idle )
{
    controlcond.lock();
    thread = new Thread( mCB( this, WorkThread, doWork));
    controlcond.unlock();

    SignalHandling::startNotify( SignalHandling::Kill,
	    			 mCB( this, WorkThread, exitWork ));
}


Threads::WorkThread::~WorkThread()
{
    SignalHandling::stopNotify( SignalHandling::Kill,
				 mCB( this, WorkThread, exitWork ));

    if ( thread )
    {
	controlcond.lock();
	exitflag = true;
	controlcond.signal(false);
	controlcond.unlock();

	thread->stop();
	thread = 0;
	delete &controlcond;
    }
}


void Threads::WorkThread::doWork( CallBacker* )
{
    sigset_t newset;
    sigemptyset(&newset);
    sigaddset(&newset,SIGINT);
    sigaddset(&newset,SIGQUIT);
    sigaddset(&newset,SIGKILL);
    pthread_sigmask(SIG_BLOCK, &newset, 0 );

    while ( true )
    {
	controlcond.lock();
	while ( !task && !exitflag && !cancelflag )
	{
	    status = Idle;
	    controlcond.wait();
	}

	if ( exitflag )
	{
	    status = Stopped;
	    controlcond.unlock();
	    thread->threadExit();
	}

	if ( task )
	{
	    status = Running;
	    if ( cancelflag )
	    {
		retval = 0;
	    }
	    else
	    {
		controlcond.unlock();
		int rval = task->doStep();
		controlcond.lock();
		retval =rval;
	    }

	    if ( retval<1 )
	    {
		status = Finished;

		controlcond.unlock();
		if ( cb ) cb->doCall( this );
		controlcond.lock();
		task = 0;
		status = Idle;
		controlcond.unlock();
		manager.imFinished( this );
	    }
	    else
		controlcond.unlock();
	}
	else
	    controlcond.unlock();
    }
}


void Threads::WorkThread::cancelWork( const BasicTask* canceltask )
{
    controlcond.lock();
    if ( !canceltask || canceltask==task )
    {
	cancelflag = true;
	controlcond.unlock();
	controlcond.signal( false );
	return;
    }

    controlcond.unlock();
}


void Threads::WorkThread::exitWork(CallBacker*)
{
    controlcond.lock();
    exitflag = true;
    controlcond.signal( false );
    controlcond.unlock();

    thread->stop();
    thread = 0;
}


bool Threads::WorkThread::assignTask(BasicTask* newtask, CallBack* cb_ )
{
    controlcond.lock();
    if ( task )
    {
	controlcond.unlock();
	return false;
    }

    task = newtask;
    cb = cb_;
    controlcond.signal(false);
    controlcond.unlock();
    return true;
}


Threads::WorkThread::Status Threads::WorkThread::getStatus()
{
    Threads::MutexLocker locker( controlcond );
    return status;
}


int Threads::WorkThread::getRetVal()
{
    Threads::MutexLocker locker( controlcond );
    return retval;
}


BasicTask* Threads::WorkThread::getTask()
{
    Threads::MutexLocker locker( controlcond );
    return task;
}


Threads::ThreadWorkManager::ThreadWorkManager( int nrthreads )
    : workloadcond( *new ConditionVar )
    , isidle( this )
{
    if ( !Threads::isThreadsImplemented() ) return;
    callbacks.allowNull(true);

    for ( int idx=0; idx<nrthreads; idx++ )
	threads += new WorkThread( *this );
}


Threads::ThreadWorkManager::~ThreadWorkManager()
{
    deepErase( threads );
    deepErase( workload );
    delete &workloadcond;
}


void Threads::ThreadWorkManager::addWork( BasicTask* newtask, CallBack* cb )
{
    const int nrthreads = threads.size();
    if ( !nrthreads )
    {
	while ( true )
	{
	    int retval = newtask->doStep();
	    if ( retval<0 ) continue;

	    if ( cb ) cb->doCall( 0 );
	    return;
	}
    }

    Threads::MutexLocker lock(workloadcond);

    for ( int idx=0; idx<nrthreads; idx++ )
    {
	if ( threads[idx]->assignTask( newtask, cb ) ) 
	    return;
    }

    workload += newtask;
    callbacks += cb;
}


void Threads::ThreadWorkManager::removeWork( const BasicTask* task )
{
    workloadcond.lock();

    const int idx = workload.indexOf( task );
    if ( idx==-1 )
    {
	workloadcond.unlock();
	for ( int idy=0; idy<threads.size(); idy++ )
	    threads[idy]->cancelWork( task );
	return;
    }

    workload.remove( idx );
    callbacks.remove( idx );
    workloadcond.unlock();
}



class ThreadWorkResultManager : public CallBacker
{
public:
		    ThreadWorkResultManager( ObjectSet<BasicTask>&  tasks_ )
			: nrtasks( tasks_.size() )
			, nrfinished( 0 )
			, error( false )
		    {}

    bool	    isFinished() const
		    { return nrfinished==nrtasks; }

    bool	    hasErrors() const
		    { return error; }

    void	    imFinished(CallBacker* cb )
		    {
			Threads::WorkThread* worker =
				    dynamic_cast<Threads::WorkThread*>( cb );
			rescond.lock();
			if ( error || worker->getRetVal()==-1 )
			    error = true;

			nrfinished++;
			bool isfin = nrfinished==nrtasks;
			if ( isfin ) rescond.signal( false );
			rescond.unlock();
		    }

    Threads::ConditionVar	rescond;

protected:
    int				nrtasks;
    int				nrfinished;
    bool			error;
};


bool Threads::ThreadWorkManager::addWork( ObjectSet<BasicTask>& work )
{
    ThreadWorkResultManager resultman( work );
    resultman.rescond.lock();

    const int nrwork = work.size();
    CallBack cb( mCB( &resultman, ThreadWorkResultManager, imFinished ));

    for ( int idx=0; idx<nrwork; idx++ )
	addWork( work[idx], &cb );

    while ( !resultman.isFinished() )
	resultman.rescond.wait();

    resultman.rescond.unlock();

    return !resultman.hasErrors();
}


void Threads::ThreadWorkManager::imFinished( WorkThread* workthread )
{
    Threads::MutexLocker lock(workloadcond);

    if ( workload.size() )
    {
	workthread->assignTask( workload[0], callbacks[0] );
	workload.remove( 0 );
	callbacks.remove( 0 );
    }
    else
	isidle.trigger(this);
}
