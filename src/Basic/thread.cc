/*
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id: thread.cc,v 1.34 2008-06-24 18:49:28 cvskris Exp $";

#include "thread.h"
#include "callback.h"
#include "settings.h"
#include "debugmasks.h"
#include "debug.h"
#include "envvars.h"
#include "errh.h"
#include "errno.h" // for EBUSY

#ifdef __msvc__
# include "windows.h"
#endif


Threads::Mutex::Mutex()
{
    pthread_mutexattr_init( &attr );
    pthread_mutex_init( &mutex, &attr );
}


//!Implemented since standard copy constuctor will hang the system
Threads::Mutex::Mutex( const Mutex& )
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


int Threads::Mutex::unLock()
{
    return pthread_mutex_unlock( &mutex );
}


bool Threads::Mutex::tryLock()
{
    return pthread_mutex_trylock( &mutex ) != EBUSY;
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
    , nrreaders_( 0 )
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
    statuscond_.lock();
    if ( nrreaders_<1 )
    {
	pErrMsg( "Object is not readlocked.");
    }
    else
    {
	nrreaders_--;
    }

    if ( !nrreaders_ )
	statuscond_.signal( true );

    statuscond_.unLock();
}


void Threads::ReadWriteLock::writeLock()
{
    statuscond_.lock();
    while ( status_!=mUnLocked && nrreaders_!=0 )
	statuscond_.wait();
    
    status_ = mWriteLocked;
    statuscond_.unLock();
}
    
   
void Threads::ReadWriteLock::writeUnLock()
{
    statuscond_.lock();
    if ( status_!=mWriteLocked )
    {
	pErrMsg( "Object is not writelocked.");
    }
    else
    {
	status_ = mUnLocked;
    }

    statuscond_.signal( true );
    statuscond_.unLock();
}


bool Threads::ReadWriteLock::convReadToWriteLock()
{
    statuscond_.lock();
    if ( status_==mUnLocked && nrreaders_==1 )
    {
	status_ = mWriteLocked;
	nrreaders_ = 0;
	statuscond_.unLock();
	return true;
    }
    else if ( !nrreaders_ )
	pErrMsg( "Object is not readlocked.");

    statuscond_.unLock();
    readUnLock();
    writeLock();
    return false;
}


void Threads::ReadWriteLock::convWriteToReadLock()
{
    statuscond_.lock();
    if ( status_!=mWriteLocked )
	pErrMsg( "Object is not writelocked.");
    status_ = mUnLocked;
    nrreaders_ = 1;
    statuscond_.signal( true );
    statuscond_.unLock();
}


void Threads::ReadWriteLock::permissiveWriteLock()
{
    statuscond_.lock();
    while ( status_!=mUnLocked )
	statuscond_.wait();

    status_ = mPermissive;
    statuscond_.unLock();
}


void Threads::ReadWriteLock::permissiveWriteUnLock()
{
    statuscond_.lock();
    if ( status_!=mPermissive )
    {
	pErrMsg("Wrong lock");
    }
    else
    {
	status_ = mUnLocked;
	statuscond_.signal( true );
    }

    statuscond_.unLock();
}


void Threads::ReadWriteLock::convPermissiveToWriteLock()
{
    statuscond_.lock();
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

    statuscond_.unLock();
}


void Threads::ReadWriteLock::convWriteToPermissive()
{
    statuscond_.lock();
    if ( status_!=mWriteLocked )
    {
	pErrMsg("Wrong lock");
    }
    else
    {
	status_ = mPermissive;
	statuscond_.signal( true );
    }

    statuscond_.unLock();
}


Threads::ConditionVar::ConditionVar()
{
    pthread_condattr_init( &condattr );
    pthread_cond_init( &cond, &condattr );
}


//!Implemented since standard copy constuctor will hang the system
Threads::ConditionVar::ConditionVar( const ConditionVar& )
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
    nrproc = 1;

    const char* envres = GetEnvVar( "DTECT_USE_MULTIPROC" );
    bool douse = true;
    if ( envres && (*envres == 'n' || *envres == 'N') )
	douse = false;
    if ( envres && (*envres == 'y' || *envres == 'Y') )
	douse = true;
    if ( !douse ) return nrproc;

    const bool havesett = Settings::common().get( "Nr Processors", nrproc );
    if ( !havesett )
	nrproc = getSysNrProc();

    if ( DBG::isOn( DBG_MT ) ) 
    {
	BufferString msg = "Number of processors (";
	msg += havesett ? "User settings" : "System";
	msg += "): "; msg += nrproc;
	DBG::message( msg );
    }

    if ( nrproc < 1 ) nrproc = 1;
    return nrproc;
}
