#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "bufstringset.h"
#include "odprocess.h"
#include "od_iosfwd.h"
#include "ptrman.h"
#include "uistring.h"

mFDQtclass(QTextStream);
class StreamProvider;

namespace OS
{

enum LaunchType { Wait4Finish, RunInBG, Batch, BatchWait };
enum KeyStyle	{ NewStyle, OldStyle };

mDeprecatedObs inline bool isBatchProg( OS::LaunchType lt );
inline bool isOldStyle( OS::KeyStyle ks ) { return ks == OS::OldStyle; }

/*!\brief Specifies how to execute a command */


mExpClass(Basic) CommandExecPars
{
public:
			CommandExecPars(LaunchType lt=Wait4Finish);
			~CommandExecPars();

    mDeprecated		("Use CommandExecPars(LaunchType) instead")
    explicit		CommandExecPars(bool isbatchprog);

    mDefSetupClssMemb(CommandExecPars,LaunchType,launchtype);
    CommandExecPars&	createstreams(bool);
    mDefSetupClssMembInit(CommandExecPars,bool,needmonitor,false);
    mDefSetupClssMemb(CommandExecPars,BufferString,monitorfnm);
			    //!< when empty, will be generated (if needed)
    mDefSetupClssMembInit(CommandExecPars,OD::Process::ChannelMode,channelmode,
			  OD::Process::ChannelMode::SeparateChannels);
    mDefSetupClssMembInit(CommandExecPars,OD::Process::InputChannelMode,
			  inputchmode,
			  OD::Process::InputChannelMode::ManagedInputChannel);
    CommandExecPars&	consolestdout(bool);
			    //!< The QProcess stdout is redirected to stdout
    CommandExecPars&	consolestderr(bool);
			    //!< The QProcess stderr is redirected to stderr
    CommandExecPars&	stdoutfnm(const char*);
			    //!< The QProcess stdout is redirected to a file
    CommandExecPars&	stderrfnm(const char*);
			    //!< The QProcess stderr is redirected to a file

    mDefSetupClssMemb(CommandExecPars,float,prioritylevel);
			    //!< -1=lowest, 0=normal, 1=highest (administrator)

    mDefSetupClssMembInit(CommandExecPars,bool,isconsoleuiprog,false);
			    //!< program uses text-based stdin console input
			    //!< if true, will ignore monitor settings

    mDefSetupClssMembInit(CommandExecPars,bool,runasadmin,false);
		//!< launch in a new session as admin
		//!< Windows only.

    mDefSetupClssMemb(CommandExecPars,BufferString,workingdir);
    mDefSetupClssMemb(CommandExecPars,BufferStringSet,environment);

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

    bool		createstreams_		= false;
    BufferString	stdoutfnm_;
    BufferString	stderrfnm_;
    bool		consolestdout_		= false;
    bool		consolestderr_		= false;

public:
    //Internal usage of CommandLauncher
    CommandExecPars&	txtbufstdout(bool);
    CommandExecPars&	txtbufstderr(bool);
    bool		txtbufstdout_		= false;
    bool		txtbufstderr_		= false;
};



class CommandLauncher;
class MachineCommand;

/*!\brief Encapsulates an actual command to execute + the machine to run it on

The default remote execution command is ssh.

 */

mExpClass(Basic) MachineCommand
{
public:

			MachineCommand(const char* prognm=nullptr);
			MachineCommand(const char* prognm,
				       const BufferStringSet & arguments);
			MachineCommand(const char* prognm,const char* arg1,
				       const char* arg2=nullptr,
				       const char* arg3=nullptr,
				       const char* arg4=nullptr,
				       const char* arg5=nullptr);
			MachineCommand(const char* prognm,bool isolated);
			MachineCommand(const MachineCommand&);
			MachineCommand(const MachineCommand&,bool isolated);
			~MachineCommand();

    MachineCommand&	operator=(const MachineCommand&);

    const char*		program() const			{ return prognm_; }
    const BufferStringSet& args() const			{ return args_; }
    BufferString	toString(const CommandExecPars* =nullptr) const;
			//!< Only for messaging purposes

    MachineCommand&	setProgram( const char* pn )
			{ prognm_.set( pn ); return *this; }
    MachineCommand&	addArg(const char*);
    MachineCommand&	addArgs(const BufferStringSet&);
    MachineCommand&	addFlag( const char* flg, KeyStyle ks=NewStyle )
			{ return addKeyedArg(flg,(const char*)nullptr,ks); }
    MachineCommand&	addKeyedArg(const char* ky,const char* valstr,
				    KeyStyle ks=NewStyle);
    MachineCommand&	addPipe()	{ return addArg("|"); }
    MachineCommand&	addPipedCommand(const MachineCommand&);
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
    MachineCommand&	setHostIsWindows(bool yn);
    const char*		hostName() const		{ return hname_; }
    MachineCommand&	setHostName(const char*);
			/*<! Host name for remote execution only,
			     should be a valid host name or IP address */
    const char*		remExec() const			{ return remexec_; }
    MachineCommand&	setRemExec(const char* sh);

    const MachineCommand* pipedCommand() const	{ return pipedmc_; }

    bool		isBad() const		{ return prognm_.isEmpty(); }
    bool		hasHostName() const	{ return !hname_.isEmpty(); }

    static const char*	defaultRemExec()	{ return defremexec_; }
    static void		setDefaultRemExec( const char* s ) { defremexec_ = s; }
    static const char*	odRemExecCmd();
    static const char*	sKeyRemoteHost()	{ return "machine"; }
    static const char*	sKeyRemoteCmd()		{ return "cmd"; }
    static const char*	sKeyPrimaryHost()	{ return "primaryhost"; }
    static const char*	sKeyPrimaryPort()	{ return "primaryport"; }
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
				//!
    uiString		errorMsg() const	{ return errmsg_; }

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
    bool		needshell_		= false;
    uiString		errmsg_;
    MachineCommand*	pipedmc_		= nullptr;

    static BufferString	defremexec_;

    friend class CommandLauncher;
    friend class ::StreamProvider;

public:

    mDeprecated("Use sKeyPrimaryHost()")
    static const char*	sKeyMasterHost()	{ return "primaryhost"; }
    mDeprecated("Use sKeyPrimaryPort()")
    static const char*	sKeyMasterPort()	{ return "primaryport"; }

    //Only for expert usage
    static void		setIsolationScript(const char*);
    static const char*	getIsolationScriptFnm();

};


/*!\brief Launches machine commands */

mExpClass(Basic) CommandLauncher : public CallBacker
{ mODTextTranslationClass(CommandLauncher);
public:
			CommandLauncher(const MachineCommand&);
			~CommandLauncher();
			mOD_DisableCopy(CommandLauncher);

    void		set(const MachineCommand&);

    bool		execute(LaunchType lt=Wait4Finish,
				const char* workdir=nullptr);
    bool		execute(BufferString& output_stdout,
				BufferString* output_stderr=nullptr,
				const char* workdir=nullptr);
				//!< run &, wait until finished, catch output
    bool		execute(const CommandExecPars&);
    bool		startServer(bool inpythonenv=false,
				    const char* stdoutfnm =nullptr,
				    const char* stderrfnm =nullptr,
				    bool consolestdout=false,
				    bool consolestderr=false,
				    double maxwaittm=20 /* seconds */);

    ConstRefMan<OD::Process>	process() const;
    RefMan<OD::Process> process();

    PID_Type		processID() const;
    int			exitCode() const;
    OD::Process::ExitStatus exitStatus() const;
    const char*		monitorFileName() const	{ return monitorfnm_; }
    uiString		errorMsg() const	{ return errmsg_; }

    bool		hasStdInput() const;
    bool		hasStdOutput() const;
    bool		hasStdError() const;

    od_int64		write(const char*,od_int64 maxsz);
    od_int64		write(const char*);
				    //!< String must be null terminated
    bool		getLine(BufferString&,bool stdoutstrm=true,
				bool* newline_found=nullptr);
    bool		getAll(BufferStringSet&,bool stdoutstrm=true);
    bool		getAll(BufferString&,bool stdoutstrm=true);

    Notifier<CommandLauncher> started;
    CNotifier<CommandLauncher,std::pair<int,OD::Process::ExitStatus> > finished;
    CNotifier<CommandLauncher,OD::Process::Error> errorOccurred;
    CNotifier<CommandLauncher,OD::Process::State> stateChanged;

    static bool		openTerminal(const char* cmd,
				     const BufferStringSet* args=nullptr,
				     BufferString* errmsg =nullptr,
				     uiString* launchermsg =nullptr,
				     const char* workdir =nullptr);

protected:

    void		reset();
    bool		doExecute(const MachineCommand&,const CommandExecPars&);
    const OD::Process*	getReadProcess() const;
    OD::Process*	getReadProcess();
    int			catchError(bool pipecmd=false);
    bool		startDetached(bool inconsole=false);
    void		startMonitor();

    void		startedCB(CallBacker*);
    void		finishedCB(CallBacker*);
    void		errorOccurredCB(CallBacker*);
    void		stateChangedCB(CallBacker*);

    static void		manageODProcess(OD::Process*);
			/*!<Add a OD::Process and it will be deleted one day. */

    MachineCommand	machcmd_;
    BufferString	monitorfnm_;

    BufferString	progvwrcmd_;
    uiString		errmsg_;
    const BufferString	odprogressviewer_;

    RefMan<OD::Process> process_;
    RefMan<OD::Process> pipeprocess_;
    PID_Type		pid_		= mUdf(PID_Type);
    int			exitcode_	= mUdf(int);
    OD::Process::ExitStatus exitstatus_ = OD::Process::ExitStatus::NormalExit;

};

} // namespace OS


namespace OD
{

/*! Shows an error message in a separate (small) program */
mGlobal(Basic) void DisplayErrorMessage(const char*);

} // namespace OD
