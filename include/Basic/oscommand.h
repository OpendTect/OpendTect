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

namespace OS
{


enum LaunchType	{ Wait4Finish, RunInBG };


/*!\brief Specifies how to execute a command */


mExpClass(Basic) CommandExecPars
{
public:
			CommandExecPars( bool isodprog=true )
			    : needmonitor_(false)
			    , prioritylevel_(isodprog ? -1.0f : 0.0f)
			    , launchtype_(isodprog?RunInBG:Wait4Finish)
			    , isconsoleuiprog_(false)	{}

    mDefSetupClssMemb(CommandExecPars,bool,needmonitor);
    mDefSetupClssMemb(CommandExecPars,BufferString,monitorfnm);
			    //!< when empty, will be generated (if needed)

    mDefSetupClssMemb(CommandExecPars,LaunchType,launchtype);

    mDefSetupClssMemb(CommandExecPars,float,prioritylevel);
			    //!< -1=lowest, 0=normal, 1=highest (administrator)

    mDefSetupClssMemb(CommandExecPars,bool,isconsoleuiprog);
			    //!< program uses text-based stdin console input
			    //!< if true, will ignore monitor settings

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;
    void		removeFromPar(IOPar&) const;
};


/*!\brief Encapsulates an actual command to execute + the machine to run it on

The default remote execution command is ssh.

 */

mExpClass(Basic) MachineCommand
{
public:

			MachineCommand(const char* comm=0);
			MachineCommand(const char* comm,const char* hnm);

    inline const char*	command() const			{ return comm_; }
    inline void		setCommand( const char* cm )	{ comm_ = cm; }
    inline const char*	hostName() const		{ return hname_; }
    inline void		setHostName( const char* hnm )	{ hname_ = hnm; }
    inline const char*	remExec() const			{ return remexec_; }
    inline void		setRemExec( const char* sh )	{ remexec_ = sh; }

    inline bool		isBad() const		{ return comm_.isEmpty(); }

    bool		setFromSingleStringRep(const char*,
						bool ignorehostname=false);
						//!< returns !isBad()
    const char*		getSingleStringRep() const;

    bool		hasHostName() const	{ return !hname_.isEmpty(); }

    static const char*	defaultRemExec()	{ return defremexec_; }
    static void		setDefaultRemExec( const char* s ) { defremexec_ = s; }

    static const char*	extractHostName(const char*,BufferString&);
			//!< returns remaining part
    BufferString	getLocalCommand() const;

protected:

    BufferString	comm_;
    BufferString	hname_;
    BufferString	remexec_;

    static BufferString	defremexec_;

};


/*!\brief Launches machine commands */

mExpClass(Basic) CommandLauncher
{
public:
			CommandLauncher(const MachineCommand&);

    void		set(const MachineCommand&);

    bool		execute(const CommandExecPars& pars);

    int			processID() const	{ return processid_; }
    const char*		monitorFileName() const	{ return monitorfnm_; }
    const char*		errorMsg() const	{ return errmsg_; }

protected:

    void		reset();
    bool		doExecute(const char* comm,bool wait4finish,
							bool incosole = false);

    MachineCommand	machcmd_;
    BufferString	monitorfnm_;
    bool		redirectoutput_;
    int			processid_;
    BufferString	progvwrcmd_;
    BufferString	errmsg_;
    const BufferString	odprogressviewer_;
};


/*! convenience function; for specific options use the CommandLauncher */
mGlobal(Basic) bool ExecCommand(const char* cmd,LaunchType lt=Wait4Finish);


} // namespace OS


/*! convenience function executing a program from the OD bindir */
mGlobal(Basic) bool ExecODProgram(const char* prognm,const char* args=0,
				  OS::LaunchType lt=OS::RunInBG);


#endif
