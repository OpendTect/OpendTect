/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "oscommand.h"

#include "commandlineparser.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "odinst.h"
#include "perthreadrepos.h"
#include "pythonaccess.h"
#include "separstr.h"
#include "uistrings.h"

#ifndef OD_NO_QT
# include <QFile>
# include <QProcess>
# include <QTextStream>
# include <QVersionNumber>
#endif

#include <iostream>

#ifdef __win__
# include "shellapi.h"
# include "winutils.h"
# include <windows.h>
# include <stdlib.h>
#endif

const char* OS::MachineCommand::odRemExecCmd()
{
    mDeclStaticString( ret );
    ret = GetODApplicationName( "od_remexec" );
    return ret.str();
}

BufferString OS::MachineCommand::defremexec_( "ssh" );


static const char* sKeyExecPars = "ExecPars";
static const char* sKeyMonitor = "Monitor";
static const char* sKeyProgType = "ProgramType";
static const char* sKeyPriorityLevel = "PriorityLevel";
static const char* sKeyWorkDir = "WorkingDirectory";


//
class ODProcessManager
{
public:
		~ODProcessManager()
		{
		    deleteProcesses();
		}
    void	takeOver( OD::Process* p )
		{
		    processes_.add( p );
		}
    void	deleteProcesses()
		{
#ifndef OD_NO_QT
		    for ( auto* process : processes_ )
			process->close();
		    processes_.erase();
#endif
		}


    static Threads::Lock	lock_;

private:
    RefObjectSet<OD::Process>		processes_;
};

static PtrMan<ODProcessManager> processmanager;
Threads::Lock ODProcessManager::lock_( true );

void DeleteProcesses()
{
    Threads::Locker locker( ODProcessManager::lock_ );
    if ( processmanager )
	processmanager->deleteProcesses();
}

void OS::CommandLauncher::manageODProcess( OD::Process* p )
{
    Threads::Locker locker( ODProcessManager::lock_ );

    if ( !processmanager )
    {
	processmanager = new ODProcessManager;
	NotifyExitProgram( DeleteProcesses );
    }

    processmanager->takeOver( p );
}


// OS::CommandExecPars

OS::CommandExecPars::CommandExecPars( LaunchType lt )
    : launchtype_(lt)
    , prioritylevel_(lt>=Batch ? -1.f : 0.f)
{}


OS::CommandExecPars::CommandExecPars( bool isbatchprog )
    : CommandExecPars(isbatchprog ? RunInBG : Wait4Finish)
{}


OS::CommandExecPars::~CommandExecPars()
{}


void OS::CommandExecPars::usePar( const IOPar& iop )
{
    PtrMan<IOPar> subpar = iop.subselect( sKeyExecPars );
    if ( !subpar || subpar->isEmpty() )
	return;

    FileMultiString fms;
    subpar->get( sKeyMonitor, fms );
    int sz = fms.size();
    if ( sz > 0 )
    {
	needmonitor_ = toBool( fms[0] );
	monitorfnm_ = fms[1];
    }

    fms.setEmpty();
    subpar->get( sKeyProgType, fms );
    sz = fms.size();
    if ( sz > 0 )
    {
	launchtype_ = *fms[0] == 'W' ? Wait4Finish
		    : (fms[0] == "BG" ? RunInBG
				      : (fms[0] == "BW" ? BatchWait : Batch) );
	isconsoleuiprog_ = *fms[0] == 'C';
    }

    subpar->get( sKeyPriorityLevel, prioritylevel_ );

    BufferString workdir;
    if ( subpar->get(sKeyWorkDir,workdir) && !workdir.isEmpty() )
	workingdir_.set( workdir );
}


void OS::CommandExecPars::fillPar( IOPar& iop ) const
{
    iop.removeSubSelection( sKeyExecPars );
    IOPar subiop;

    FileMultiString fms;
    fms += needmonitor_ ? "Yes" : "No";
    fms += monitorfnm_;
    subiop.set( sKeyMonitor, fms );

    fms = launchtype_ == Wait4Finish ? "Wait"
	   : (launchtype_ == RunInBG ? "BG"
	       : (launchtype_ == Batch ? "Batch" : "BW") );
    fms += isconsoleuiprog_ ? "ConsoleUI" : "";
    subiop.set( sKeyProgType, fms );

    subiop.set( sKeyPriorityLevel, prioritylevel_ );
    if ( !workingdir_.isEmpty() )
	subiop.set( sKeyWorkDir, workingdir_ );

    iop.mergeComp( subiop, sKeyExecPars );
}


void OS::CommandExecPars::removeFromPar( IOPar& iop ) const
{
    iop.removeSubSelection( sKeyExecPars );
}


OS::CommandExecPars& OS::CommandExecPars::createstreams( bool yn )
{
    createstreams_ = yn;
    txtbufstdout_  = yn;
    txtbufstderr_  = yn;
    return *this;
}


OS::CommandExecPars& OS::CommandExecPars::consolestdout( bool yn )
{
    if ( yn )
	createstreams_ = true;

    consolestdout_ = yn;
    return *this;
}


