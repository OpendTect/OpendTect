#ifndef jobinfo_h
#define jobinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2004
 RCS:		$Id: jobinfo.h,v 1.1 2004-10-25 07:26:20 bert Exp $
________________________________________________________________________

-*/

#include "iopar.h"

class HostData;


/*!\brief Processing job: All info on a job. */

class JobInfo
{
public:

    enum State		{ ToDo, Working, Paused, Completed, Failed };
    			//!< This is always the 'real' status
			//!< This as opposed to 'requested' or whatever

    			JobInfo( int dnr )
			    : descnr_(dnr)
			    , state_(ToDo)
			    , curmsg("Scheduled")
			    , attempts_(0)
			    , hostdata_(0)
			    , osprocid_(-1)		{}

    int			descnr_;	//!< JobdescProv's job number
    State		state_;
    int			attempts_;
    int			osprocid_;	//!< OS process ID
    BufferString	curmsg_;	//!< If Failed error message

    const HostData*	hostdata_;	//!< Host currently working on job
};


#endif
