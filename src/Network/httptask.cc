/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Oct 2011
 RCS:		$Id: httptask.cc,v 1.1 2011-10-24 05:24:55 cvsumesh Exp $
________________________________________________________________________

-*/

#include "httptask.h"
#include "odhttp.h"

HttpTask::HttpTask( ODHttp& http )
    : Executor("Http-ing file(s)")
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
{ return state_; }


void HttpTask::controlWork( Control ctrl )
{
    if ( ctrl == Task::Run )
	return;

    msg_ = "Data transfer aborted";
    http_.abort();
    state_ = ErrorOccurred();
    Task::controlWork( ctrl );
}


void HttpTask::progressCB( CallBacker* )
{
    nrdone_ = http_.nrDone();
    totalnr_ = http_.totalNr();
    state_ = MoreToDo();
}


void HttpTask::doneCB( CallBacker* )
{ state_ = http_.isOK() ? Finished() : ErrorOccurred(); }