OS::CommandExecPars& OS::CommandExecPars::consolestderr( bool yn )
{
    if ( yn )
	createstreams_ = true;

    consolestderr_ = yn;
    return *this;
}


OS::CommandExecPars& OS::CommandExecPars::stdoutfnm( const char* fnm )
{
    createstreams_ = true;
    stdoutfnm_ = fnm;
    return *this;
}


OS::CommandExecPars& OS::CommandExecPars::stderrfnm( const char* fnm )
{
    createstreams_ = true;
    stderrfnm_ = fnm;
    return *this;
}


OS::CommandExecPars& OS::CommandExecPars::txtbufstdout( bool yn )
{
    if ( yn )
	createstreams_ = true;

    txtbufstdout_ = yn;
    return *this;
}


OS::CommandExecPars& OS::CommandExecPars::txtbufstderr( bool yn )
{
    if ( yn )
	createstreams_ = true;

    txtbufstderr_ = yn;
    return *this;
}


const StepInterval<int> OS::CommandExecPars::cMachineUserPriorityRange(
					     bool iswin )
{ return iswin ? StepInterval<int>( 6, 8, 1 ) :StepInterval<int>( 0, 19, 1 ); }


int OS::CommandExecPars::getMachinePriority( float priority, bool iswin )
{
    const StepInterval<int> machpriorg( cMachineUserPriorityRange(iswin) );
    const float scale = iswin ? 1.f : -1.f;

    int machprio = mCast(int, mNINT32(scale * priority * machpriorg.width()) );

    return machprio += iswin ? machpriorg.stop_ : machpriorg.start_;
}


//  OS::MachineCommand

OS::MachineCommand::MachineCommand( const char* prognm )
  : prognm_(prognm)
{}


OS::MachineCommand::MachineCommand( const char* prognm,
				    const BufferStringSet& arguments )
  : prognm_(prognm)
  , args_(arguments)
{}


OS::MachineCommand::MachineCommand( const char* prognm, const char* arg1,
       const char* arg2,const char* arg3, const char* arg4,const char* arg5 )
    : MachineCommand(prognm)
{
    addArg( arg1 ).addArg( arg2 ).addArg( arg3 ).addArg( arg4 ).addArg( arg5 );
}


OS::MachineCommand::MachineCommand( const char* prognm, bool isolated )
{
    if ( isolated )
	setIsolated( prognm );
    else
	*this = MachineCommand( prognm );
}


OS::MachineCommand::MachineCommand( const MachineCommand& oth )
{
    *this = oth;
}


OS::MachineCommand::MachineCommand( const MachineCommand& oth, bool isolated )
{
    if ( isolated )
    {
	setIsolated( oth.program() );
	addArgs( oth.args() );
    }
    else
	*this = oth;
}


OS::MachineCommand::~MachineCommand()
{
    delete pipedmc_;
}


OS::MachineCommand& OS::MachineCommand::operator=( const MachineCommand& oth )
{
    if ( &oth == this )
	return *this;

    prognm_ = oth.prognm_;
    args_ = oth.args_;
    hostiswin_ = oth.hostiswin_;
    hname_ = oth.hname_;
    remexec_ = oth.remexec_;
    needshell_ = oth.needshell_;
    errmsg_ = oth.errmsg_;
    delete pipedmc_;
    pipedmc_ = oth.pipedmc_ ? new MachineCommand( *oth.pipedmc_ ) : nullptr;

    return *this;
}


OS::MachineCommand& OS::MachineCommand::addArg( const char* str )
{
    if ( str && *str )
	args_.add( str );
    return *this;
}


OS::MachineCommand& OS::MachineCommand::setRemExec( const char* sh )
{
    remexec_.set( sh );
    return *this;
}


OS::MachineCommand& OS::MachineCommand::setHostName( const char* hnm )
{
    const BufferString hostnm( hnm );
    const BufferString localhostnm( GetLocalHostName() );
    if ( !hostnm.isEmpty() && !hostnm.startsWith(localhostnm) &&
	  hostnm != "localhost" )
	hname_.set( hnm );

    return *this;
}


OS::MachineCommand& OS::MachineCommand::setHostIsWindows( bool yn )
{
    hostiswin_ = yn;
    return *this;
}


OS::MachineCommand& OS::MachineCommand::addFileRedirect( const char* fnm,
						int stdcode, bool append )
{
    BufferString redirect;
    if ( stdcode==1 || stdcode==2 )
	redirect.add( stdcode );

    redirect.add( ">" );
    if ( append )
	redirect.add( ">" );

    if ( stdcode == 3 )
	return addArg( redirect ).addArg( fnm ).addArg( "2>&1" );
    else
	return addArg( redirect ).addArg( fnm );
}


OS::MachineCommand& OS::MachineCommand::addArgs( const BufferStringSet& toadd )
{
    args_.append( toadd );
    return *this;
}


OS::MachineCommand& OS::MachineCommand::addKeyedArg( const char* ky,
			 const char* str, KeyStyle ks )
{
    if ( isOldStyle(ks) )
	addArg( BufferString( "-", ky ) );
    else
	addArg( CommandLineParser::createKey(ky) );
    addArg( str );
    return *this;
}


