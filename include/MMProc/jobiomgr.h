#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Lammertink
 Date:		Oct 2004
________________________________________________________________________

-*/

#include "mmprocmod.h"

#include "networkcommon.h"
#include "oscommand.h"

class HostData;
class JobInfo;
class JobIOHandler;
class od_ostream;
namespace File { class Path; }
template <class T> class ObjQueue;

/*!
\brief Encapsulates status message from a running client.

 * Running clients report back to the master on a regular basis.
 * Whenever a client contacts the master, whatever it has
 * to say is put into a StatusInfo structure.
 * These are put in a mutexed queue in order to keep a strict separation
 * between the communication thread and the GUI/manager thread.
 *
*/

mExpClass(MMProc) StatusInfo
{
public:
			StatusInfo( char tg, int desc, int stat, int pid,
				    const char* mg, const char* hostname,
				    int time )
			    : tag(tg), descnr(desc), status(stat), msg(mg)
			    , hostnm(hostname), timestamp(time), procid(pid) {}

    char		tag;
    int			descnr;
    int			status;
    int			timestamp;
    int			procid;
    BufferString	hostnm;
    BufferString	msg;
};


/*!
\brief Handles starting & stopping of jobs on client machines. Sets up a
separate thread to maintain contact with client.
*/

mExpClass(MMProc) JobIOMgr : public CallBacker
{
public:
    enum		Mode { Work, Pause, Stop };

			JobIOMgr(PortNr_Type firstport=19345,
				 float priority=-1.f,
				 od_ostream* logstrm=nullptr);
    virtual		~JobIOMgr();

    const char*		peekMsg()  { if ( msg_.size() ) return msg_; return 0; }
    void		fetchMsg( BufferString& bs )	{ bs = msg_; msg_ = "";}

    bool		startProg(const char*,IOPar&,const File::Path&,
				  const JobInfo&,const char*);

    void		setPriority( float p );
    void		reqModeForJob(const JobInfo&,Mode);
    void		removeJob(const char*,int);
    bool		isReady() const;

    ObjQueue<StatusInfo>& statusQueue();

    static bool		mkIOParFile(const File::Path& basefnm,
				    const HostData&,const IOPar&,
				    File::Path&,BufferString& msg);

protected:

    JobIOHandler&	iohdlr_;
    BufferString	msg_;
    OS::CommandExecPars execpars_;
    od_ostream*		logstrm_;

    void		mkCommand(OS::MachineCommand&,const HostData&,
				  const char* progname,const File::Path& basefp,
				  const File::Path& iopfp,const JobInfo&,
				  const char* rshcomm);
private:

    void		setRexecCmd(const char* prognm,
				    const HostData& machine,
				    const HostData& localhost,
				    OS::MachineCommand&) const;
			/*!< Sets up the command to be executed with
			     GetShellScript(exec_prog) script.
			     This latter ensures all the environment is
			     restored on the remote machine ( rsh/ssh
			     do NOT forward environment variables such as
			     LD_LIBRARY_PATH ).
			     */

};


mGlobal(MMProc) const OD::String& getTempBaseNm();
mGlobal(MMProc) int mkTmpFileNr();

