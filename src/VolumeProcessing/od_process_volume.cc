    /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu / Bert
 * DATE     : April 2007 / Feb 2016
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"

#include "jobcommunic.h"
#include "volprocprocessor.h"
#include "moddepmgr.h"
#include "thread.h"

class CommThread : public CallBacker
{
public:

			    CommThread(JobCommunic* comm, double delay)
				: comm_(comm)
				, delay_(delay)
				, thread_(0)
			    {}
			    ~CommThread()
			    {
				comm_->setState( JobCommunic::Finished );
				comm_->sendState();
			    }

void start()
{
    if ( thread_ )
	return;

    thread_ = new Threads::Thread( mCB(this,CommThread,doWork) );
}

void stop()
{
    if ( thread_ )
    {	delete thread_; thread_ = 0; }
}


protected:

void doWork( CallBacker* cb )
{
    while( 1 )
    {
	if ( comm_->state() == JobCommunic::JobError
		|| comm_->state() == JobCommunic::Finished )
		break;

	Threads::sleep( delay_ );
	comm_->sendState();
    }
}
     
    JobCommunic*	comm_;
    double		delay_;
    Threads::Thread*	thread_;
};


bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded( "VolumeProcessing" );
    OD::ModDeps().ensureLoaded( "Well" );
    PtrMan<CommThread> commthrd = 0;
    if ( comm_ )
    {
	comm_->setState( JobCommunic::Working );
	comm_->sendState();
	comm_->setTimeBetweenMsgUpdates( 5000 );
	commthrd = new CommThread( comm_, 0.6 );
	commthrd->start();
    }
    
    VolProc::Processor proc( pars() );

    const bool ret = proc.run(strm,comm_);

    if ( comm_ )
    {
	JobCommunic::State stat =  ret ? JobCommunic::Finished
				       : JobCommunic::JobError;
	comm_->setState( stat );
	comm_->sendState();
	commthrd->stop();
    }
    
    return ret;
}