OS::MachineCommand&
OS::MachineCommand::addPipedCommand( const MachineCommand& mc )
{
    if ( pipedmc_ )
	*pipedmc_ = mc;
    else
	pipedmc_ = new MachineCommand( mc );

    return *this;
}


namespace OS {

BufferString& GetIsolateScript()
{
    mDefineStaticLocalObject( PtrMan<BufferString>, ret, = new BufferString );
    return *ret.ptr();
}

} // namespace OS


void OS::MachineCommand::setIsolationScript( const char* fnm )
{
    if ( !fnm || File::exists(fnm) )
	GetIsolateScript().set( fnm );
}


const char* OS::MachineCommand::getIsolationScriptFnm()
{
    return GetIsolateScript().buf();
}


void OS::MachineCommand::setIsolated( const char* prognm )
{
    const BufferString& isolatescript = GetIsolateScript();
    BufferString scriptcmd( isolatescript.isEmpty() ? GetODExternalScript()
			  : isolatescript.str() );
    prognm_.set( scriptcmd );
    if ( prognm && *prognm )
	args_.insertAt( new BufferString(prognm), 0 );
    const BufferString pathed( GetEnvVarDirListWoOD("PATH") );
    if ( !pathed.isEmpty() )
	SetEnvVar( "OD_INTERNAL_CLEANPATH", pathed.buf() );

#ifdef __unix__
    if ( GetEnvVar("OD_SYSTEM_LIBRARY_PATH") )
	return;

    const BufferString ldlibpathed( GetEnvVarDirListWoOD("LD_LIBRARY_PATH") );
    if ( !ldlibpathed.isEmpty() )
	SetEnvVar( "OD_SYSTEM_LIBRARY_PATH", ldlibpathed.buf() );
#endif
}


#ifdef __win__

static BufferString getUsableWinCmd( const char* fnm, BufferStringSet& args )
{
    BufferString ret( fnm );

    BufferString execnm( fnm );
    char* ptr = execnm.find( ':' );
    if ( !ptr )
	return ret;

    char* argsptr = nullptr;

    // if only one char before the ':', it must be a drive letter.
    if ( ptr == execnm.buf() + 1 )
    {
	ptr = firstOcc( ptr , ' ' );
	if ( ptr ) { *ptr = '\0'; argsptr = ptr+1; }
    }
    else if ( ptr == execnm.buf()+2)
    {
	char sep = *execnm.buf();
	if ( sep == '\"' || sep == '\'' )
	{
	    execnm=fnm+1;
	    ptr = execnm.find( sep );
	    if ( ptr ) { *ptr = '\0'; argsptr = ptr+1; }
	}
    }
    else
	return ret;

    if ( execnm.contains(".exe") || execnm.contains(".EXE")
       || execnm.contains(".bat") || execnm.contains(".BAT")
       || execnm.contains(".com") || execnm.contains(".COM") )
	return ret;

    const char* interp = nullptr;

    if ( execnm.contains(".csh") || execnm.contains(".CSH") )
	interp = "tcsh.exe";
    else if ( execnm.contains(".sh") || execnm.contains(".SH") ||
	      execnm.contains(".bash") || execnm.contains(".BASH") )
	interp = "sh.exe";
    else if ( execnm.contains(".awk") || execnm.contains(".AWK") )
	interp = "awk.exe";
    else if ( execnm.contains(".sed") || execnm.contains(".SED") )
	interp = "sed.exe";
    else if ( File::exists(execnm) )
    {
	// We have a full path to a file with no known extension,
	// but it exists. Let's peek inside.

	od_istream strm( execnm );
	if ( !strm.isOK() )
	    return ret;

	BufferString buf( 41, false );
	strm.getC( buf.getCStr(), buf.bufSize(), buf.bufSize()-1 );
	BufferString line( buf.buf() );

	if ( !line.contains("#!") && !line.contains("# !") )
	    return ret;

	if ( line.contains("csh") )
	    interp = "tcsh.exe";
	else if ( line.contains("awk") )
	    interp = "awk.exe";
	else if ( line.contains("sh") )
	    interp = "sh.exe";
    }

    if ( !interp )
	return ret;

    if ( argsptr && *argsptr )
	args.unCat( argsptr, " " );

    args.insertAt( new BufferString(
		FilePath(execnm).fullPath(FilePath::Unix)), 0 );

    return ret;
}

#endif


static BufferString getUsableUnixCmd( const char* fnm, BufferStringSet& )
{
    BufferString ret( fnm );
    if ( FilePath(fnm).isAbsolute() &&
	 File::exists(fnm) && File::isFile(fnm) )
	return ret;

    ret = GetShellScript( fnm );
    if ( File::exists(ret) && File::isFile(fnm) )
	return ret;

    ret = GetPythonScript( fnm );
    if ( File::exists(ret) && File::isFile(fnm) )
	return ret;

    return BufferString( fnm );
}


static BufferString getUsableCmd( const char* fnm, BufferStringSet& args )
{
#ifdef __win__
    return getUsableWinCmd( fnm, args );
#else
    return getUsableUnixCmd( fnm, args );
#endif
}


