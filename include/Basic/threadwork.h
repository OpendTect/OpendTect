#ifndef threadwork_h
#define threadwork_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: threadwork.h,v 1.2 2002-04-17 10:12:26 kristofer Exp $
________________________________________________________________________


-*/

#include "sets.h"

namespace Threads
{
class Thread;
class ConditionVar;

class WorkThread;
class ThreadWorkManager;

/*!\brief
Basic class for tasks that can be performed in the a thread. The
WorkThread::checkForExit should be called every now and then to se if the
thread wants to end (i.e.  is about to be destroyed). It is not dangerous to
not call it, but it might introduce delays when exiting the program.
*/

class ThreadTask
{
public:
    virtual		~ThreadTask();
    virtual int		run( WorkThread* ) = 0;
};


/*!\brief
is the worker that actually does the job and is the link between the manager
and the tasks to be performed.
*/

class WorkThread
{
public:
    enum		Status { Idle, Running, Finished, Stopped };
    			WorkThread( ThreadWorkManager& );
    			~WorkThread();
    			// Interface from running thread
    void		checkForExit();

    			//Interface from manager
    bool		assignTask(ThreadTask*);
    			/*!< becomes mine */
    Status		getStatus();
    int			getRetVal();

protected:
    void static		threadfunc( void* );
    void		threadFunc();
    ThreadWorkManager&	manager;

    Status		status;		//These are protected by the condvar
    int			retval;		//Lock before reading or writing
    bool		exitflag;
    ThreadTask*		task;		

    ConditionVar&	exitcond;	//Dont change this order!
    Thread*		thread;
};


/*!\brief
is the top manager of everything. Give the tasks to it and it will be performed
in time. Note that no notification is done when the task is done. It's up to
the user of the class to implement such things in the ThreadTask.
*/

class ThreadWorkManager
{
public:
				//Interface from outside world
    				ThreadWorkManager( int nrthreads );
				~ThreadWorkManager();

    void			addWork( ThreadTask* );
    				/*!< becomes mine */

    				/* Interface from threads */
    void			imFinished( WorkThread* );
protected:
    ObjectSet<ThreadTask>	workload;
    ObjectSet<WorkThread>	threads;

    ConditionVar&		workloadcond;
};


}; // Namespace


#endif
