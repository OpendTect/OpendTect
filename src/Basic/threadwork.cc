/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: threadwork.cc,v 1.11 2003-03-06 11:18:56 kristofer Exp $";

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
    			WorkThread( ThreadWorkManager& );
    			~WorkThread();

    			//Interface from manager
    void		assignTask(BasicTask&, CallBack* finishedcb = 0);

    int			getRetVal();
    			/*!< Do only call when task is finished,
			     i.e. from the cb or
			     Threads::ThreadWorkManager::imFinished()
			*/

    void		cancelWork( const BasicTask* );
    			//!< If working on this task, cancel it and continue.
    			//!< If a nullpointer is given, it will cancel
    			//!< regardless of which task we are working on

protected:

    void		doWork(CallBacker*);
    void 		exitWork(CallBacker*);
    ThreadWorkManager&	manager;

    ConditionVar&	controlcond;	//Dont change this order!
    int			retval;		//Lock before reading or writing

    bool		exitflag;	//Set only from destructor
    bool		cancelflag;	//Cancel current work and continue
    BasicTask*		task;		
    CallBack*		finishedcb;

    Thread*		thread;

private:
    long		_spacefiller[24];
};

}; // Namespace


Threads::WorkThread::WorkThread( ThreadWorkManager& man )
    : manager( man )
    , controlcond( *new Threads::ConditionVar )
    , thread( 0 )
    , exitflag( false )
    , cancelflag( false )
    , task( 0 )
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

    controlcond.lock();
    while ( true )
    {
	while ( !task && !exitflag )
	    controlcond.wait();

	if ( exitflag )
	{
	    controlcond.unlock();
	    thread->threadExit();
	}

	while ( task && !exitflag )
	{
	    controlcond.unlock(); //Allow someone to set the exitflag
	    retval = cancelflag ? 0 : task->doStep();
	    controlcond.lock();

	    if ( retval<1 )
	    {
		if ( finishedcb ) finishedcb->doCall( this );
		manager.workloadcond.lock();

		if ( manager.workload.size() )
		{
		    task = manager.workload[0];
		    finishedcb = manager.callbacks[0];
		    manager.workload.remove( 0 );
		    manager.callbacks.remove( 0 );
		}
		else
		{
		    task = 0;
		    finishedcb = 0;
		    manager.freethreads += this;
		}

		manager.workloadcond.unlock();
		manager.isidle.trigger(&manager);
	    }
	}
    }

    controlcond.unlock();
}


void Threads::WorkThread::cancelWork( const BasicTask* canceltask )
{
    Threads::MutexLocker lock( controlcond );
    if ( !canceltask || canceltask==task )
	cancelflag = true;
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


void Threads::WorkThread::assignTask(BasicTask& newtask, CallBack* cb_ )
{
    controlcond.lock();
    if ( task )
    {
	pErrMsg( "Trying to set existing task");
	controlcond.unlock();
	return;
    }

    task = &newtask;
    finishedcb = cb_;
    controlcond.signal(false);
    controlcond.unlock();
    return;
}


int Threads::WorkThread::getRetVal()
{
    return retval;
}


Threads::ThreadWorkManager::ThreadWorkManager( int nrthreads )
    : workloadcond( *new ConditionVar )
    , isidle( this )
{
    if ( !Threads::isThreadsImplemented() ) return;
    callbacks.allowNull(true);

    for ( int idx=0; idx<nrthreads; idx++ )
    {
	WorkThread* wt = new WorkThread( *this );
	threads += wt;
	freethreads += wt;
    }
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

    const int nrfreethreads = freethreads.size();
    if ( nrfreethreads )
    {
	freethreads[nrfreethreads-1]->assignTask( *newtask, cb );
	freethreads.remove( nrfreethreads-1 );
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
			if ( nrfinished==nrtasks ) rescond.signal( false );
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
