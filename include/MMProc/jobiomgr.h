#ifndef jobiomgr_h
#define jobiomgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Lammertink
 Date:		Oct 2004
 RCS:		$Id: jobiomgr.h,v 1.5 2004-11-05 13:21:33 arend Exp $
________________________________________________________________________

-*/

#include "general.h"
#include "callback.h"
//#include "queue.h"

class HostData;
class CommandString;
class JobInfo;
class IOPar;
class HostDataList;
class FilePath;
class JobIOHandler;
template <class T> class ObjQueue;

/*!\brief Encapsulates status message from a running client.
 *
 * Running clients report back to the master on a regular basis.
 * Whenever a client contacts the master, whatever it has
 * to say is put into a StatusInfo structure.
 * These are put in a mutexed queue in order to keep a strict separation
 * between the communication thread and the GUI/manager thread.
 *
*/
class StatusInfo
{
public:
			StatusInfo( char tg, int desc, int stat,
				    const BufferString& mg, 
				    const BufferString& hostname, int time )
			    : tag(tg), descnr(desc), status(stat), msg(mg)
			    , hostnm(hostname), timestamp(time) {}

    char		tag;
    int			descnr;
    int			status;
    int			timestamp;
    BufferString	hostnm;
    BufferString	msg;
};


/*!\brief Handles starting&stopping of jobs on client machines
 
  sets up a separate thread to maintain contact with client.

*/
class JobIOMgr : public CallBacker
{
public:
    enum		Mode { Work, Pause, Stop };

    			JobIOMgr( int firstport=19345, int niceval=19 );
    virtual		~JobIOMgr();

    void		fetchMsg( BufferString& bs )	{ bs = msg_; msg_ = "";}


    bool		startProg( const char* progname, const HostData&,
			       IOPar&, const FilePath& basefp, const JobInfo&,
			       const char* rshcomm );

    void		setNiceNess( int n )		{ niceval_ = n; }
    void		reqModeForJob( const JobInfo&, Mode );
    void		removeJob( const char* hostnm, int descnr );

    ObjQueue<StatusInfo>& statusQueue(); 

protected:

    JobIOHandler&	iohdlr_;
    BufferString	msg_;
    int			niceval_;

    bool 		mkIOParFile( FilePath&, const FilePath& basefnm,
				     const HostData&, const IOPar&);
    void 		mkCommand( CommandString&, const HostData&,
				   const char* progname, const FilePath& basefp,
				   const FilePath& iopfp, const JobInfo&,
				   const char* rshcomm );
};


#endif
