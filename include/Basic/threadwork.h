#ifndef threadwork_h
#define threadwork_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: threadwork.h,v 1.20 2009-07-22 15:56:48 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "objectset.h"
#include "callback.h"

class SequentialTask;

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

mClass ThreadWorkManager : public CallBacker
{
public:
				//Interface from outside world
    				ThreadWorkManager(int nrthreads=-1);
				~ThreadWorkManager();

    void			addWork(SequentialTask*,CallBack* finished);
    				/*!< Managed by caller */

    bool			addWork(ObjectSet<SequentialTask>&);
    bool			removeWork(const SequentialTask*);	
    				/*!< Removes the task from queue
				     and stop it if allready running
				    \returns true if the task was removed
				    before it had started.*/

    const SequentialTask*	getWork(CallBacker*) const;
    				/*!When a work is sumbmitted with a
				   callback, the callback is called with a
				   callbacker. If called from the callback and
				   the callbacker is non-zero, a pointer to the
				   work that was completed is returned.
				   If not possible, a zero pointer will be
				   returned. */

    int				nrThreads() const { return threads_.size(); }

    Notifier<ThreadWorkManager>	isidle;
    
protected:

    friend class		WorkThread;

    ObjectSet<SequentialTask>	workload_;
    ObjectSet<CallBack>		callbacks_;
    ObjectSet<WorkThread>	threads_;

    ObjectSet<WorkThread>	freethreads_;

    ConditionVar&		workloadcond_;
};

}; // Namespace


#endif
