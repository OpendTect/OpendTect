#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2013
________________________________________________________________________

-*/

#include "basicmod.h"

#include "bufstringset.h"
#include "od_iosfwd.h"
#include "uistring.h"

mFDQtclass(QProcess);
class qstreambuf;
class StreamProvider;

namespace OS
{

enum LaunchType { Wait4Finish, RunInBG, Batch, BatchWait };
enum KeyStyle	{ NewStyle, OldStyle };

}

inline bool isOldStyle( OS::KeyStyle ks ) { return ks == OS::OldStyle; }


namespace OS
{

/*!\brief Specifies how to execute a command */


mExpClass(Basic) CommandExecPars
{
public:
			CommandExecPars( LaunchType lt=Wait4Finish )
			    : launchtype_(lt)
			    , createstreams_(false)
			    , needmonitor_(false)
			    , prioritylevel_(lt>=Batch ? -1.0f : 0.0f)
			    , isconsoleuiprog_(false)
                , runasadmin_(false)     {}

    mDefSetupClssMemb(CommandExecPars,LaunchType,launchtype);
    mDefSetupClssMemb(CommandExecPars,bool,createstreams);
    mDefSetupClssMemb(CommandExecPars,bool,needmonitor);
    mDefSetupClssMemb(CommandExecPars,BufferString,monitorfnm);
			    //!< when empty, will be generated (if needed)

    mDefSetupClssMemb(CommandExecPars,float,prioritylevel);
			    //!< -1=lowest, 0=normal, 1=highest (administrator)

    mDefSetupClssMemb(CommandExecPars,bool,isconsoleuiprog);
			    //!< program uses text-based stdin console input
			    //!< if true, will ignore monitor settings

    mDefSetupClssMemb(CommandExecPars,bool,runasadmin);
                //!< launch in a new session as admin
                //!< Windows only.

    mDefSetupClssMemb(CommandExecPars,BufferString,workingdir);

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;
    void		removeFromPar(IOPar&) const;

    static const char*	sKeyPriority()	{ return "priority"; }

    static const StepInterval<int>	cMachineUserPriorityRange(bool iswin);
			/*!< Restricted to OS-specific user available range
			     Unix: 0-19 (0=normal)
			     Windows: 6-8 (8=normal)
			  */

    static int		getMachinePriority(float priolevel,bool iswin);
};



class CommandLauncher;

/*!\brief Encapsulates an actual command to execute + the machine to run it on

The default remote execution command is ssh.

 */

mExpClass(Basic) MachineCommand
{
public:

			MachineCommand( const char* prognm=nullptr )
			    : prognm_(prognm)		{}
			MachineCommand( const char* prognm,
					const BufferStringSet& arguments )
			    : prognm_(prognm)
			    , args_(arguments)		{}
			MachineCommand(const char* prognm,const char* arg1,
				       const char* arg2=0,const char* arg3=0,
				       const char* arg4=0,const char* arg5=0);
			MachineCommand(const char* prognm,bool isolated);
			MachineCommand(const MachineCommand&,bool isolated);

    const char*		program() const			{ return prognm_; }
    const BufferStringSet& args() const			{ return args_; }
    BufferString	toString(const CommandExecPars* =nullptr) const;
			//!< Only for messaging purposes

    MachineCommand&	setProgram( const char* pn )
			{ prognm_.set( pn ); return *this; }
    MachineCommand&	addArg(const char*);
    MachineCommand&	addArgs(const BufferStringSet&);
    MachineCommand&	addFlag( const char* flg, KeyStyle ks=NewStyle )
			{ return addKeyedArg(flg,nullptr,ks); }
    MachineCommand&	addKeyedArg(const char* ky,const char* valstr,
				    KeyStyle ks=NewStyle);
    MachineCommand&	addPipe()	{ return addArg("|"); }
    MachineCommand&	addFileRedirect(const char* fnm,int stdcode=0,
					bool append=false);
			/*!< stdcode=0: '>'; stdcode=1: '1>'; stdcode=2: '2>'
			     stdcode=3: '> fnm 2>&1';
			     append: '>>'; otherwise: '>'	*/

			// convenience:
    template <class T>
    MachineCommand&	addArg( const T& t )	{ return addArg(::toString(t));}
    template <class T>
    MachineCommand&	addKeyedArg( const char* ky, const T& t,
				     KeyStyle ks=NewStyle )
			{ return addKeyedArg(ky,::toString(t),ks);}

    bool		hostIsWindows() const		{ return hostiswin_; }
    void		setHostIsWindows( bool yn )	{ hostiswin_ = yn; }
    const char*		hostName() const		{ return hname_; }
    void		setHostName( const char* hnm )	{ hname_ = hnm; }
    const char*		remExec() const			{ return remexec_; }
    void		setRemExec( const char* sh )	{ remexec_ = sh; }

    bool		isBad() const		{ return prognm_.isEmpty(); }
    bool		hasHostName() const	{ return !hname_.isEmpty(); }

    static const char*	defaultRemExec()	{ return defremexec_; }
    static void		setDefaultRemExec( const char* s ) { defremexec_ = s; }

    static const char*	odRemExecCmd()		{ return "od_remexec"; }
    static const char*	sKeyRemoteHost()	{ return "machine"; }
    static const char*	sKeyRemoteCmd()		{ return "cmd"; }
    static const char*	sKeyMasterHost()	{ return "masterhost"; }
    static const char*	sKeyMasterPort()	{ return "masterport"; }
    static const char*	sKeyJobID()		{ return "jobid"; }
    static const char*	sKeyIsolationScript()	{ return "isolatefnm"; }

    bool		execute(LaunchType lt=Wait4Finish,
				const char* workdir=nullptr);
    bool		execute(BufferString& output_stdout,
				BufferString* output_stderr=nullptr,
				const char* workdir=nullptr);
				//!< run &, wait until finished, catch output
    bool		execute(const CommandExecPars&);

    BufferString	runAndCollectOutput(BufferString* errmsg=nullptr);
				//!< for quick get-me-the-output-of-this-command

private:

    void		setIsolated(const char* prognm);
    MachineCommand	getExecCommand(const CommandExecPars* =nullptr) const;
    void		addShellIfNeeded();
			/*!<Analyses the cmd and looks for pipes or redirects.
			    If these are found, the cmd is converted to a
			    shell command. */

    BufferString	prognm_;
    BufferStringSet	args_;
    bool		hostiswin_		= __iswin__;
    BufferString	hname_;
    BufferString	remexec_		= defremexec_;
    bool		needshell_ = false;

    static BufferString	defremexec_;

    friend class CommandLauncher;
    friend class ::StreamProvider;

public:

    //Only for expert usage
    static void		setIsolationScript(const char*);
    static const char*	getIsolationScriptFnm();

};


/*!\brief Launches machine commands */

mExpClass(Basic) CommandLauncher
{ mODTextTranslationClass(CommandLauncher);
public:
			CommandLauncher(const MachineCommand&);
			~CommandLauncher();

    void		set(const MachineCommand&);

    bool		execute(LaunchType lt=Wait4Finish,
				const char* workdir=nullptr);
    bool		execute(BufferString& output_stdout,
				BufferString* output_stderr=nullptr,
				const char* workdir=nullptr);
				//!< run &, wait until finished, catch output
    bool		execute(const CommandExecPars&);
    bool		startServer(bool inpythonenv=false,
				    double maxwaittm=20 /* seconds */);

    PID_Type		processID() const;
    const char*		monitorFileName() const	{ return monitorfnm_; }
    uiString		errorMsg() const	{ return errmsg_; }

    od_istream*		getStdOutput() { return stdoutput_; }
    od_istream*		getStdError() { return stderror_; }
    od_ostream*		getStdInput() { return stdinput_; }

    static bool		openTerminal(const char* workdir);

protected:

    void		reset();
    bool		doExecute(const MachineCommand&,const CommandExecPars&);
    int			catchError();
    bool		startDetached(const MachineCommand&,
				      bool inconsole=false,
				      const char* workingdir=nullptr);
    void		startMonitor();
    static void		manageQProcess(QProcess*);
			/*!<Add a QProcess and it will be deleted one day. */

    MachineCommand	machcmd_;
    BufferString	monitorfnm_;
    bool		redirectoutput_;

    BufferString	progvwrcmd_;
    uiString		errmsg_;
    const BufferString	odprogressviewer_;

    QProcess*		process_;
    PID_Type		pid_;

    od_istream*		stdoutput_;
    od_istream*		stderror_;
    od_ostream*		stdinput_;

    qstreambuf*		stdoutputbuf_;
    qstreambuf*		stderrorbuf_;
    qstreambuf*		stdinputbuf_;

};

} // namespace OS


namespace OD
{

/*! Shows an error message in a separate (small) program */
mGlobal(Basic) void DisplayErrorMessage(const char*);

} // namespace OD
