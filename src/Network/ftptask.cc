/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: ftptask.cc,v 1.2 2010-06-30 12:45:00 cvsnanne Exp $";

#include "ftptask.h"
#include "odftp.h"


FtpTask::FtpTask( ODFtp& ftp )
    : Executor("Ftp-ing file(s)")
    , nrdone_(0)
    , totalnr_(0)
    , ftp_(ftp)
    , state_(MoreToDo())
{
    ftp_.dataTransferProgress.notify( mCB(this,FtpTask,progressCB) );
    ftp_.done.notify( mCB(this,FtpTask,doneCB) );
}


FtpTask::~FtpTask()
{
    ftp_.dataTransferProgress.remove( mCB(this,FtpTask,progressCB) );
    ftp_.done.remove( mCB(this,FtpTask,doneCB) );
}


int FtpTask::nextStep()
{ return state_; }


void FtpTask::controlWork( Control c )
{
    msg_ = "Data transfer aborted";
    Task::controlWork( c );
    ftp_.abort();
}


void FtpTask::progressCB( CallBacker* )
{
    nrdone_ = ftp_.nrDone();
    totalnr_ = ftp_.totalNr();
    state_ = MoreToDo();
}


void FtpTask::doneCB( CallBacker* )
{ state_ = ftp_.isOK() ? Finished() : ErrorOccurred(); }
