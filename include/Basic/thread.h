#ifndef thread_h
#define thread_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	K. Tingdahl
 Date:		9-3-1999
 RCS:		$Id: thread.h,v 1.3 2000-12-11 12:29:21 dgb Exp $
________________________________________________________________________

*/

#include <gendefs.h>

#ifndef __win__
#define __pthread__ 1
#endif

#ifdef __pthread__
#include <pthread.h>
#endif

class BasicTask;

class Threads
{
public:
    class Mutex
    {
    public:
					Mutex();
					~Mutex();	
	int				lock();
	int				unlock();


	class Locker
	{
	public:
					Locker( Threads::Mutex& mutex_ );
					~Locker();
	protected:
	    Threads::Mutex&		mutex;
	};

    protected:

#ifdef __pthread__
        pthread_mutex_t 		mutex;
        pthread_mutexattr_t 		attr;
#endif

    };

    class ConditionVar : public Threads::Mutex
    {
    public:
					ConditionVar();
					~ConditionVar();
	int				wait();

	int				unlock(bool);
	int				unlock();
	int 				signal(bool);

    protected:
#ifdef __pthread__
	pthread_cond_t			cond;
	pthread_condattr_t		condattr;
#endif
    };

    class Thread
    {
    public:
	bool				setFunction(void (*)(void*));
	int				start();
	void				stop(); 
					// Signals the thread to end and
					// waits until the thread does
					// an threadExit().

					Thread();
					~Thread();

	static void			threadExit( void* =0 );
					// Should only be called by the 
					// running thread

	bool				exitflag;
	ConditionVar			exitcond;
    private:
	void*				func;
#ifdef __pthread__
	pthread_t			id;
#endif

	void*				ret_val;

	bool				is_running;
	int				join();

    };
					

    class TaskRunner
    {
    public:
	enum			Status { Idle, Running, Stopped, Finished };
	Status			status() const;
	bool			setTask( BasicTask* );

	bool			start();
	bool			stop();

	int			getLastVal() const { return data.lastval; }

				TaskRunner();
				~TaskRunner();

	class Data : public Thread
	{
	public:
	    bool		stopflag;
	    BasicTask*		task;
	    int 		lastval;
	    Status		stat;
	};

	Data*			getData();

    protected:
	static void		threadFunc(void*);
	Data			data;
    };
};

#endif

