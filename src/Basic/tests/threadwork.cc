/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "threadwork.h"
#include "thread.h"
#include "atomic.h"
#include "testprog.h"
#include "math2.h"


#define mPrintTestResult( queuetypename, testname ) \
{\
    errStream() << queuetypename << "\"" testname "\"" << od_endl;\
    return false;\
}\
else \
{\
    logStream() << queuetypename << "\"" testname "\" Success" << od_endl;\
}

#define mAddWork( work, queueid ) \
{ \
    CallBack finishedcb( mCB(&work,CallBackTestClass,workFinishedCB) ); \
    workmanager.addWork( \
	  Threads::Work(mCB(&work,CallBackTestClass,doTheWork)), \
	  &finishedcb,queueid,false ); \
}

#define mAddMultipleJobs( work, queueid ) \
for ( int idy=0; idy<worksize; idy++ )\
{\
    mAddWork( work, queueid ); \
}

#define mWorkLoadLength 10000000

class CallBackTestClass : public CallBacker
{
public:
			CallBackTestClass()
			    : workload_(0)
			    , maxworkload_(0)
			    , nrfinished_(0)
			{}

    void		reset() { maxworkload_=0; nrfinished_ = 0; }

    void		doTheWork(CallBacker*)
			{
			    mutex_.lock();
			    workload_++;
			    maxworkload_ = mMAX(maxworkload_,workload_);
			    mutex_.unLock();

			    run();

			    mutex_.lock();
			    workload_--;
			    mutex_.unLock();
			}

    bool		run();

    void		workFinishedCB(CallBacker*)
			{ nrfinished_++; }

    Threads::Atomic<int> nrfinished_;

    Threads::Mutex	mutex_;
    int			workload_;
    int			maxworkload_;
};


bool CallBackTestClass::run()
{
    for ( float idx=0; idx<mWorkLoadLength; idx++ )
    {
	Math::Sqrt( idx );
    }
    return true;
}

CallBackTestClass backgroundwork;

class WorkManagerTester : public CallBacker
{
public:

    bool runCallBackTests()
    {
	Threads::WorkManager& workmanager = Threads::WorkManager::twm();
	const int nrthreads = workmanager.nrThreads();
	const int worksize = nrthreads*2;

	TypeSet<Threads::WorkManager::QueueType> queuetype;
	BufferStringSet typenames;
	queuetype += Threads::WorkManager::MultiThread;
	typenames.add( "Multi Threaded Queue ");
	queuetype += Threads::WorkManager::SingleThread;
	typenames.add( "Single Threaded Queue ");
	queuetype += Threads::WorkManager::Manual;
	typenames.add( "Manual Queue ");

	for ( int idx=0; idx<queuetype.size(); idx++ )
	{
	    int queueid = workmanager.addQueue( queuetype[idx], "Test" );
	    const char* queuetypename = typenames.get(idx).buf();
	    CallBackTestClass testwork;

	    mAddWork( testwork, queueid );
	    mAddMultipleJobs( backgroundwork, workmanager.cDefaultQueueID() );

	    if ( queuetype[idx] == Threads::WorkManager::Manual )
		workmanager.executeQueue( queueid );

	    workmanager.emptyQueue( queueid, true );

	    if ( testwork.nrfinished_ != 1 )
		mPrintTestResult( queuetypename,
				"Single work finished in emptyQueue." );

	    if ( workmanager.queueSize(queueid) > 0 )
		mPrintTestResult( queuetypename,
			"Queue size after emptying the queue is zero." );

	    testwork.reset();
	    mAddWork( testwork, queueid );
	    mAddMultipleJobs( backgroundwork, workmanager.cDefaultQueueID() );
	    if ( queuetype[idx] == Threads::WorkManager::Manual )
		workmanager.executeQueue( queueid );

	    workmanager.removeQueue( queueid, true );

	    if ( testwork.nrfinished_ != 1 )
		mPrintTestResult( queuetypename,
				"Work finished when queue is removed." );

	    if ( queuetype[idx] == Threads::WorkManager::Manual )
		continue;

	    queueid = workmanager.addQueue( queuetype[idx], "Test" );

	    testwork.reset();
	    mAddMultipleJobs( testwork, queueid );
	    mAddMultipleJobs( backgroundwork, workmanager.cDefaultQueueID() );

	    workmanager.emptyQueue( queueid, true );
	    if ( testwork.nrfinished_!=worksize )
		mPrintTestResult( queuetypename,
				"All threads finished in large batch." );

	    testwork.reset();
	    mAddMultipleJobs( testwork, queueid );
	    mAddMultipleJobs( backgroundwork, workmanager.cDefaultQueueID() );

	    workmanager.emptyQueue( queueid, false );
	    if ( testwork.nrfinished_==worksize )
		mPrintTestResult( queuetypename,
				"Pending workload removed in emptyQueue." );

	    workmanager.emptyQueue( queueid, true );

	    if ( testwork.workload_ )
		mPrintTestResult( queuetypename,
				"No pending work left after empty queue." );

	    if ( queuetype[idx]==Threads::WorkManager::SingleThread )
	    {
		if ( testwork.maxworkload_>1 )
		    mPrintTestResult( queuetypename,
				"Only one thread for single threaded queue." );
	    }

	    testwork.reset();
	    mAddMultipleJobs( testwork, queueid );
	    mAddMultipleJobs( backgroundwork, workmanager.cDefaultQueueID() );

	    workmanager.removeQueue( queueid, false );
	    if ( testwork.nrfinished_==worksize )
		mPrintTestResult( queuetypename,
				"Pending workload removed in removeQueue." );

	    if ( testwork.workload_ )
		mPrintTestResult( queuetypename,
				"No pending work left after removeQueue." );
	}

	//workmanager.emptyQueue( workmanager.cDefaultQueueID(), true );

	return true;
    }

    bool trueFunc() { return true; }
    static bool falseFunc() { return false; }

    bool testWorkResults()
    {
	Threads::WorkManager& workmanager = Threads::WorkManager::twm();

	TypeSet<Threads::Work> tasks;
	tasks += Threads::Work( mWMT(this,WorkManagerTester,trueFunc) );

	if ( !workmanager.addWork( tasks, workmanager.cDefaultQueueID() ) )
	    mPrintTestResult( "", "Result of bool function is true." );

	tasks.erase();
	tasks += Threads::Work( falseFunc );
	if ( workmanager.addWork( tasks, workmanager.cDefaultQueueID() ) )
	    mPrintTestResult( "", "Result of bool function is false." );

	return true;
    }
};


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    WorkManagerTester tester;
    const bool res = tester.runCallBackTests()
		  && tester.testWorkResults();

   // Threads::WorkManager::twm().shutdown();
    return res ? 0 : 1;
}
