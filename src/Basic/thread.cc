/*
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id: thread.cc,v 1.16 2004-01-08 10:57:11 bert Exp $";

#include "thread.h"
#include "callback.h"
#include "errno.h"

#include "debug.h"
#include "debugmasks.h"

#include "settings.h"		// Only for getNrProcessors

Threads::Mutex::Mutex()
{
    pthread_mutexattr_init( &attr );
    pthread_mutex_init( &mutex, &attr );
}

Threads::Mutex::~Mutex()
{
    pthread_mutex_destroy( &mutex );
    pthread_mutexattr_destroy( &attr );
}

int Threads::Mutex::lock()
{ 
    return pthread_mutex_lock( &mutex ); 
}


int Threads::Mutex::unlock()
{
    return pthread_mutex_unlock( &mutex );
}


bool Threads::Mutex::tryLock()
{
    return pthread_mutex_trylock( &mutex ) != EBUSY;
}


Threads::ConditionVar::ConditionVar()
{
    pthread_condattr_init( &condattr );
    pthread_cond_init( &cond, &condattr );
}

Threads::ConditionVar::~ConditionVar()
{
    pthread_cond_destroy( &cond );
    pthread_condattr_destroy( &condattr );
}


int Threads::ConditionVar::wait()
{
    return pthread_cond_wait( &cond, &mutex );
}


int Threads::ConditionVar::signal(bool all)
{
    return all 	? pthread_cond_broadcast( &cond )
		: pthread_cond_signal( &cond );
}


Threads::Thread::Thread( void (func)(void*) )
    	: id(0)
{
    pthread_create( &id, 0, (void* (*)(void*)) func, 0 );
}


static void* thread_exec_fn( void* obj )
{
    CallBack* cbptr = reinterpret_cast<CallBack*>( obj );
    cbptr->doCall( 0 );
    return 0;
}


Threads::Thread::Thread( const CallBack& cbin )
    	: id(0)
    	, cb(cbin)
{
    if ( !cb.willCall() ) return;
    pthread_create( &id, 0, thread_exec_fn, (void*)(&cb) );
}


void Threads::Thread::stop()
{
    pthread_join( id, 0 );
    delete this;
}


void Threads::Thread::detach()
{
    pthread_detach( id );
}


void Threads::Thread::threadExit()
{
    pthread_exit( 0 );
}

#define dbg_nr_proc() \
    if ( DBG::isOn( DBG_MT ) )  \
    { \
	BufferString msg( "Number of Processors found: " ); \
	msg += ret; \
	DBG::message( msg ); \
    }


#ifdef __win__
int Threads::getNrProcessors()
{
    int res = 1;

    if ( !Settings::common().get("Nr Processors", res ) )
    {
	struct _SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	res = sysinfo.dwNumberOfProcessors; /* total number of CPUs */
    }
    return res;

}
#else

#include <unistd.h>

int Threads::getNrProcessors()
{
    int res;
    if ( Settings::common().get("Nr Processors", res ) )
	return res;

    int maxnrproc = sysconf(_SC_CHILD_MAX);

// also see: www.ks.uiuc.edu/Research/vmd/doxygen/VMDThreads_8C-source.html
#ifdef sgi
    int nrprocessors = sysconf(_SC_NPROC_ONLN);
#else
    int nrprocessors = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    int ret;
    if ( maxnrproc==-1 && nrprocessors==-1 ) ret= 2;
    else if ( maxnrproc==-1 ) ret= nrprocessors;
    else if ( nrprocessors==-1 ) ret= maxnrproc;
    else  ret= mMIN(nrprocessors,maxnrproc);

    if ( DBG::isOn( DBG_MT ) ) 
    {
	BufferString msg( "Number of Processors found: " );
	msg += ret;
	DBG::message( msg );
    }

    return ret;
}

#endif
