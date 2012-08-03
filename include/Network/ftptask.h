#ifndef ftptask_h
#define ftptask_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2010
 RCS:		$Id: ftptask.h,v 1.3 2012-08-03 13:00:31 cvskris Exp $
________________________________________________________________________

-*/

#include "networkmod.h"
#include "executor.h"

class ODFtp;

mClass(Network) FtpTask : public Executor
{
public:
			FtpTask(ODFtp&);
    			~FtpTask();

    int			nextStep();
    virtual void	controlWork(Control);

    const char*		message() const		{ return msg_; }
    od_int64		totalNr() const		{ return totalnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    const char*		nrDoneText() const	{ return "Bytes done"; }

protected:

    void		progressCB(CallBacker*);
    void		doneCB(CallBacker*);

    ODFtp&		ftp_;

    od_int64		totalnr_;
    od_int64		nrdone_;
    BufferString	msg_;

    int			state_;
};

#endif