OS::MachineCommand OS::MachineCommand::getExecCommand(
					const CommandExecPars* pars ) const
{
    MachineCommand ret;

    if ( pars && pars->isconsoleuiprog_ )
    {
#ifdef __unix__
	const BufferString str(
	    FilePath( GetSoftwareDir(true), "bin",
		    "od_exec_consoleui.scr" ).fullPath() );
	ret.setProgram( FilePath( GetSoftwareDir(true), "bin",
				    "od_exec_consoleui.scr" ).fullPath() );
#endif
    }

    BufferStringSet mcargs;
    const BufferString prognm = getUsableCmd( prognm_, mcargs );
    if ( remexec_.isEmpty() || hname_.isEmpty() )
    {
	if ( ret.isBad() )
	    ret.setProgram( prognm );
	else
	    ret.addArg( prognm );
    }
    else
    {
	if ( ret.isBad() )
	    ret.setProgram( remexec_ );
	else
	    ret.addArg( remexec_ );
	if ( remexec_ == odRemExecCmd() )
	{
	    ret.addKeyedArg( sKeyRemoteHost(), hname_ )
	       .addFlag( sKeyRemoteCmd() );
	}
	else
	{
	    ret.addArg( hname_.str() );
	    if ( prognm.startsWith("od_") )
		ret.addArg( FilePath(GetShellScript("exec_prog")).fullPath() );
	}
	ret.addArg( prognm );
    }

    ret.addArgs( mcargs );
    ret.addArgs( args_ );

    if ( pars && pars->launchtype_ != Wait4Finish &&
	 !mIsZero(pars->prioritylevel_,1e-2f) )
	ret.addKeyedArg( CommandExecPars::sKeyPriority(),pars->prioritylevel_);

    ret.addShellIfNeeded();

    return ret;
}


void OS::MachineCommand::addShellIfNeeded()
{
    if ( !args_.isEmpty() &&
	 ((__iswin__ && prognm_ == StringView("cmd") &&
			*args_.first() == StringView("/c")) ||
	 ( *args_.first() == StringView("-c") &&
	   ((__islinux__ && prognm_ == StringView("/bin/sh")) ||
	    (__ismac__ && prognm_ == StringView("/bin/bash"))) )) )
	return;

    bool needsshell = prognm_.startsWith( "echo", OD::CaseInsensitive );
    if ( !needsshell )
    {
	for ( const auto arg : args_ )
	{
	    if ( arg->find(">") || arg->find("<") || arg->find("|") )
	    {
		needsshell = true;
		break;
	    }
	}
    }

    if ( !needsshell )
	return;

    args_.insertAt( new BufferString(prognm_), 0 );
    if ( __iswin__ )
    {
	args_.insertAt( new BufferString("/c"), 0 );
	prognm_.set( "cmd" );
    }
    else
    {
	if ( __islinux__ )
	    prognm_.set( "/bin/sh" );
	else
	    prognm_.set( "/bin/bash" );

	for ( auto arg : args_ )
	{
	    if ( arg->find(' ') && arg->firstChar() != '\'' )
		arg->quote();
	}

	const BufferString cmdstr( args_.cat(" ") );
	args_.setEmpty();
	args_.add( "-c" ).add( cmdstr.buf() );
	/* The whole command as one arguments.
	   Quotes will be added automatically */
    }
}


BufferString OS::MachineCommand::toString( const OS::CommandExecPars* pars
									) const
{
    const MachineCommand mc = getExecCommand( pars );
    BufferString ret( mc.program() );
    if ( !mc.args().isEmpty() )
	ret.addSpace().add( mc.args().cat( " " ) );

    return ret;
}


bool OS::MachineCommand::execute( LaunchType lt, const char* workdir )
{
    CommandLauncher cl( *this );
    const bool res = cl.execute( lt, workdir );
    errmsg_ = cl.errorMsg();
    return res;
}


bool OS::MachineCommand::execute( BufferString& out, BufferString* err,
				  const char* workdir )
{
    CommandLauncher cl( *this );
    const bool res = cl.execute( out, err, workdir );
    errmsg_ = cl.errorMsg();
    return res;
}


bool OS::MachineCommand::execute( const CommandExecPars& execpars )
{
    CommandLauncher cl( *this );
    const bool res = cl.execute( execpars );
    errmsg_ = cl.errorMsg();
    return res;
}


BufferString OS::MachineCommand::runAndCollectOutput( BufferString* errmsg )
{
    BufferString ret;
    if ( !getNonConst(*this).execute(ret,errmsg) )
	ret.setEmpty();

    return ret;
}


// OS::CommandLauncher

OS::CommandLauncher::CommandLauncher( const OS::MachineCommand& mc )
    : odprogressviewer_(FilePath(GetExecPlfDir(),
			ODInst::sKeyODProgressViewerExecNm()).fullPath())
    , machcmd_(mc)
    , started(this)
    , finished(this)
    , errorOccurred(this)
    , stateChanged(this)
{
}


