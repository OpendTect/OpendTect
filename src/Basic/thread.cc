/*
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id: thread.cc,v 1.14 2003-06-10 13:50:19 arend Exp $";

#include "thread.h"
#include "callback.h"
#include "errno.h"

#include "debug.h"
#include "debugmasks.h"

#include "settings.h"		// Only for getNrProcessors

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


bool Threads::Mutex::tryLock()
{
#ifdef __pthread__
    return pthread_mutex_trylock( &mutex ) != EBUSY;
#endif
    return false;
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


Threads::Thread::Thread( void (func)(void*) )
    	: id(0)
{
#ifdef __pthread__
    pthread_create( &id, 0, (void* (*)(void*)) func, 0 );
#endif
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
#ifdef __pthread__
    pthread_create( &id, 0, thread_exec_fn, (void*)(&cb) );
#endif
}


void Threads::Thread::stop()
{
#ifdef __pthread__
    pthread_join( id, 0 );
    delete this;
#endif
}


void Threads::Thread::detach()
{
#ifdef __pthread__
    pthread_detach( id );
#endif
}


void Threads::Thread::threadExit()
{
#ifdef __pthread__
    pthread_exit( 0 );
#endif
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
