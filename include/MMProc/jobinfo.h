#ifndef jobinfo_h
#define jobinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2004
 RCS:		$Id: jobinfo.h,v 1.4 2004-11-05 19:24:19 arend Exp $
________________________________________________________________________

-*/

#include "iopar.h"

class HostData;


/*!\brief All info on a job. */

class JobInfo
{
    friend class	JobRunner;
public:

    enum State		{ ToDo, Working, Paused, Completed, Failed };
    			//!< This is always the 'real' status
			//!< This as opposed to 'requested' or whatever

    			JobInfo( int dnr )
			    : descnr_(dnr)
			    , state_(ToDo)
			    , curmsg_("Scheduled")
			    , attempts_(0)
			    , hostdata_(0)
			    , osprocid_(-1)
			    , timestamp_(0){}

    int			descnr_;	//!< JobdescProv's job number
    State		state_;
    int			attempts_;
    int			nrdone_;
    int			osprocid_;	//!< OS process ID
    BufferString	curmsg_;	//!< If Failed error message

    const HostData*	hostdata_;	//!< Host currently working on job

protected:

    int			timestamp_;
};


#endif
