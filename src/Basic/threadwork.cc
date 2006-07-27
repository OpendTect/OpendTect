/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: threadwork.cc,v 1.17 2006-07-27 13:46:05 cvskris Exp $";

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
    }

    delete &controlcond;
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
		manager.workloadcond_.lock();

		if ( manager.workload_.size() )
		{
		    task = manager.workload_[0];
		    finishedcb = manager.callbacks_[0];
		    manager.workload_.remove( 0 );
		    manager.callbacks_.remove( 0 );
		}
		else
		{
		    task = 0;
		    finishedcb = 0;
		    manager.freethreads_ += this;
		}

		manager.workloadcond_.unlock();
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
    : workloadcond_( *new ConditionVar )
    , isidle( this )
{
    callbacks_.allowNull(true);

    if ( nrthreads == -1 )
	nrthreads = Threads::getNrProcessors();

    for ( int idx=0; idx<nrthreads; idx++ )
    {
	WorkThread* wt = new WorkThread( *this );
	threads_ += wt;
	freethreads_ += wt;
    }
}


Threads::ThreadWorkManager::~ThreadWorkManager()
{
    deepErase( threads_ );
    deepErase( workload_ );
    delete &workloadcond_;
}


void Threads::ThreadWorkManager::addWork( BasicTask* newtask, CallBack* cb )
{
    const int nrthreads = threads_.size();
    if ( !nrthreads )
    {
	while ( true )
	{
	    int retval = newtask->doStep();
	    if ( retval>0 ) continue;

	    if ( cb ) cb->doCall( 0 );
	    return;
	}
    }

    Threads::MutexLocker lock(workloadcond_);

    const int nrfreethreads = freethreads_.size();
    if ( nrfreethreads )
    {
	freethreads_[nrfreethreads-1]->assignTask( *newtask, cb );
	freethreads_.remove( nrfreethreads-1 );
	return;
    }

    workload_ += newtask;
    callbacks_ += cb;
}


void Threads::ThreadWorkManager::removeWork( const BasicTask* task )
{
    workloadcond_.lock();

    const int idx = workload_.indexOf( task );
    if ( idx==-1 )
    {
	workloadcond_.unlock();
	for ( int idy=0; idy<threads_.size(); idy++ )
	    threads_[idy]->cancelWork( task );
	return;
    }

    workload_.remove( idx );
    callbacks_.remove( idx );
    workloadcond_.unlock();
}

class ThreadWorkResultManager : public CallBacker
{
public:
		    ThreadWorkResultManager( int nrtasks )
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
			rescond_.unlock();
		    }

    Threads::ConditionVar	rescond_;

protected:
    int				nrtasks_;
    int				nrfinished_;
    bool			error_;
};


bool Threads::ThreadWorkManager::addWork( ObjectSet<BasicTask>& work )
{
    if ( !work.size() )
	return true;

    const int nrwork = work.size();
    ThreadWorkResultManager resultman( nrwork-1 );
    resultman.rescond_.lock();

    CallBack cb( mCB( &resultman, ThreadWorkResultManager, imFinished ));

    for ( int idx=1; idx<nrwork; idx++ )
	addWork( work[idx], &cb );

    bool res = true;
    while ( true )
    {
	int retval = work[0]->doStep();
	if ( retval>0 ) continue;

	if ( retval<0 )
	    res = false;
	break;
    }

    while ( !resultman.isFinished() )
	resultman.rescond_.wait();

    resultman.rescond_.unlock();

    return res && !resultman.hasErrors();
}