OS::CommandLauncher::~CommandLauncher()
{
    detachAllNotifiers();
#ifndef OD_NO_QT
    if ( process_ && process_->state()!=OD::Process::State::NotRunning )
	manageODProcess( process_.ptr() );
#endif
}


ConstRefMan<OD::Process> OS::CommandLauncher::process() const
{
    ConstRefMan<OD::Process> ret = process_.ptr();
    return ret;
}


RefMan<OD::Process> OS::CommandLauncher::process()
{
    RefMan<OD::Process> ret = process_.ptr();
    return ret;
}


PID_Type OS::CommandLauncher::processID() const
{
    return mIsUdf(pid_) ? (process_ ? process_->processId() : 0) : pid_;
}


int OS::CommandLauncher::exitCode() const
{
    return process_ ? process_->exitCode() : exitcode_;
}


OD::Process::ExitStatus OS::CommandLauncher::exitStatus() const
{
    return process_ ? process_->exitStatus() : exitstatus_;
}


void OS::CommandLauncher::reset()
{
#ifndef OD_NO_QT
    if ( process_ && process_->state()!=OD::Process::State::NotRunning )
	manageODProcess( process_.ptr() );

    mDetachCB( process_->started, CommandLauncher::startedCB );
    mDetachCB( process_->finished, CommandLauncher::finishedCB );
    mDetachCB( process_->stateChanged, CommandLauncher::stateChangedCB );
    mDetachCB( process_->errorOccurred, CommandLauncher::errorOccurredCB );
    process_ = nullptr;
    pipeprocess_ = nullptr;
#endif
    errmsg_.setEmpty();
    monitorfnm_.setEmpty();
    progvwrcmd_.setEmpty();
    pid_ = mUdf(PID_Type);
    exitcode_ = mUdf(int);
    exitstatus_ = OD::Process::ExitStatus::NormalExit;
}


void OS::CommandLauncher::set( const OS::MachineCommand& cmd )
{
    machcmd_ = cmd;
    reset();
}


bool OS::CommandLauncher::execute( OS::LaunchType lt, const char* workdir )
{
    CommandExecPars execpars( lt );
    if ( workdir && *workdir )
	execpars.workingdir( workdir );

    return execute( execpars );
}


bool OS::CommandLauncher::execute( BufferString& out, BufferString* err,
				   const char* workdir )
{
    CommandExecPars execpars( Wait4Finish );
    execpars.txtbufstdout( true ).txtbufstderr( err );
    if ( workdir && *workdir )
	execpars.workingdir( workdir );

    const bool res = execute( execpars );
    if ( hasStdOutput() )
	getAll( out, true );

    if ( err && hasStdError() )
	getAll( *err, false );

    return res;
}


bool OS::CommandLauncher::execute( const OS::CommandExecPars& pars )
{
    reset();
    if ( machcmd_.isBad() )
	{ errmsg_ = toUiString("Command is invalid"); return false; }

    if ( StringView(machcmd_.program()).contains("python") )
    {
	const FilePath pythfp( machcmd_.program() );
	if ( pythfp.nrLevels() < 2 ||
	    (pythfp.exists() && pythfp.fileName().contains("python") ) )
	    pErrMsg("Python commands should be run using OD::PythA().execute");
    }

    const MachineCommand mcmd = machcmd_.getExecCommand( &pars );
    if ( mcmd.isBad() )
	{ errmsg_ = toUiString("Empty command to execute"); return false; }

    if ( pars.needmonitor_ && !pars.isconsoleuiprog_ )
    {
	monitorfnm_ = pars.monitorfnm_;
	if ( monitorfnm_.isEmpty() )
	    monitorfnm_ = FilePath::getTempFullPath( "mon", "txt" );

	if ( File::exists(monitorfnm_) && !File::remove(monitorfnm_) )
	    return false;
    }

    const bool ret = doExecute( mcmd, pars );
    uiString cannotlaunchstr = toUiString( "Cannot launch '%1'" );
    if ( pars.isconsoleuiprog_ )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = cannotlaunchstr.arg( mcmd.toString(&pars) );
	return ret;
    }

    if ( !ret )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = cannotlaunchstr.arg( mcmd.toString(&pars) );
	return false;
    }

    if ( pars.launchtype_ != Wait4Finish )
	startMonitor();

    return ret;
}


