#ifndef threadwork_h
#define threadwork_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: threadwork.h,v 1.8 2002-12-30 12:00:23 kristofer Exp $
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

/*!\brief
is the top manager of everything. Give the tasks to it and it will be performed
in time. Note that no notification is done when the task is done. It's up to
the user of the class to implement such things in the ThreadTask.
*/

class ThreadWorkManager : public CallBacker
{
public:
				//Interface from outside world
    				ThreadWorkManager( int nrthreads );
				~ThreadWorkManager();

    void			addWork( BasicTask*, CallBack* finished );
    				/*!< Managed by caller */

    bool			addWork( ObjectSet<BasicTask>& );
    void			removeWork( const BasicTask* );	
    				/*!< Removes the task from queue
				     and stop it if allready running
				*/

    Notifier<ThreadWorkManager>	isidle;
    
protected:

    friend class		WorkThread;
    				/* Interface from threads */
    void			imFinished( WorkThread* );

    ObjectSet<BasicTask>	workload;
    ObjectSet<CallBack>		callbacks;
    ObjectSet<WorkThread>	threads;

    ConditionVar&		workloadcond;
};

}; // Namespace


#endif
