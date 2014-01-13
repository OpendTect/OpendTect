#ifndef oscommand_h
#define oscommand_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"


/*!\brief Specifies how to execute a command */


mExpClass(Basic) OSCommandExecPars
{
public:
			OSCommandExecPars()
			    : inprogresswindow_(true)
			    , prioritylevel_(-1)		{}

    bool		inprogresswindow_;
			    //!< if not, to logfile (if any)
    BufferString	logfname_;
			    //!< only when !inprogresswindow_
    float		prioritylevel_;
			    //!< -1=lowest, 0=normal, 1=highest (administrator)
};


/*!\brief Can execute OS commands.

Executes command without opening pipe.
The default remote execution command is ssh.

 */

mExpClass(Basic) OSCommand
{
public:

			OSCommand(const char* comm=0,const char* hnm=0);

    inline const char*	command() const			{ return comm_; }
    inline void		setCommand( const char* cm )	{ comm_ = cm; }
    inline const char*	hostName() const		{ return hname_; }
    inline void		setHostName( const char* hnm )	{ hname_ = hnm; }
    inline const char*	remExec() const			{ return remexec_; }
    inline void		setRemExec( const char* sh )	{ remexec_ = sh; }

    inline bool		isBad() const		{ return comm_.isEmpty(); }

    bool		set(const char*,bool lookforhostname=true);
			//!< returns !isBad()
    const char*		get() const;

    bool		execute(bool inconsole=false,bool inbg=false) const;
    bool		execute(OSCommandExecPars,bool) const;

    static const char*	defaultRemExec()	{ return defremexec_; }
    static void		setDefaultRemExec( const char* s ) { defremexec_ = s; }

    static const char*	extractHostName(const char*,BufferString&);
			//!< returns remaining part

protected:

    BufferString	comm_;
    BufferString	hname_;
    BufferString	remexec_;
    static BufferString	defremexec_;

    void		mkOSCmd(bool,BufferString&) const;
#ifdef __win__
    void		mkConsoleCmd(BufferString& comm) const;
#endif
};

/*!\brief A single interface to launch commands/processes and OpendTect programs

*/

mExpClass(Basic) CommandLauncher
{
public:
			CommandLauncher(const char* cmd,const char* par);
			~CommandLauncher();
			 
    bool		execute(const OSCommandExecPars&
			     pars=OSCommandExecPars(), bool isODprogram=true);

    int			getProcessID() const;
    const char*		errorMsg() const { return errmsg_; }

protected:
    
    bool		doExecute(bool inwindow, bool waitforfinish);

    void		makeFullCommand();
    void		makeConsoleCommand();

    BufferString	command_;
    BufferString	parameters_;
    BufferString	fullcommand_;
    int			processid_;
    BufferString	errmsg_;
};

//! Execute command on local host
mGlobal(Basic) bool ExecOSCmd(const char*,bool inconsole=false,bool inbg=false);

//! Execute od program via the startup script
mGlobal(Basic) bool ExecODProgram(const char* prognm,const char* filenm,
				  int nicelvl=19,const char* args=0);


#endif
