#ifndef threadwork_h
#define threadwork_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: threadwork.h,v 1.5 2002-09-11 06:16:24 kristofer Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "callback.h"

class BasicTask;

namespace Threads
{
class Thread;
class ConditionVar;

class WorkThread;
class ThreadWorkManager;


/*!\brief
is the worker that actually does the job and is the link between the manager
and the tasks to be performed.
*/

class WorkThread : public CallBacker
{
public:
    enum		Status { Idle, Running, Finished, Stopped };

    			WorkThread( ThreadWorkManager& );
    			~WorkThread();

    			//Interface from manager
    bool		assignTask(BasicTask*, CallBack* cb = 0);
    			/*!< becomes mine */

    Status		getStatus();
    int			getRetVal();
    BasicTask*		getTask();

protected:

    void		doWork(CallBacker*);
    void 		cancelWork(CallBacker*);
    ThreadWorkManager&	manager;

    ConditionVar&	controlcond;	//Dont change this order!
    Status		status;		//These are protected by the condvar
    int			retval;		//Lock before reading or writing

    bool		exitflag;
    BasicTask*		task;		
    CallBack*		cb;

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

    void			addWork( BasicTask*, CallBack* finished );
    				/*!< Managed by caller */

    bool			addWork( ObjectSet<BasicTask>& );


protected:
    friend			WorkThread;
    				/* Interface from threads */
    void			imFinished( WorkThread* );

    ObjectSet<BasicTask>	workload;
    ObjectSet<CallBack>		callbacks;
    ObjectSet<WorkThread>	threads;

    ConditionVar&		workloadcond;
};

}; // Namespace


#endif
