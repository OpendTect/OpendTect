/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: threadwork.cc,v 1.3 2002-09-06 07:50:06 kristofer Exp $";

#include "threadwork.h"
#include "basictask.h"
#include "thread.h"
#include "errh.h"


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
}


Threads::WorkThread::~WorkThread()
{
    controlcond.lock();
    exitflag = true;
    controlcond.unlock();
    controlcond.signal(false);

    thread->stop();
    delete &controlcond;
}


void Threads::WorkThread::doWork( CallBacker* )
{
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
			: tasks( tasks_ )
			, results( tasks_.size(), 0 )
			, finished( tasks_.size(), false )
		    {}

    bool	    isFinished() const
		    {
			int nr = finished.size();
			for ( int idx=0; idx<nr; idx++ )
			    if ( !finished[idx] ) return false;
			return true;
		    }

    bool	    hasErrors() const
		    {
			int nr = finished.size();
			for ( int idx=0; idx<nr; idx++ )
			    if ( results[idx]<0 ) return true;
			return false;
		    }
				    

    void	    imFinished(CallBacker* cb )
		    {
			Threads::WorkThread* worker =
				    dynamic_cast<Threads::WorkThread*>( cb );
			BasicTask* task = worker->getTask();
			int idx = tasks.indexOf( task );

			rescond.lock();
			results[idx] = worker->getRetVal();
			finished[idx] = true;
			rescond.unlock();
			rescond.signal( true );
		    }

    Threads::ConditionVar	rescond;

protected:
    TypeSet<int>		results;
    BoolTypeSet 		finished;
    ObjectSet<BasicTask>&	tasks;

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
