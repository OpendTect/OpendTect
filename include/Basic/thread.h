#ifndef thread_h
#define thread_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: thread.h,v 1.1 1999-11-16 16:05:41 kristofer Exp $
________________________________________________________________________

*/

#ifdef lux
#define pthread
#endif

#ifdef ibm
#define pthread
#endif

#ifdef sun5
#define pthread
#endif

#ifdef pthread
#include <pthread.h>

class Thread
{
public:
    class Mutex
    {
    public:
		Mutex()
		{ pthread_mutex_init( &mutex, &attr );}
		~Mutex() { pthread_mutex_destroy( &mutex ); }

	int	lock() { return pthread_mutex_lock( &mutex ); }
	int	unlock() { return pthread_mutex_unlock( &mutex ); }

protected:
	pthread_mutex_t mutex;	
	pthread_mutexattr_t attr;

    };

    class MutexLocker
    {
    public:
				MutexLocker( Thread::Mutex& mutex_ )
				: mutex( mutex_ )
				{ mutex.lock(); }

				~MutexLocker()
				{ mutex.unlock(); }

    protected:
	Thread::Mutex&		mutex;
    };
};

#endif

// Place other non-pthread platforms here

#endif

