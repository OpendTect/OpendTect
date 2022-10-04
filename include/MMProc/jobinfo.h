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

			JobInfo(int dnr);
			~JobInfo();

    int			descnr_;	//!< JobdescProv's job number
    State		state_ = ToDo;
    int			jobfailures_ = 0; //!< Failures probably caused by job
    int			hstfailures_ = 0; //!< Failures probably caused by host
    int			nrdone_ = -1;
    int			osprocid_ = -1; //!< OS process ID
    BufferString	statusmsg_ = "Scheduled";
    BufferString	infomsg_;	//!< Error msg if failure

    const HostData*	hostdata_ = nullptr; //!< Host currently working on job

protected:

    int			starttime_ = 0;
    int			recvtime_ = 0;
};
