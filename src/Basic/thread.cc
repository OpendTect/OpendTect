/*
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id: thread.cc,v 1.42 2009-07-22 16:01:31 cvsbert Exp $";

#include "thread.h"
#include "callback.h"
#include "debug.h"
#include "settings.h"
#include "debugmasks.h"
#include "debug.h"
#include "envvars.h"
#include "errh.h"
#include "errno.h" // for EBUSY

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#ifdef __msvc__
# include "windows.h"
#endif


Threads::Mutex::Mutex( bool recursive )
    : qmutex_( new QMutex(QMutex::NonRecursive) )
{}


//!Implemented since standard copy constuctor will hang the system
Threads::Mutex::Mutex( const Mutex& m )
    : qmutex_( new QMutex )
{ }


Threads::Mutex::~Mutex()
{
    delete qmutex_;
}


void Threads::Mutex::lock()
{ 
    qmutex_->lock();
}


void Threads::Mutex::unLock()
{
    qmutex_->unlock();
}


bool Threads::Mutex::tryLock()
{
    return qmutex_->tryLock();
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


Threads::Barrier::Barrier( int nrthreads, bool immrel )
    : nrthreads_( nrthreads )
    , threadcount_( 0 )
    , dorelease_( false )
    , immediaterelease_( immrel )
{}


void Threads::Barrier::setNrThreads( int nthreads )
{
    Threads::MutexLocker lock( condvar_ );
    if ( threadcount_ )
    {
	pErrMsg("Thread waiting. Should never happen");
	return;
    }
}


bool Threads::Barrier::waitForAll( bool unlock )
{
    condvar_.lock();
    threadcount_++;
    if ( threadcount_==nrthreads_ )
    {
	threadcount_--;
	if ( immediaterelease_ )
	    releaseAllNoLock();

	if ( !threadcount_ )
	    dorelease_ = false;

	if ( unlock ) condvar_.unLock();
	return true;
    }
    else
    {
	while ( !dorelease_ )
	    condvar_.wait();
    }

    threadcount_--;
    if ( !threadcount_ )
	dorelease_ = false;

    if ( unlock ) condvar_.unLock();
    return false;
}


void Threads::Barrier::releaseAllNoLock()
{
    dorelease_ = true;
    condvar_.signal( true );
}


void Threads::Barrier::releaseAll()
{
    Threads::MutexLocker lock( condvar_ );
    releaseAllNoLock();
}



Threads::ConditionVar::ConditionVar()
    : Mutex( false )
    , cond_( new QWaitCondition )
{ }


//!Implemented since standard copy constuctor will hang the system
Threads::ConditionVar::ConditionVar( const ConditionVar& )
    : Mutex( false )
    , cond_( new QWaitCondition )
{ }


Threads::ConditionVar::~ConditionVar()
{
    delete cond_;
}


void Threads::ConditionVar::wait()
{ cond_->wait( qmutex_ ); }


void Threads::ConditionVar::signal(bool all)
{
    if ( all ) cond_->wakeAll();
    else cond_->wakeOne();
}


typedef void (*ThreadFunc)(void*);

class ThreadBody : public QThread
{
public:
    			ThreadBody( ThreadFunc func )
			    : func_( func )
			{}

    			ThreadBody( const CallBack& cb )
			    : func_( 0 )
			    , cb_( cb )
			{}

protected:
    void		run()
			{
			    if ( !cb_.willCall() )
				func_( 0 );
			    else
				cb_.doCall( 0 );
			}

    ThreadFunc		func_;
    CallBack		cb_;
};



Threads::Thread::Thread( void (func)(void*) )
    : thread_( new ThreadBody( func ) )
{ thread_->start(); }


Threads::Thread::~Thread()
{ delete thread_; }


Threads::Thread::Thread( const CallBack& cb )
{
    if ( !cb.willCall() ) return;
    thread_ = new ThreadBody( cb );
    thread_->start();
}


void Threads::Thread::stop()
{
    thread_->wait();
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
	    nrproc = QThread::idealThreadCount();
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