bool OS::CommandLauncher::startServer( bool ispyth, const char* stdoutfnm,
				       const char* stderrfnm,
				       bool consolestdout, bool consolestderr,
				       double waittm )
{
    CommandExecPars execpars( RunInBG );
    execpars.txtbufstdout( true ).txtbufstderr( true );
	// this has to be done otherwise we cannot pick up any error messages
    if ( stdoutfnm )
	execpars.stdoutfnm( stdoutfnm );

    if ( stderrfnm )
	execpars.stderrfnm( stderrfnm );

    uiRetVal ret;
    pid_ = mUdf(PID_Type);
    if ( ispyth )
    {
	if ( !OD::PythA().execute(machcmd_,execpars,ret,&pid_) )
	    pid_ = mUdf(PID_Type);
    }
    else
    {
	if ( !execute(execpars) )
	    pid_ = mUdf(PID_Type);
    }

    if ( mIsUdf(pid_) || pid_ < 1 )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = uiStrings::phrCannotStart(
				::toUiString(machcmd_.toString(&execpars) ) );
	return false;
    }

    bool wasalive = false;
    const double waitstepsec = 0.1;
    while ( waittm > 0 )
    {
	if ( isProcessAlive(pid_) )
	{
	    wasalive = true;
	    break;
	}
	waittm -= waitstepsec;
	sleepSeconds( waitstepsec );
    }

    if ( !wasalive && !isProcessAlive(pid_) )
    {
	if ( ispyth )
	    errmsg_ = ret.messages().cat();

	if ( errmsg_.isEmpty() )
	    errmsg_ = tr("Server process (%1) exited early")
			    .arg( machcmd_.toString(&execpars) );
	return false;
    }

    return true;
}


void OS::CommandLauncher::startMonitor()
{
    if ( monitorfnm_.isEmpty() )
	return;

    MachineCommand progvwrcmd( odprogressviewer_ );
    progvwrcmd.addKeyedArg( "inpfile", monitorfnm_ );
    progvwrcmd.addKeyedArg( "pid", processID() );

    OS::CommandLauncher progvwrcl( progvwrcmd );
    if ( !progvwrcl.execute(RunInBG) )
    {
	// sad ... but the process has been launched anyway
	ErrMsg( BufferString( "[Monitoring does not start] ",
				 progvwrcl.errorMsg() ) );
    }
}


namespace OD
{

static void checkKernelVersion()
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    static bool istested = false;
    if ( !istested )
    {
	istested = true;
	const QString qkernelver = QSysInfo::kernelVersion();
	const BufferString kernelver( qkernelver );
	if ( kernelver.isEmpty() )
	    return;

	const SeparString kernelverss( kernelver.buf(), '-' );
	if ( kernelver.size() < 2 )
	    return;

	const QVersionNumber qnum =
			QVersionNumber::fromString( kernelverss[0].str() );
	if ( qnum.majorVersion() > 4 )
	    return;

	const QVersionNumber qpatch =
			QVersionNumber::fromString( kernelverss[1].str() );
	if ( qpatch.majorVersion() > 400 )
	    return;

	od_ostream::logStream() << "[WARNING] "
			    "OpendTect may not work reliably on this kernel, "
			    "please consider upgrading" << od_endl;
    }
#endif
}

} // namespace OD


