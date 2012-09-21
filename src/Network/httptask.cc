/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Oct 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "httptask.h"
#include "odhttp.h"

HttpTask::HttpTask( ODHttp& http )
    : Executor("Downloading file(s)")
    , nrdone_(0)
    , totalnr_(0)
    , http_(http)
    , state_(MoreToDo())
{
    http_.dataReadProgress.notify( mCB(this,HttpTask,progressCB) );
    http_.done.notify( mCB(this,HttpTask,doneCB) );
}


HttpTask::~HttpTask()
{
    http_.dataReadProgress.remove( mCB(this,HttpTask,progressCB) );
    http_.done.remove( mCB(this,HttpTask,doneCB) );
}


int HttpTask::nextStep()
{
    if ( !http_.isOK() )
    {
	msg_ = http_.message();
	state_ = ErrorOccurred();
    }

    return state_;
}


void HttpTask::controlWork( Control ctrl )
{
    if ( ctrl == Task::Run )
	return;

    http_.forceAbort();
    state_ = ErrorOccurred();
    Task::controlWork( ctrl );
}


void HttpTask::progressCB( CallBacker* )
{
    nrdone_ = http_.nrDone() / 1024;
    totalnr_ = http_.totalNr() / 1024;
    state_ = MoreToDo();
}


void HttpTask::doneCB( CallBacker* )
{
    if ( http_.isOK() )
    {
	state_ = Finished();
    }
    else
    {
	state_ = ErrorOccurred();
	msg_ = http_.message();
    }
}

