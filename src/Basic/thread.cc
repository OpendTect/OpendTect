/*
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "thread.h"
#include "callback.h"
#include "debug.h"
#include "settings.h"
#include "debugmasks.h"
#include "debug.h"
#include "envvars.h"
#include "errh.h"

#ifdef __ittnotify__
# include <ittnotify.h>
# define mSetupIttNotify( var, name ) \
	__itt_sync_create( &var, 0, name,  __itt_attr_mutex )
# define mDestroyIttNotify( var ) __itt_sync_destroy( &var )
# define mPrepareIttNotify( var ) __itt_sync_destroy( &var )
# define mIttNotifyAcquired( var ) __itt_sync_acquired( &var )
# define mIttNotifyReleasing( var ) __itt_sync_releasing( &var )
# define mIttNotifyCancel( var ) __itt_sync_cancel( &var )
#else
# define mSetupIttNotify( var, name ) 
# define mDestroyIttNotify( var )
# define mPrepareIttNotify( var )
# define mIttNotifyAcquired( var )
# define mIttNotifyReleasing( var )
# define mIttNotifyCancel( var )
#endif

#include "qatomic.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#ifdef __msvc__
# include "windows.h"
#endif

mUseQtnamespace

Threads::Mutex::Mutex( bool recursive )
#ifndef OD_NO_QT
    : qmutex_( new QMutex( recursive
		? QMutex::Recursive : QMutex::NonRecursive) )
#endif
{}


//!Implemented since standard copy constuctor will hang the system
Threads::Mutex::Mutex( const Mutex& m )
#ifndef OD_NO_QT
    : qmutex_( new QMutex )
#endif
{
#ifdef __debug__
    lockingthread_ = 0;
    count_ = 0;
#endif
}


Threads::Mutex::~Mutex()
{
#ifndef OD_NO_QT
    delete qmutex_;
#endif
}


void Threads::Mutex::lock()
{ 
#ifndef OD_NO_QT
    qmutex_->lock();
#endif

#ifdef __debug__
    count_++;
    lockingthread_ = currentThread();
#endif
}


void Threads::Mutex::unLock()
{
#ifndef OD_NO_QT
# ifdef __debug__
    count_--;
    if ( lockingthread_ !=currentThread() )
    {
	pErrMsg("Unlocked from the wrong thead");
	DBG::forceCrash( false );
    }

    if ( !count_ )
	lockingthread_ = 0;
# endif
    qmutex_->unlock();
#endif
}


bool Threads::Mutex::tryLock()
{
#ifndef OD_NO_QT
    if ( qmutex_->tryLock() )
    {
# ifdef __debug__
	lockingthread_ = currentThread();
# endif
	return true;
    }
    return false;
#else
# ifdef __debug__
    lockingthread_ = currentThread();
# endif
    return true;
#endif
}

Threads::SpinLock::SpinLock( bool recursive )
    : count_( 0 )
    , recursive_( recursive )
    , lockingthread_( 0 )
{
    mSetupIttNotify( lockingthread_, "Threads::SpinLock" );
}

Threads::SpinLock::~SpinLock()
{
    mDestroyIttNotify( lockingthread_ );
}

void Threads::SpinLock::lock()
{
    const void* currentthread = currentThread();
    if ( recursive_ && lockingthread_ == currentthread )
    {
	count_ ++;
	return;
    }

    mPrepareIttNotify( lockingthread_ );
    while ( !lockingthread_.setIfEqual( currentthread, 0 ) )
	;

    mIttNotifyAcquired( lockingthread_ );

    count_ = 1;
}


void Threads::SpinLock::unLock()
{
    count_--;
    if ( !count_ )
    {
	mIttNotifyReleasing( lockingthread_ );
	lockingthread_ = 0;
    }
}


bool Threads::SpinLock::tryLock()
{
    const void* currentthread = currentThread();
    if ( recursive_ && lockingthread_ == currentthread )
    {
	count_ ++;
	return true;
    }

    mPrepareIttNotify( lockingthread_ );
    if ( lockingthread_.setIfEqual( currentthread, 0 ) )
    {
	mIttNotifyAcquired( lockingthread_ );
	count_ ++;

	return true;
    }

    mIttNotifyCancel( lockingthread_ );
    return false;
}

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


bool Threads::ReadWriteLock::tryReadLock()
{
    Threads::MutexLocker lock( statuscond_ );
    if ( status_==mWriteLocked )
	return false;

    nrreaders_++; 
    return true;
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
    
   
bool Threads::ReadWriteLock::tryWriteLock()
{
    Threads::MutexLocker lock( statuscond_ );
    if ( status_!=mUnLocked || nrreaders_ )
	return false;

    status_ = mWriteLocked;
    return true;
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

    nrthreads_ = nthreads;
}


bool Threads::Barrier::waitForAll( bool unlock )
{
    if ( nrthreads_==-1 )
    {
	pErrMsg("Nr threads not set");
	DBG::forceCrash(true);
	return false;
    }

    condvar_.lock();

    //Check of all threads our out of previous iteration
    while ( threadcount_ && dorelease_ )
	condvar_.wait();

    threadcount_++;
    if ( threadcount_==nrthreads_ )
    {
	threadcount_--;
	if ( immediaterelease_ )
	    releaseAllNoLock();

	if ( !threadcount_ )
	{
	    dorelease_ = false;
	    condvar_.signal( true );
	}

	if ( unlock ) condvar_.unLock();
	return true;
    }
    else
    {
	dorelease_ = false;
	while ( !dorelease_ )
	    condvar_.wait();
    }

    threadcount_--;
    if ( !threadcount_ )
    {
	dorelease_ = false;
	condvar_.signal( true );
    }

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
#ifndef OD_NO_QT
    , cond_( new QWaitCondition )
#endif
{ }


//!Implemented since standard copy constuctor will hang the system
Threads::ConditionVar::ConditionVar( const ConditionVar& )
    : Mutex( false )
#ifndef OD_NO_QT
    , cond_( new QWaitCondition )
#endif
{ }


Threads::ConditionVar::~ConditionVar()
{
#ifndef OD_NO_QT
    delete cond_;
#endif
}


void Threads::ConditionVar::wait()
{
#ifndef OD_NO_QT
# ifdef __debug__
    if ( lockingthread_ !=currentThread() )
    {
	pErrMsg("Waiting from the wrong thead");
	DBG::forceCrash( false );
    }
    count_ --;
    lockingthread_ = 0;
# endif
    cond_->wait( qmutex_ );

#ifdef __debug__
    lockingthread_ = currentThread();
    count_++;
# endif
#endif
}


void Threads::ConditionVar::signal(bool all)
{
#ifndef OD_NO_QT
    if ( all ) cond_->wakeAll();
    else cond_->wakeOne();
#endif
}


typedef void (*ThreadFunc)(void*);

#ifndef OD_NO_QT
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

    static void		sleep( double tm )
			{ QThread::msleep( mNINT32(1000*tm) ); }

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
#endif



Threads::Thread::Thread( void (func)(void*) )
#ifndef OD_NO_QT
    : thread_( new ThreadBody( func ) )
#endif
{
#ifndef OD_NO_QT
    thread_->start();
#endif
}


Threads::Thread::~Thread()
{
#ifndef OD_NO_QT
    thread_->wait();
    delete thread_;
#endif
}


Threads::Thread::Thread( const CallBack& cb )
{
    if ( !cb.willCall() ) return;
#ifndef OD_NO_QT
    thread_ = new ThreadBody( cb );
    thread_->start();
#endif
}


const void* Threads::Thread::threadID() const
{
    return thread_;
}


const void* Threads::currentThread()
{
#ifndef OD_NO_QT
    return QThread::currentThread();
#else
    return 0;
#endif
}


void Threads::Thread::waitForFinish()
{
#ifndef OD_NO_QT
    thread_->wait();
#endif
}


int Threads::getSystemNrProcessors()
{ return QThread::idealThreadCount(); }

int Threads::getNrProcessors()
{
    static int nrproc = -1;
    if ( nrproc > 0 ) return nrproc;

    nrproc = 0;

    bool havesett = false; bool haveenv = false;
    if ( !GetEnvVarYN("OD_NO_MULTIPROC") )
    {
	havesett = Settings::common().get( "Nr Processors", nrproc );
	if ( !havesett )
	    havesett = Settings::common().get( "dTect.Nr Processors", nrproc );

	bool needauto = !havesett;
	float perc = 100;
	const char* envval = GetEnvVar( "OD_NR_PROCESSORS" );
	if ( envval && *envval )
	{
	    BufferString str( envval );
	    char* ptr = strrchr(str.buf(),'%');
	    if ( ptr )
		{ *ptr = '\0'; needauto = true; perc = toFloat(str.buf()); }
	    else
		{ needauto = false; nrproc = toInt(envval); }
	    haveenv = true;
	}

	if ( nrproc < 1 || needauto )
	{
	    havesett = false;
	    nrproc = QThread::idealThreadCount();
	}

	float fnrproc = nrproc * perc * 0.01;
	nrproc = mNINT32(fnrproc);
    }

    if ( DBG::isOn( DBG_MT ) ) 
    {
	BufferString msg;
	if ( nrproc == 0 )
	    msg = "OD_NO_MULTIPROC set: one processor used";
	else
	{
	    msg = "Number of processors (";
	    msg += haveenv ? "Environment"
			   : (havesett ? "User settings" : "System");
	    msg += "): "; msg += nrproc;
	}
	DBG::message( msg );
    }

    if ( nrproc < 1 ) nrproc = 1;
    return nrproc;
}


void Threads::sleep( double tm )
{ ThreadBody::sleep( tm ); }
