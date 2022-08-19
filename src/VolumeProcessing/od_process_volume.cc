/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"

#include "jobcommunic.h"
#include "volprocprocessor.h"
#include "moddepmgr.h"
#include "thread.h"

class CommThread : public CallBacker
{
public:

CommThread( JobCommunic* comm, double delay )
    : comm_(comm)
    , delay_(delay)
{
    comm_->setState( JobCommunic::Working );
    comm_->sendState();
}

~CommThread()
{
    comm_->setState( JobCommunic::Finished );
    comm_->sendState();
}

void start()
{
    thread_ = new Threads::Thread( mCB(this,CommThread,reportWork) );
}

void stop()
{
    if ( thread_ )
	deleteAndZeroPtr( thread_ );
}


private:

void reportWork( CallBacker* cb )
{
    while( true )
    {
	if ( comm_->state() == JobCommunic::JobError ||
	     comm_->state() == JobCommunic::Finished )
	    break;

	Threads::sleep( delay_ );
	comm_->sendState();
    }
}

    JobCommunic*	comm_;
    double		delay_;
    Threads::Thread*	thread_ = nullptr;
};


mLoad2Modules("VolumeProcessing","Well")

bool BatchProgram::doWork( od_ostream& strm )
{
    PtrMan<CommThread> commthrd = nullptr;
    if ( comm_ )
    {
	comm_->setTimeBetweenMsgUpdates( 5000 );
	commthrd = new CommThread( comm_, 0.6 );
	commthrd->start();
    }

    VolProc::Processor proc( pars() );

    const bool ret = proc.run(strm,comm_);

    if ( comm_ )
    {
	const JobCommunic::State stat =  ret ? JobCommunic::Finished
					     : JobCommunic::JobError;
	comm_->setState( stat );
	comm_->sendState();
	commthrd->stop();
	commthrd = nullptr;
    }

    return ret;
}