bool OS::CommandLauncher::doExecute( const MachineCommand& mc,
				     const CommandExecPars& pars )
{
    if ( mc.isBad() )
	{ errmsg_ = tr("Command is empty"); return false; }

#ifdef __win__
    if ( StringView(mc.program()) == GetIsolateScript() &&
	 pars.workingdir_.isEmpty() )
    {
	CommandExecPars& parsedit = const_cast<CommandExecPars&>( pars );
	parsedit.workingdir( GetPersonalDir() );
    }

    if ( pars.runasadmin_ )
    {
	BufferString argsstr;
	for ( int idx=0; idx<mc.args().size(); idx++ )
	{
	    BufferString arg( mc.args().get(idx) );
	    if ( arg.find(" ") )
		arg.quote('\"');
	    if ( !argsstr.isEmpty() )
		argsstr.addSpace();
	    argsstr.add( arg );
	}
	const HINSTANCE res = ShellExecuteA( NULL, "runas", mc.program(),
	    argsstr, pars.workingdir_, SW_SHOW );
	return static_cast<int>(reinterpret_cast<uintptr_t>(res)) >
							    HINSTANCE_ERROR;
    }
#endif

    if ( __islinux__ )
	OD::checkKernelVersion();

    if ( process_ )
    {
	errmsg_ = tr("Process is already running ('%1')")
				.arg( mc.toString(&pars) );
	return false;
    }

    DBG::message( DBG_DBG,
	BufferString("About to execute: ",mc.toString(&pars)) );

    const bool wt4finish = pars.launchtype_ == Wait4Finish;
    const bool createstreams = pars.createstreams_;
#ifndef OD_NO_QT
    process_ = new OD::Process;
    process_->setProgram(  mc.program() ).setArguments( mc.args() );
    mAttachCB( process_->started, CommandLauncher::startedCB );
    mAttachCB( process_->finished, CommandLauncher::finishedCB );
    mAttachCB( process_->stateChanged, CommandLauncher::stateChangedCB );
    mAttachCB( process_->errorOccurred, CommandLauncher::errorOccurredCB );
    const MachineCommand* pipedmc = machcmd_.pipedCommand();
    if ( wt4finish && pipedmc )
    {
	pipeprocess_ = new OD::Process;
	pipeprocess_->setProgram( pipedmc->program() )
		     .setArguments( pipedmc->args() );
	process_->setStandardOutputProcess( *pipeprocess_.ptr() );
    }

    if ( createstreams )
    {
	OD::Process* proc = getReadProcess();
	const BufferString& stdoutfnm = monitorfnm_.isEmpty()
				      ? pars.stdoutfnm_
				      : monitorfnm_;
	if ( !pars.consolestdout_ && !pars.txtbufstdout_ &&
	     !stdoutfnm.isEmpty() )
	{
	    proc->setStandardOutputFile( stdoutfnm.str() );
	}
	else if ( pars.consolestdout_ || pars.txtbufstdout_ )
	{
	    proc->setStandardOutputStream( pars.consolestdout_,
				pars.txtbufstdout_, stdoutfnm.buf() );
	}

	const BufferString& stderrfnm = pars.stderrfnm_;
	if ( !pars.consolestderr_ && !pars.txtbufstderr_ &&
	     !stderrfnm.isEmpty() )
	{
	    proc->setStandardErrorFile( stderrfnm.str() );
	}
	else if ( pars.consolestderr_ || pars.txtbufstderr_ )
	{
	    proc->setStandardErrorStream( pars.consolestderr_,
				pars.txtbufstderr_, stderrfnm.buf() );
	}
    }

    const BufferString& workingdir = pars.workingdir_;
    if ( !workingdir.isEmpty() )
	process_->setWorkingDirectory( workingdir.str() );

    const BufferStringSet& environment = pars.environment_;
    if ( !environment.isEmpty() )
	process_->setEnvironment( environment );

    const OD::Process::InputChannelMode inputchmode = pars.inputchmode_;
    if ( inputchmode != OD::Process::InputChannelMode::ManagedInputChannel )
	process_->setInputChannelMode( inputchmode );

    const OD::Process::ChannelMode channelmode = pars.channelmode_;
    if ( channelmode != OD::Process::ChannelMode::SeparateChannels )
	process_->setProcessChannelMode( channelmode );

    if ( wt4finish || createstreams )
    {
	//TODO: use inconsole on Windows ?
	if ( pipeprocess_ )
	{
	    pipeprocess_->start();
	    if ( !pipeprocess_->waitForStarted(10000) )
		return !catchError( true );
	}

	process_->start();
	if ( !process_->waitForStarted(10000) ) //Timeout of 10 secs
	    return !catchError();
    }
    else
    {
	const bool res = startDetached( pars.isconsoleuiprog_ );
	return res;
    }

    if ( wt4finish )
    {
	startMonitor();
	if ( createstreams )
	    process_->closeWriteChannel();

	if ( process_->isRunning() )
	{
	    process_->waitForFinished(-1);
	    if ( pipeprocess_ )
		pipeprocess_->waitForFinished(-1);
	}

	OD::Process* proc = getReadProcess();
	bool res = proc->exitStatus() == OD::Process::ExitStatus::NormalExit;
	if ( res )
	{
	    exitcode_ = proc->exitCode();
	    res = exitcode_ == 0;
	}

	return res;
    }
#endif

    return true;
}


#ifndef OD_NO_QT
#if QT_VERSION < QT_VERSION_CHECK(5,10,0)

namespace OS {

static bool startDetachedLegacy( const char* prog,
		const BufferStringSet& progargs, const char* workdir,
		bool inconsole, PID_Type& pid )
{
#ifdef __win__
    BufferString comm( prog );
    if ( !progargs.isEmpty() )
    {
	BufferStringSet args( progargs );
	for ( auto* arg : args )
	{
	    if ( arg->find(" ") && !arg->startsWith("\"") &&
		!arg->startsWith("'") )
		arg->quote('\"');
	    comm.addSpace().add( arg->str() );
	}
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(STARTUPINFO) );
    ZeroMemory( &pi, sizeof(pi) );
    si.cb = sizeof( STARTUPINFO );

    LPCSTR curdir = workdir && *workdir ? workdir : nullptr;
    const int wincrflg = inconsole ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW;
    const bool res = CreateProcess( NULL,
	comm.getCStr(),
	NULL,	// Process handle not inheritable.
	NULL,	// Thread handle not inheritable.
	FALSE,	// Set handle inheritance.
	wincrflg,   // Creation flags.
	NULL,	// Use parent's environment block.
	curdir,   // Use parent's starting directory.
	&si, &pi );

    if ( res )
    {
	pid = (PID_Type)pi.dwProcessId;
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
    }

    return res;
#else
    const QString qprog( prog );
    QStringList qargs;
    progargs.fill( qargs );
    const QString qworkdir( workdir );
    qint64 qpid = 0;

    if ( !QProcess::startDetached(qprog,qargs,qworkdir,&qpid) )
	return false;

    pid = (PID_Type)qpid;

    return true;

#endif
}

} // namespace OS

#endif
#endif


bool OS::CommandLauncher::startDetached( bool inconsole )
{
#ifndef OD_NO_QT

#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)

#ifdef __win__
    if ( inconsole )
    {
	process_->process()->setCreateProcessArgumentsModifier(
	    [] (QProcess::CreateProcessArguments *args )
	{
	    args->flags |= CREATE_NEW_CONSOLE;
	    args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
	    args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
	    args->startupInfo->hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	    args->startupInfo->wShowWindow = SW_SHOW;
	});
    }
#endif

    return process_->startDetached( &pid_ );

#else

    const BufferString program = process_->program();
    const BufferStringSet args = process_->arguments();
    const BufferString workdir = process_->workingDirectory();
    return startDetachedLegacy( program.buf(), args, workdir.buf(),
				inconsole, pid_ );

#endif

#else

    return false;

#endif
}


