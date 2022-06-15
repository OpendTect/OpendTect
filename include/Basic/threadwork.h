#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "basicmod.h"

#include "bufstringset.h"
#include "callback.h"
#include "genc.h"
#include "objectset.h"
#include "task.h"
#include "uistring.h"

using StaticTaskFunction = bool(*)(void);
typedef bool (CallBacker::*TaskFunction)(void);

namespace Threads
{
class Thread;
class ConditionVar;
class WorkThread;
class Work;


/*!
\brief Takes work and puts it on a queue for execution either in parallel,
singlethread or manual.
*/

mExpClass(Basic) WorkManager : public CallBacker
{ mODTextTranslationClass(WorkManager)
public:

				//Interface from outside world
				WorkManager(int nrthreads=-1);
				~WorkManager();

    enum QueueType		{ MultiThread, SingleThread, Manual };
    int				addQueue(QueueType type,const char* name);
				/*!<Manual queues will not be executed
				    automaticall, only at executeQueue.
				    \returns queid
				    \param type See QueueType
				    \param name is for debugging
                                */
    void                        setQueueName(int queueid,const char*);
    int				queueSize(int queueid) const;
    void			emptyQueue(int queueid,bool finishall);
    void			removeQueue(int queueid,bool finishall);
				/*!<Removes queue. If finishall is true,
				    all work in the queue will be finished. */
    static int			cDefaultQueueID() { return 0; }
    bool			executeQueue(int queueid);
				/*!<Runs all jobs in a que. Only for manual
				    queues */

    void			addWork(const Work&,CallBack* finished =0,
					int queueid=cDefaultQueueID(),
					bool putfirstinline=false,
					bool discardduplicates=false,
					bool forcedifferentthread=false);
				/*!<\param forcedifferentthread will force
					work to be put on the queue, even if it
					could be done directly in current
					thread.
				    \param finished, queueid, putfirstinline,
					   discardduplicates*/

    bool			addWork(TypeSet<Work>&, int queueid = -1,
					bool firstinline = false);
				/*!<Will wait for all work to be finished.
				   \returns true if all Work was completed
					    without error.*/

    bool			executeWork( Work*, int sz, int queueid = -1,
					bool firstinline = false );
				//!<Returns when finished with all
    bool			removeWork(const Work&);
				/*!< Removes the task from queue
				     and stop it if allready running.
				    \returns true if the task was removed
				    before it had started.*/

    const Work*			getWork(CallBacker*) const;
				/*!<When a work is submitted with a
				    callback, the callback is called with a
				    callbacker. If called from the callback and
				    the callbacker is non-zero, a pointer to the
				    work that was completed is returned.
				    If not possible, a zero pointer will be
				    returned. */
    bool			getWorkExitStatus(CallBacker*) const;
				/*!<When a work is submitted with a
				    callback, the callback is called with a
				    callbacker. If called from the callback and
				    the callbacker is non-zero, the exit status
				    of the work will be returned. Otherwise
				    false.
				*/
    uiString			uiMessage(CallBacker*) const;
				/*!<When a work is submitted and setup with a
				    task, returns the task message when not
				    finished successfully.
				  */

    int				nrThreads() const { return threads_.size(); }
    int				nrFreeThreads() const;
				//!<Valid right now, may change any time
    bool			isWorkThread() const;

    Notifier<WorkManager>	isidle;

    static Threads::WorkManager& twm();

    Notifier<WorkManager>	isShuttingDown;
    void			shutdown();

protected:

    int				queueSizeNoLock(int queueid) const;
    int				reportFinishedAndAskForMore(WorkThread*,
							    int oldqueueid );
    inline void			reduceWorkload(int queueidx);

    friend class		WorkThread;

    //Linked (one entry per que-entry)
    TypeSet<Work>		workload_;
    TypeSet<int>		workqueueid_;
    TypeSet<CallBack>		callbacks_;

    ObjectSet<WorkThread>	threads_;
    ObjectSet<const void>	threadids_;
    ObjectSet<WorkThread>	freethreads_;

    //Linked (one entry per queue)
    TypeSet<int>		queueids_;
    TypeSet<int>		queueworkload_; //Nr threads working on it
    TypeSet<QueueType>		queuetypes_;
    BufferStringSet             queuenames_;
    BoolTypeSet			queueisclosing_;

    ConditionVar&		workloadcond_;

    int				freeid_;
    const int			twmid_; //!<Only for debugging
};


/*!
\brief The abstraction of something that can be done. It can be an ordinary
 CallBack, a static function (must return bool) or a TaskFunction on a
 CallBackerinheriting class, or a Task. The three examples are shown below.

\code
    mExpClass(Basic) MyClass : public CallBacker
    {
        void		normalCallBack(CallBacker*);
	bool		taskFunction();
	static bool	staticFunc();
    };
\endcode
    Calls to normalCallBack and task functions can be invoked as:
\code
    Threads::WorkManager::twm().addWork(
                Work(mCB(this,MyClass,normalCallBack) ) );
\endcode
    or
\code
    Threads::WorkManager::twm().addWork( mWMT(this,MyClass,taskFunction));
\endcode
    or
\code
    Threads::WorkManager::twm().addWork( Work( &MyClass::staticFunc) );
\endcode

You can also add Tasks, with the option that they may be deleted when
the work is done, or if there is an error.
*/

mExpClass(Basic) Work
{ mODTextTranslationClass(Work)
public:
    inline		Work();
    inline		Work(const CallBack&);
    inline		Work(CallBacker* o,TaskFunction f);
    inline		Work(StaticTaskFunction f);
    inline		Work(Task& t,bool takeover);

    bool		operator==(const Work&) const;
    inline const Task*	getTask() const;

    inline bool		isOK() const;
    inline bool		execute();
    inline uiString	errMsg() const	{ return msg_; }

private:

    void		destroy();
    CallBacker*		obj_;
    CallBackFunction	cbf_;
    TaskFunction	tf_;
    StaticTaskFunction	stf_;
    bool		takeover_=false;

    uiString		msg_;

    friend class	WorkThread;
    friend class	WorkManager;
};

#define mSTFN(clss,fn) ((::TaskFunction)(&clss::fn))
#define mWMT(obj,clss,fn) ::Threads::Work( obj, mSTFN(clss,fn) )


}; // Namespace

