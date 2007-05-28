#ifndef threadwork_h
#define threadwork_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: threadwork.h,v 1.15 2007-05-28 15:07:06 cvskris Exp $
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
    				ThreadWorkManager(int nrthreads=-1);
				~ThreadWorkManager();

    void			addWork(BasicTask*,CallBack* finished);
    				/*!< Managed by caller */

    bool			addWork(ObjectSet<BasicTask>&);
    void			removeWork(const BasicTask*);	
    				/*!< Removes the task from queue
				     and stop it if allready running
				*/

    const BasicTask*		getWork(CallBacker*) const;
    				/*!When a work is sumbmitted with a
				   callback, the callback is called with a
				   callbacker. If called from the callback,
				   this function may (not guaranteed) return
				   a pointer to the work that was completed.
				   If not possible, a zero pointer will be
				   returned. */

    int				nrThreads() const { return threads_.size(); }

    Notifier<ThreadWorkManager>	isidle;
    
protected:

    friend class		WorkThread;

    ObjectSet<BasicTask>	workload_;
    ObjectSet<CallBack>		callbacks_;
    ObjectSet<WorkThread>	threads_;

    ObjectSet<WorkThread>	freethreads_;

    ConditionVar&		workloadcond_;
};

}; // Namespace


#endif
