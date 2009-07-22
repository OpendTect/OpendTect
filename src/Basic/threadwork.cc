/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: threadwork.cc,v 1.26 2009-07-22 16:01:31 cvsbert Exp $";

#include "threadwork.h"
#include "task.h"
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
    void		assignTask(SequentialTask&, CallBack* finishedcb = 0);

    int			getRetVal();
    			/*!< Do only call when task is finished,
			     i.e. from the cb or
			     Threads::ThreadWorkManager::imFinished()
			*/

    void		cancelWork( const SequentialTask* );
    			//!< If working on this task, cancel it and continue.
    			//!< If a nullpointer is given, it will cancel
    			//!< regardless of which task we are working on
    const SequentialTask* getTask() const { return task_; }

protected:

    void		doWork(CallBacker*);
    void 		exitWork(CallBacker*);

    ThreadWorkManager&	manager_;

    ConditionVar&	controlcond_;	//Dont change this order!
    int			retval_;	//Lock before reading or writing

    bool		exitflag_;	//Set only from destructor
    bool		cancelflag_;	//Cancel current work and continue
    SequentialTask*	task_;		
    CallBack*		finishedcb_;

    Thread*		thread_;

private:
    long		spacefiller_[24];
};

}; // Namespace


Threads::WorkThread::WorkThread( ThreadWorkManager& man )
    : manager_( man )
    , controlcond_( *new Threads::ConditionVar )
    , thread_( 0 )
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

		if ( manager_.workload_.size() )
		{
		    task_ = manager_.workload_[0];
		    finishedcb_ = manager_.callbacks_[0];
		    manager_.workload_.remove( 0 );
		    manager_.callbacks_.remove( 0 );
		}
		else
		{
		    task_ = 0;
		    finishedcb_ = 0;
		    manager_.freethreads_ += this;
		}

		manager_.workloadcond_.unLock();
		manager_.isidle.trigger(&manager_);
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


void Threads::WorkThread::assignTask(SequentialTask& newtask, CallBack* cb_ )
{
    controlcond_.lock();
    if ( task_ )
    {
	pErrMsg( "Trying to set existing task");
	controlcond_.unLock();
	return;
    }

    task_ = &newtask;
    finishedcb_ = cb_;
    controlcond_.signal(false);
    controlcond_.unLock();
    return;
}


int Threads::WorkThread::getRetVal()
{
    return retval_;
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


void Threads::ThreadWorkManager::addWork( SequentialTask* newtask, CallBack* cb)
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


bool Threads::ThreadWorkManager::removeWork( const SequentialTask* task )
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

    workload_.remove( idx );
    callbacks_.remove( idx );
    workloadcond_.unLock();
    return true;
}


const SequentialTask* Threads::ThreadWorkManager::getWork(CallBacker* cb) const
{
    if ( !cb ) return 0;

    for ( int idx=0; idx<threads_.size(); idx++ )
    {
	if ( threads_[idx]==cb )
	    return threads_[idx]->getTask();
    }

    return 0;
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
			rescond_.unLock();
		    }

    Threads::ConditionVar	rescond_;

protected:
    int				nrtasks_;
    int				nrfinished_;
    bool			error_;
};


bool Threads::ThreadWorkManager::addWork( ObjectSet<SequentialTask>& work )
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
	ThreadWorkResultManager resultman( nrwork-1 );

	CallBack cb( mCB( &resultman, ThreadWorkResultManager, imFinished ));

	for ( int idx=1; idx<nrwork; idx++ )
	    addWork( work[idx], &cb );

	res = work[0]->execute();

	resultman.rescond_.lock();
	while ( !resultman.isFinished() )
	    resultman.rescond_.wait();
	resultman.rescond_.unLock();

	if ( res ) res = !resultman.hasErrors();
    }

    return res;
}