inline Threads::Work::Work()
    : obj_( 0 ), cbf_( 0 ), tf_( 0 ), stf_( 0 )				{}


inline Threads::Work::Work( const CallBack& cb )
    : obj_( const_cast<CallBacker*>(cb.cbObj()) )
    , cbf_( cb.cbFn() ), tf_( 0 ), stf_( 0 )				{}


inline Threads::Work::Work( CallBacker* o, TaskFunction f )
    : obj_( o ), cbf_( 0 ), tf_( f ), stf_( 0 ), takeover_( false )	{}


inline Threads::Work::Work( StaticTaskFunction f )
    : obj_( 0 ), cbf_( 0 ), tf_( 0 ), stf_( f )				{}


inline Threads::Work::Work( Task& t, bool takeover )
    : obj_( &t ), cbf_( 0 ), tf_( mSTFN(Task,execute) ), stf_( 0 )
    , takeover_( takeover )						{}


inline const Task* Threads::Work::getTask() const
{
    mDynamicCastGet(const Task*, task, obj_);
    return task;
}


inline bool Threads::Work::isOK() const
{ return stf_ || (obj_ && (tf_ || cbf_) ); }


inline bool Threads::Work::execute()
{
    if ( stf_ ) return stf_();
    if ( tf_ )
    {
	const bool res = (obj_->*tf_)();
	if ( res )
	    msg_ = uiString::emptyString();
	else
	{
	    mDynamicCastGet(Task*,task,obj_)
	    msg_ = task ? task->uiMessage() : uiString::emptyString();
	}

	return res;
    }

    (obj_->*cbf_)( 0 );

    return true;
}
