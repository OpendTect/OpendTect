/*
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id";

#include <thread.h>
#include <basictask.h>

//
//! Platform dependent implementations

Threads::Mutex::Mutex()
{
#ifdef __pthread__
    pthread_mutexattr_init( &attr );
    pthread_mutex_init( &mutex, &attr );
#endif
}

Threads::Mutex::~Mutex()
{
#ifdef __pthread__
    pthread_mutex_destroy( &mutex );
    pthread_mutexattr_destroy( &attr );
#endif
}

int Threads::Mutex::lock()
{ 
#ifdef __pthread__
    return pthread_mutex_lock( &mutex ); 
#endif
    return 0;
}

int Threads::Mutex::unlock()
{
#ifdef __pthread__
    return pthread_mutex_unlock( &mutex );
#endif
    return 0;
}



Threads::ConditionVar::ConditionVar()
{
#ifdef __pthread__
    pthread_condattr_init( &condattr );
    pthread_cond_init( &cond, &condattr );
#endif
}

Threads::ConditionVar::~ConditionVar()
{
#ifdef __pthread__
    pthread_cond_destroy( &cond );
    pthread_condattr_destroy( &condattr );
#endif
}


int Threads::ConditionVar::wait()
{
#ifdef __pthread__
    return pthread_cond_wait( &cond, &mutex );
#endif
    return 0;
}


int Threads::ConditionVar::signal(bool all)
{
#ifdef __pthread__
    return all 	? pthread_cond_broadcast( &cond )
		: pthread_cond_signal( &cond );
#endif
    return 0;
}



int Threads::Thread::start()
{
   is_running = true;
#ifdef __pthread__
   return pthread_create( &id, 0, (void* (*)(void*)) func, this );
#endif
    return 0;
}


int Threads::Thread::join()
{
#ifdef __pthread__
    return pthread_join( id, &ret_val );
#endif
    return 0;
}


void Threads::Thread::threadExit( void* rv )
{
#ifdef __pthread__
    pthread_exit( rv );
#endif
}


//
//! Platform independent implementations

Threads::Mutex::Locker::Locker( Threads::Mutex& mutex_ )
    : mutex( mutex_ )
{ mutex.lock(); }


Threads::Mutex::Locker::~Locker()
{ mutex.unlock(); }


int Threads::ConditionVar::unlock( bool all )
{
    unlock();
    return signal( all );
}


int Threads::ConditionVar::unlock()
{ return Mutex::unlock(); }


Threads::Thread::Thread()
    : func( 0 )
    , exitflag( false )
    , ret_val( 0 )
    , is_running( false )
{}


Threads::Thread::~Thread()
{ if ( is_running ) stop(); }


bool Threads::Thread::setFunction( void (nf)(void*)  )
{
    if ( func ) return false;
    func = nf;

    return true;
}


void Threads::Thread::stop()
{
    exitcond.lock();
    exitflag=true;
    exitcond.unlock(false);

    join();

    is_running = false;
}



Threads::TaskRunner::TaskRunner()
{
    data.exitcond.lock();
    data.stat = Idle;
    data.stopflag = true;
    data.exitcond.unlock();
    data.task=0;

    data.setFunction( Threads::TaskRunner::threadFunc );
    data.start();
}


Threads::TaskRunner::~TaskRunner()
{ }


Threads::TaskRunner::Status Threads::TaskRunner::status() const
{ return data.stat; }


bool Threads::TaskRunner::setTask( BasicTask* nt )
{
    if ( data.stat!=Idle ) return false;

    Threads::Mutex::Locker locker( data.exitcond );
    data.task = nt;
    data.lastval = 0;
    data.stopflag = true;

    return true;
}


bool Threads::TaskRunner::start()
{
    if ( data.stat==Running ) return false;

    Threads::Mutex::Locker locker( data.exitcond );
    data.stat=Running;
    data.stopflag = false;

    return true;
}


bool Threads::TaskRunner::stop()
{
    if ( data.stat!=Running ) return false;

    Threads::Mutex::Locker locker( data.exitcond );
    data.stopflag = true;
    data.stat=Stopped;

    return true;
}


void Threads::TaskRunner::threadFunc(void *ptr)
{
    Threads::TaskRunner::Data* data = (Threads::TaskRunner::Data*) ptr;

    while ( true )
    {
	data->exitcond.lock();

	while ( !data->exitflag && data->stopflag ) data->exitcond.wait();

	if ( data->exitflag )
	{
	    data->exitcond.unlock();
	    data->threadExit();
	}
	else if ( !data->stopflag )
	{
	    data->exitcond.unlock();

	    data->lastval = data->task->nextStep();

	    if ( data->lastval>0 )
		continue;

	    Threads::Mutex::Locker locker( data->exitcond );
	    data->stopflag = true;
	    data->stat = Finished;
	}

    }
};
