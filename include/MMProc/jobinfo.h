#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "iopar.h"

class HostData;

/*!
\brief All information on a job.
*/

mExpClass(MMProc) JobInfo
{
    friend class	JobRunner;
public:

    enum State		{ ToDo, Scheduled, Preparing, Working, WrappingUp, 
			  Paused, Completed, JobFailed, HostFailed };
    			//!< This is always the 'real' status
			//!< This as opposed to 'requested' or whatever

    			JobInfo( int dnr )
			    : descnr_(dnr)
			    , state_(ToDo)
			    , statusmsg_("Scheduled")
			    , infomsg_("")
			    , jobfailures_(0)
			    , hstfailures_(0)
			    , hostdata_(0)
			    , osprocid_(-1)
			    , starttime_(0)
			    , recvtime_(0) {}

    int			descnr_;	//!< JobdescProv's job number
    State		state_;
    int			jobfailures_;	//!< Failures probably caused by job
    int			hstfailures_;	//!< Failures probably caused by host
    int			nrdone_;
    int			osprocid_;	//!< OS process ID
    BufferString	statusmsg_;	
    BufferString	infomsg_;	//!< Error msg if failure

    const HostData*	hostdata_;	//!< Host currently working on job

protected:

    int			starttime_;
    int			recvtime_;
};
