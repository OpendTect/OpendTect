#ifndef threadwork_h
#define threadwork_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: threadwork.h,v 1.24 2010-11-10 20:34:13 cvskris Exp $
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

The object can handle multiple queues. This is mostly intersting when you want
to ensure that a shared resource is always accessed single threaded, but without using syncronization.
*/

mClass ThreadWorkManager : public CallBacker
{
public:
				//Interface from outside world
    				ThreadWorkManager(int nrthreads=-1);
				~ThreadWorkManager();

    int				addQueue(bool parallel);
    				//!<\returns queid
    int				queueSize(int queueid) const;
    void			removeQueue(int queueid,bool finishall);
    				/*!<Removes queue. If finishall is true,
				    all work in the queue will be finished. */
    static int			cDefaultQueueID() { return 0; }

    void			addWork(SequentialTask*,CallBack* finished,
	    				int queueid, bool putfirstinline );
    				/*!< Managed by caller */

    bool			addWork(ObjectSet<SequentialTask>&,
	    				bool firstinline = false);
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

    int				queueSizeNoLock(int queueid) const;
    int				reportFinishedAndAskForMore(WorkThread*,
							    int oldqueueid );

    friend class		WorkThread;

    //Linked (one entry per que-entry)
    ObjectSet<SequentialTask>	workload_;
    TypeSet<int>		workqueueid_;
    ObjectSet<CallBack>		callbacks_;

    ObjectSet<WorkThread>	threads_;
    ObjectSet<WorkThread>	freethreads_;


    //Linked (one entry per queue)
    TypeSet<int>		queueids_;
    TypeSet<int>		queueworkload_; //Nr threads working on it
    BoolTypeSet			queueisparallel_;
    BoolTypeSet			queueisclosing_;

    ConditionVar&		workloadcond_;

    int				freeid_;
};

}; // Namespace


#endif
