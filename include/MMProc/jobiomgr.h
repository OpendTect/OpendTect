#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmprocmod.h"

#include "networkcommon.h"
#include "oscommand.h"

class CommandString;
class FilePath;
class HostData;
class JobInfo;
class JobIOHandler;
template <class T> class ObjQueue;

/*!
\brief Encapsulates status message from a running client.

 * Running clients report back to the primary host on a regular basis.
 * Whenever a client contacts the primary host, whatever it has
 * to say is put into a StatusInfo structure.
 * These are put in a mutexed queue in order to keep a strict separation
 * between the communication thread and the GUI/manager thread.
 *
*/

mExpClass(MMProc) StatusInfo
{
public:
			StatusInfo(char tg,int desc,int stat,int pid,
				   const char* mg,const char* hostname,
				   int time);
			~StatusInfo();

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

			JobIOMgr(PortNr_Type firstport=19345,int niceval=19);
    virtual		~JobIOMgr();

    Network::Authority	authority() const;

    const char*		peekMsg()  { if ( msg_.size() ) return msg_; return 0; }
    void		fetchMsg( BufferString& bs )	{ bs = msg_; msg_ = "";}

    bool		startProg(const char*,IOPar&,const FilePath&,
				  const JobInfo&,const char*);

    void		setNiceNess( int n )		{ niceval_ = n; }
    void		reqModeForJob(const JobInfo&,Mode);
    void		removeJob(const char*,int);
    bool		isReady() const;

    ObjQueue<StatusInfo>& statusQueue();

    static bool		mkIOParFile(const FilePath& basefnm,
				    const HostData&,const IOPar&,
				    FilePath&,BufferString& msg);
    static BufferString mkRexecCmd(const char* prognm,
				   const HostData& machine,
				   const HostData& localhost);
			/*!< Sets up the command to be executed with
			     GetScriptDir()/exec_prog script.
			     This latter ensures all the environment is
			     restored on the remote machine ( rsh/ssh
			     do NOT forward environment variables such as
			     LD_LIBRARY_PATH ).
			     */

protected:

    JobIOHandler&	iohdlr_;
    BufferString	msg_;
    int			niceval_;
    OS::CommandExecPars execpars_;

    bool		mkIOParFile(FilePath&,const FilePath& basefnm,
				    const HostData&,const IOPar&);
    void		mkCommand(OS::MachineCommand&,const HostData&,
				  const char* progname,const FilePath& basefp,
				  const FilePath& iopfp,const JobInfo&,
				  const char* rshcomm);
private:

    void		setRexecCmd(const char* prognm,
				    const HostData& machine,
				    const HostData& localhost,
				    OS::MachineCommand&) const;

};


mGlobal(MMProc) const OD::String& getTempBaseNm();
mGlobal(MMProc) int mkTmpFileNr();
mGlobal(MMProc) int& MMJob_getTempFileNr();

 //Deprecated, will be removed after 6.6

mClass(MMProc) CommandString
{
public:
			CommandString(const HostData& targetmachine,
				      const char* init=0);
			~CommandString();

    CommandString&	operator=(const char*);

    void		addFlag(const char* flag,const char* value);
    void		addFlag(const char* flag,int value);
    void		addFilePath(const FilePath&);

    const OD::String&	string()		{ return cmd_; }

private:

    void		add(const char*);

    BufferString	cmd_;
    const HostData&	hstdata_;

};