const OD::Process* OS::CommandLauncher::getReadProcess() const
{
    return pipeprocess_ ? pipeprocess_.ptr() : process_.ptr();
}


OD::Process* OS::CommandLauncher::getReadProcess()
{
    return pipeprocess_ ? pipeprocess_.ptr() : process_.ptr();
}


int OS::CommandLauncher::catchError( bool pipecmd )
{
    const OD::Process* process = getReadProcess();
    if ( !process )
	return 0;

    if ( !errmsg_.isEmpty() )
	return 1;

#ifndef OD_NO_QT
    const OD::Process::Error error = process->error();
    switch ( error )
    {
	case OD::Process::Error::FailedToStart :
	    errmsg_ = tr("Cannot start process %1.");
	    break;
	case OD::Process::Error::Crashed :
	    errmsg_ = tr("%1 crashed.");
	    break;
	case OD::Process::Error::Timedout :
	    errmsg_ = tr("%1 timeout");
	    break;
	case OD::Process::Error::ReadError :
	    errmsg_ = tr("Read error from process %1");
	    break;
	case OD::Process::Error::WriteError :
	    errmsg_ = tr("Write error from process %1");
	    break;
	default :
	    break;
    }

    if ( !errmsg_.isEmpty() )
    {
	const OS::MachineCommand& mc = pipecmd ? *machcmd_.pipedCommand()
					       : machcmd_;
	BufferString argstr( mc.program() );
	if ( mc.hasHostName() )
	    argstr.add( " @ " ).add( mc.hostName() );
	errmsg_.arg( argstr );
	return 1;
    }
    return process->exitCode();
#else

    return 0;
#endif
}


void OS::CommandLauncher::startedCB( CallBacker* cb )
{
    started.trigger();
}


using procresobj = std::pair<int,OD::Process::ExitStatus>;

void OS::CommandLauncher::finishedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( procresobj, exitcaps, cb );
    finished.trigger( exitcaps );
}


void OS::CommandLauncher::errorOccurredCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( OD::Process::Error, error, cb );
    errorOccurred.trigger( error );
}


void OS::CommandLauncher::stateChangedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( OD::Process::State, state, cb );
    stateChanged.trigger( state );
}


bool OS::CommandLauncher::hasStdInput() const
{
    return process_ && process_->hasStdInput();
}


bool OS::CommandLauncher::hasStdOutput() const
{
    const OD::Process* process = getReadProcess();
    return process && process->hasStdOutput();
}


bool OS::CommandLauncher::hasStdError() const
{
    const OD::Process* process = getReadProcess();
    return process && process->hasStdError();
}


od_int64 OS::CommandLauncher::write( const char* data, od_int64 maxsz )
{
    return process_ ? process_->write( data, maxsz ) : mUdf(od_int64);
}


od_int64 OS::CommandLauncher::write( const char* data )
{
    return process_ ? process_->write( data ) : mUdf(od_int64);
}


bool OS::CommandLauncher::getLine( BufferString& ret, bool stdoutstrm,
				   bool* newline_found )
{
    OD::Process* process = getReadProcess();
    return process ? process->getLine( ret, stdoutstrm, newline_found )
		   : false;
}


bool OS::CommandLauncher::getAll( BufferStringSet& ret, bool stdoutstrm )
{
    OD::Process* process = getReadProcess();
    return process ? process->getAll( ret, stdoutstrm ) : false;
}


bool OS::CommandLauncher::getAll( BufferString& ret, bool stdoutstrm )
{
    OD::Process* process = getReadProcess();
    return process ? process->getAll( ret, stdoutstrm ) : false;
}


bool OS::CommandLauncher::openTerminal( const char* cmdstr,
			    const BufferStringSet* args, BufferString* errmsg,
			    uiString* launchermsg, const char* workdirstr )
{
    if ( !cmdstr || !*cmdstr )
    {
	if ( errmsg )
	    errmsg->set( "[Internal] No terminal name provided" );
	return false;
    }

    MachineCommand mc( cmdstr );
    if ( args )
	mc.addArgs( *args );

    BufferString workdir( workdirstr );
    if ( workdir.isEmpty() )
	workdir = GetPersonalDir();

    CommandExecPars pars( RunInBG );
    pars.createstreams( !__iswin__ )
	.workingdir( workdir )
	.isconsoleuiprog( __iswin__ );

    CommandLauncher cl( mc );
    const bool res = cl.execute( pars );
    if ( launchermsg )
	launchermsg->set( cl.errorMsg() );

    if ( errmsg && cl.hasStdError() )
	cl.getAll( *errmsg, false );

    return res;
}


void OD::DisplayErrorMessage( const char* msg )
{
    OS::MachineCommand machcomm( GetODApplicationName("od_DispMsg") );
    machcomm.addKeyedArg( "err", msg );
    machcomm.execute( OS::RunInBG );
}
