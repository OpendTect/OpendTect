/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: threadwork.cc,v 1.1 2002-04-15 12:08:30 kristofer Exp $";

#include "threadwork.h"
#include "thread.h"
#include "errh.h"


Threads::WorkThread::WorkThread( ThreadWorkManager& man )
    : manager( man )
    , exitcond( *new Threads::ConditionVar )
    , thread( new Thread( threadfunc, (void*) this ))
    , exitflag( true )
    , task( 0 )
    , status( Idle )
{}


Threads::WorkThread::~WorkThread()
{
    exitcond.lock();
    exitflag = true;
    exitcond.unlock();
    exitcond.signal(false);

    thread->destroy(true, 0 );
    delete &exitcond;
}


void Threads::WorkThread::checkForExit()
{
    exitcond.lock();
    if ( exitflag )
    {
	status = Stopped;
	exitcond.unlock();
	thread->threadExit();
    }

    exitcond.unlock();
}


bool Threads::WorkThread::assignTask(ThreadTask* newtask)
{
    exitcond.lock();
    if ( task )
    {
	pErrMsg("Assigning new task, but old one is not completed" );
	exitcond.unlock();
	return false;
    }

    task = newtask;
    exitcond.unlock();
    exitcond.signal(false);
    return true;
}


Threads::WorkThread::Status Threads::WorkThread::getStatus()
{
    Threads::MutexLocker locker( exitcond );
    return status;
}


int Threads::WorkThread::getRetVal()
{
    Threads::MutexLocker locker( exitcond );
    return retval;
}


void Threads::WorkThread::threadfunc( void* data )
{
    ((Threads::WorkThread*) data)->threadFunc();
}


void Threads::WorkThread::threadFunc()
{
    while ( true )
    {
	exitcond.lock();

	while ( !exitflag && !task ) exitcond.wait();

	if ( exitflag )
	{
	    exitcond.unlock();
	    thread->threadExit();
	}
	else
	{
	    status = Running;
	    exitcond.unlock();

	    retval = task->run( *this );

	    exitcond.lock();
	    status = Finished;
	    manager.imFinished( this );
	    delete task; task = 0;
	    status = Idle;
	    exitcond.unlock();
	}
    }
}


Threads::ThreadWorkManager::ThreadWorkManager( int nrthreads )
    : workloadcond( *new ConditionVar )
{
    for ( int idx=0; idx<nrthreads; idx++ )
	threads += new WorkThread( *this );
}


Threads::ThreadWorkManager::~ThreadWorkManager()
{
    deepErase( threads );
    deepErase( workload );
    delete &workloadcond;
}


void Threads::ThreadWorkManager::addWork( ThreadTask* newtask )
{
    Threads::MutexLocker lock(workloadcond);

    for ( int idx=0; idx<threads.size(); idx++ )
    {
	if ( threads[idx]->getStatus()== WorkThread::Idle )
	{
	    threads[idx]->assignTask( newtask );
	    return;
	}
    }

    workload += newtask;
}


void Threads::ThreadWorkManager::imFinished( WorkThread* workthread )
{
    Threads::MutexLocker lock(workloadcond);

    if ( workload.size() )
    {
	workthread->assignTask( workload[0] );
	workload.remove( 0 );
    }
}
