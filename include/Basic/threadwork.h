#ifndef threadwork_h
#define threadwork_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: threadwork.h,v 1.12 2005-01-27 16:03:25 kristofer Exp $
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

    int				nrThreads() const { return threads.size(); }

    Notifier<ThreadWorkManager>	isidle;
    
protected:

    friend class		WorkThread;

    ObjectSet<BasicTask>	workload;
    ObjectSet<CallBack>		callbacks;
    ObjectSet<WorkThread>	threads;

    ObjectSet<WorkThread>	freethreads;

    ConditionVar&		workloadcond;
};

}; // Namespace


#endif
