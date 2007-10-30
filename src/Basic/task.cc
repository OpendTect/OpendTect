/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
-*/

static const char* rcsID = "$Id: task.cc,v 1.1 2007-10-30 16:53:35 cvskris Exp $";

#include "task.h"

#include "threadwork.h"
#include "thread.h"
#include "varlenarray.h"


bool SequentialTask::execute()
{
    while ( true )
    {
	int res = doStep();
	if ( !res )     return true;
	if ( res < 0 )  break;
    }

    return false;
}



class ParallelTaskRunner : public SequentialTask
{
public:
    		ParallelTaskRunner()
		    : task_( 0 )					{}

    void	set( ParallelTask* task, int start, int stop, int threadid )
    		{
		    task_ = task;
		    start_ = start;
		    stop_ = stop;
		    threadid_ = threadid;;
		}

    int		nextStep()
		{
		    return task_->doWork(start_,stop_,threadid_)
			? SequentialTask::cFinished()
			: SequentialTask::cErrorOccurred();
		}

protected:
    int			start_;
    int			stop_;
    int			threadid_;
    ParallelTask*	task_;
};


ParallelTask::ParallelTask()
    : nrdone_( -1 )
    , nrdonemutex_( 0 )
    , stopwork_( false )
{}


ParallelTask::~ParallelTask()
{
    delete nrdonemutex_;
}


void ParallelTask::enableNrDoneCounting( bool yn )
{
    const bool isenabled = nrdonemutex_;
    if ( isenabled==yn )
	return;

    if ( !yn )
    {
	nrdonemutex_->lock();
	Threads::Mutex* tmp = nrdonemutex_;
	nrdonemutex_ = 0;
	tmp->unlock();
    }
    else
    {
	nrdonemutex_ = new Threads::Mutex;
    }
}



void ParallelTask::reportNrDone( int nr )
{
    if ( !nrdonemutex_ )
	return;

    Threads::MutexLocker lock( *nrdonemutex_ );
    if ( nrdone_==-1 )
	nrdone_ = nr;
    else
	nrdone_ += nr;
}


int ParallelTask::nrDone() const
{
    if ( nrdonemutex_ ) nrdonemutex_->lock();
    int res = nrdone_;
    if ( nrdonemutex_ ) nrdonemutex_->unlock();
    return res;
}


Threads::ThreadWorkManager& ParallelTask::twm()
{
    static Threads::ThreadWorkManager twm_;
    return twm_;
}


bool ParallelTask::execute()
{
    if ( nrdonemutex_ ) nrdonemutex_->lock();
    nrdone_ = -1;
    stopwork_ = false;
    if ( nrdonemutex_ ) nrdonemutex_->unlock();

    if ( Threads::getNrProcessors()==1 )
    {
	if ( !doPrepare( 1 ) ) return false;
	return doFinish( doWork( 0, totalNr()-1, 0 ) );
    }

    const int nrthreads = twm().nrThreads();
    const int size = totalNr();
    if ( !size ) return true;

    mVariableLengthArr( ParallelTaskRunner, runners, nrthreads );

    int start = 0;
    ObjectSet<SequentialTask> tasks;
    for ( int idx=nrthreads-1; idx>=0; idx-- )
    {
	if ( start>=size )
	    break;

	const int threadsize = calculateThreadSize( size, nrthreads, idx );

	const int stop = start + threadsize-1;
	runners[idx].set( this, start, stop, idx );
	tasks += &runners[idx];
	start = stop+1;
    }

    if ( !doPrepare( tasks.size() ) )
	return false;

    const bool res = tasks.size()<2
	? doWork( 0, totalNr()-1, 0 ) : twm().addWork( tasks );

    if ( !doFinish( res ) )
	return false;

    return res;
}


int ParallelTask::calculateThreadSize( int totalnr, int nrthreads,
				       int idx ) const
{
    if ( !nrthreads==1 ) return totalnr;

    const int idealnrperthread = mNINT((float) totalnr/nrthreads);
    const int nrperthread = idealnrperthread<minThreadSize()
	?  minThreadSize()
	: idealnrperthread;

    const int start = idx*idealnrperthread;
    int nextstart = start + idealnrperthread;

    if ( nextstart>totalnr )
	nextstart = totalnr;

    if ( idx==nrthreads-1 )
	nextstart = totalnr;

    if ( nextstart<=start )
	return 0;

    return nextstart-start;
}

