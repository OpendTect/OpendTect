/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: threadwork.cc,v 1.5 2002-09-10 07:35:25 kristofer Exp $";

#include "threadwork.h"
#include "basictask.h"
#include "thread.h"
#include "errh.h"
#include "sighndl.h"
#include <signal.h>


Threads::WorkThread::WorkThread( ThreadWorkManager& man )
    : manager( man )
    , controlcond( *new Threads::ConditionVar )
    , thread( 0 )
    , exitflag( false )
    , task( 0 )
    , status( Idle )
{
    controlcond.lock();
    thread = new Thread( mCB( this, WorkThread, doWork));
    controlcond.unlock();

    SignalHandling::startNotify( SignalHandling::Kill,
	    			 mCB( this, WorkThread, cancelWork ));
}


Threads::WorkThread::~WorkThread()
{
    SignalHandling::stopNotify( SignalHandling::Kill,
				 mCB( this, WorkThread, cancelWork ));

    if ( thread )
    {
	controlcond.lock();
	exitflag = true;
	controlcond.unlock();
	controlcond.signal(false);

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
	while ( !task && !exitflag )
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
	    controlcond.unlock();
	    int rval = task->doStep();
	    controlcond.lock();
	    retval = rval;
	    if ( retval < 1 )
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
	}
	else
	    controlcond.unlock();
    }
}


void Threads::WorkThread::cancelWork(CallBacker*)
{
    controlcond.lock();
    exitflag = true;
    controlcond.unlock();
    controlcond.signal( false );

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
    controlcond.unlock();
    controlcond.signal(false);
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
{
    if ( !Threads::isThreadsImplemented() ) return;

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
			rescond.unlock();
			if ( isfin ) rescond.signal( false );
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
}
