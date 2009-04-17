/*
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id: thread.cc,v 1.39 2009-04-17 11:53:44 cvsbert Exp $";

#include "thread.h"
#include "callback.h"
#include "debug.h"
#include "settings.h"
#include "debugmasks.h"
#include "debug.h"
#include "envvars.h"
#include "errh.h"
#include "errno.h" // for EBUSY

#ifdef __msvc__
# include "windows.h"
#endif


Threads::Mutex::Mutex( bool deadlockdetection )
{
    pthread_mutexattr_init( &attr_ );
    if ( deadlockdetection )
	pthread_mutexattr_settype( &attr_, PTHREAD_MUTEX_ERRORCHECK );

    pthread_mutex_init( &mutex_, &attr_ );
}


//!Implemented since standard copy constuctor will hang the system
Threads::Mutex::Mutex( const Mutex& m )
{
    pthread_mutexattr_init( &attr_ );
    pthread_mutex_init( &mutex_, &attr_ );
}


Threads::Mutex::~Mutex()
{
    pthread_mutex_destroy( &mutex_ );
    pthread_mutexattr_destroy( &attr_ );
}


int Threads::Mutex::lock()
{ 
    const int res = pthread_mutex_lock( &mutex_ ); 
    if ( res==EDEADLK )
    {
	pErrMsg( "Deadlock detected" );
	DBG::forceCrash(true);
    }

    return res;
}


int Threads::Mutex::unLock()
{
    return pthread_mutex_unlock( &mutex_ );
}


bool Threads::Mutex::tryLock()
{
    return pthread_mutex_trylock( &mutex_ ) != EBUSY;
}


Threads::MutexLocker::MutexLocker( Mutex& mutex, bool wait )
    : mutex_( mutex )
    , islocked_( true )
{
    if ( wait ) mutex_.lock();
    else islocked_ = mutex_.tryLock();
}


Threads::MutexLocker::~MutexLocker()
{
    if ( islocked_ ) mutex_.unLock();
}


void Threads::MutexLocker::unLock()
{ islocked_ = false; mutex_.unLock(); }


void Threads::MutexLocker::lock()
{ islocked_ = true; mutex_.lock(); }


bool Threads::MutexLocker::isLocked() const
{ return islocked_; }


#define mUnLocked	0
#define mPermissive	-1
#define mWriteLocked	-2


Threads::ReadWriteLock::ReadWriteLock()
    : status_( 0 )
    , nrreaders_( 0 )
{}


//!Implemented since standard copy constuctor will hang the system
Threads::ReadWriteLock::ReadWriteLock( const ReadWriteLock& )
    : status_( 0 )
{}


Threads::ReadWriteLock::~ReadWriteLock()
{}


void Threads::ReadWriteLock::readLock()
{
    statuscond_.lock();
    while ( status_==mWriteLocked )
	statuscond_.wait();

    nrreaders_++; 
    statuscond_.unLock();
}


void Threads::ReadWriteLock::readUnLock()
{
    Threads::MutexLocker lock( statuscond_ );
    if ( status_==mWriteLocked )
	pErrMsg( "Object is not readlocked.");
    else
	nrreaders_--;

    if ( !nrreaders_ )
    {
	status_ = mUnLocked;
	statuscond_.signal( false );
    }
}


void Threads::ReadWriteLock::writeLock()
{
    Threads::MutexLocker lock( statuscond_ );
    while ( status_!=mUnLocked || nrreaders_ )
	statuscond_.wait();

    status_ = mWriteLocked;
}
    
   
void Threads::ReadWriteLock::writeUnLock()
{
    Threads::MutexLocker lock( statuscond_ );
    if ( status_!=mWriteLocked )
    {
	pErrMsg( "Object is not writelocked.");
    }
    else
    {
	status_ = mUnLocked;
    }

    statuscond_.signal( true );
}


bool Threads::ReadWriteLock::convReadToWriteLock()
{
    Threads::MutexLocker lock( statuscond_ );
    if ( status_==mUnLocked && nrreaders_==1 )
    {
	status_ = mWriteLocked;
	nrreaders_ = 0;
	return true;
    }
    else if ( status_==mWriteLocked )
	pErrMsg( "Object is not readlocked.");

    lock.unLock();

    readUnLock();
    writeLock();
    return false;
}


void Threads::ReadWriteLock::convWriteToReadLock()
{
    Threads::MutexLocker lock( statuscond_ );
    if ( status_!=mWriteLocked )
	pErrMsg( "Object is not writelocked.");
    status_ = mUnLocked;
    nrreaders_ = 1;
    statuscond_.signal( true );
}


void Threads::ReadWriteLock::permissiveWriteLock()
{
    Threads::MutexLocker lock( statuscond_ );
    while ( status_ )
	statuscond_.wait();

    status_ = mPermissive;
}


void Threads::ReadWriteLock::permissiveWriteUnLock()
{
    Threads::MutexLocker lock( statuscond_ );
    if ( status_!=mPermissive )
    {
	pErrMsg("Wrong lock");
    }
    else
    {
	status_ = mUnLocked;
    }
}


void Threads::ReadWriteLock::convPermissiveToWriteLock()
{
    Threads::MutexLocker lock( statuscond_ );
    if ( status_!=mPermissive )
    {
	pErrMsg("Wrong lock");
    }
    else
    {
	while ( nrreaders_ )
	    statuscond_.wait();

	status_ = mWriteLocked;
    }
}


void Threads::ReadWriteLock::convWriteToPermissive()
{
    Threads::MutexLocker lock( statuscond_ );
    if ( status_!=mWriteLocked )
    {
	pErrMsg("Wrong lock");
    }
    else
    {
	status_ = mPermissive;
	statuscond_.signal( true );
    }
}


Threads::ConditionVar::ConditionVar()
{
    pthread_condattr_init( &condattr_ );
    pthread_cond_init( &cond_, &condattr_ );
}


//!Implemented since standard copy constuctor will hang the system
Threads::ConditionVar::ConditionVar( const ConditionVar& )
{
    pthread_condattr_init( &condattr_ );
    pthread_cond_init( &cond_, &condattr_ );
}


Threads::ConditionVar::~ConditionVar()
{
    pthread_cond_destroy( &cond_ );
    pthread_condattr_destroy( &condattr_ );
}


int Threads::ConditionVar::wait()
{
    return pthread_cond_wait( &cond_, &mutex_ );
}


int Threads::ConditionVar::signal(bool all)
{
    return all 	? pthread_cond_broadcast( &cond_ )
		: pthread_cond_signal( &cond_ );
}


Threads::Thread::Thread( void (func)(void*) )
    	: id_(0)
{
    pthread_create( &id_, 0, (void* (*)(void*)) func, 0 );
}


static void* thread_exec_fn( void* obj )
{
    CallBack* cbptr = reinterpret_cast<CallBack*>( obj );
    cbptr->doCall( 0 );
    return 0;
}


Threads::Thread::Thread( const CallBack& cbin )
    	: id_(0)
    	, cb(cbin)
{
    if ( !cb.willCall() ) return;
    pthread_create( &id_, 0, thread_exec_fn, (void*)(&cb) );
}


void Threads::Thread::stop()
{
    pthread_join( id_, 0 );
    delete this;
}


void Threads::Thread::detach()
{
    pthread_detach( id_ );
}


void Threads::Thread::threadExit()
{
    pthread_exit( 0 );
}


#ifndef __win__

#include <unistd.h>

#ifdef mac
# include <mach/mach.h>
# include <mach/mach_host.h>
# include <mach/host_info.h>
# include <mach/machine.h>
#endif

#endif


static int getSysNrProc()
{
    int ret;

#ifdef __win__

    struct _SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    ret = sysinfo.dwNumberOfProcessors; // total number of CPUs

#else

    int maxnrproc = sysconf(_SC_CHILD_MAX);

// also see: www.ks.uiuc.edu/Research/vmd/doxygen/VMDThreads_8C-source.html
# ifdef mac

    host_basic_info_data_t hostInfo;
    mach_msg_type_number_t infoCount;

    infoCount = HOST_BASIC_INFO_COUNT;
    host_info( mach_host_self(), HOST_BASIC_INFO, 
		(host_info_t)&hostInfo, &infoCount);

    int nrprocessors = hostInfo.avail_cpus;

# else

    int nrprocessors = sysconf(_SC_NPROCESSORS_ONLN);

# endif

    if ( maxnrproc == -1 && nrprocessors == -1 )
	ret = 2;
    else if ( maxnrproc == -1 )
	ret = nrprocessors;
    else if ( nrprocessors == -1 )
	ret = maxnrproc;
    else
	ret = mMIN(nrprocessors,maxnrproc);

#endif

    return ret;
}


int Threads::getNrProcessors()
{
    static int nrproc = -1;
    if ( nrproc > 0 ) return nrproc;

    nrproc = 0;

    bool havesett = false;
    if ( !GetEnvVarYN("OD_NO_MULTIPROC") )
    {
	havesett = Settings::common().get( "Nr Processors", nrproc );
	if ( !havesett )
	    nrproc = getSysNrProc();
    }

    if ( DBG::isOn( DBG_MT ) ) 
    {
	BufferString msg;
	if ( nrproc == 0 )
	    msg = "OD_NO_MULTIPROC set: one processor used";
	else
	{
	    msg = "Number of processors (";
	    msg += havesett ? "User settings" : "System";
	    msg += "): "; msg += nrproc;
	}
	DBG::message( msg );
    }

    if ( nrproc < 1 ) nrproc = 1;
    return nrproc;
}
