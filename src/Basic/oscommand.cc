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
#include "pythonaccess.h"
#include "separstr.h"
#include "settingsaccess.h"
#include "uistrings.h"

#include "hiddenparam.h"

#ifndef OD_NO_QT
# include "qstreambuf.h"
# include <QProcess>
#endif

#include <iostream>

#ifdef __win__
# include "shellapi.h"
# include "winutils.h"
# include <windows.h>
# include <stdlib.h>
#endif

BufferString OS::MachineCommand::defremexec_( "ssh" );
static const char* sODProgressViewerProgName = "od_ProgressViewer";
static const char* sKeyExecPars = "ExecPars";
static const char* sKeyMonitor = "Monitor";
static const char* sKeyProgType = "ProgramType";
static const char* sKeyPriorityLevel = "PriorityLevel";
static const char* sKeyWorkDir = "WorkingDirectory";


//
class QProcessManager
{
public:
		~QProcessManager()
		{
		    deleteProcesses();
		}
    void	takeOver( QProcess* p )
		{
		    processes_ += p;
		}
    void	deleteProcesses()
		{
#ifndef OD_NO_QT
		    for ( auto process : processes_ )
			process->close();
		    deepErase( processes_ );
#endif
		}


    static Threads::Lock	lock_;

private:
    ObjectSet<QProcess>		processes_;
};

static PtrMan<QProcessManager> processmanager;
Threads::Lock QProcessManager::lock_( true );

void DeleteProcesses()
{
    Threads::Locker locker( QProcessManager::lock_ );
    if ( processmanager )
	processmanager->deleteProcesses();
}

void OS::CommandLauncher::manageQProcess( QProcess* p )
{
    Threads::Locker locker( QProcessManager::lock_ );

    if ( !processmanager )
    {
	processmanager = new QProcessManager;
	NotifyExitProgram( DeleteProcesses );
    }

    processmanager->takeOver( p );
}



OS::CommandExecPars::CommandExecPars( bool isbatchprog )
    : CommandExecPars( isbatchprog ? RunInBG : Wait4Finish )
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


const StepInterval<int> OS::CommandExecPars::cMachineUserPriorityRange(
					     bool iswin )
{ return iswin ? StepInterval<int>( 6, 8, 1 ) :StepInterval<int>( 0, 19, 1 ); }


int OS::CommandExecPars::getMachinePriority( float priority, bool iswin )
{
    const StepInterval<int> machpriorg( cMachineUserPriorityRange(iswin) );
    const float scale = iswin ? 1.f : -1.f;

    int machprio = mCast(int, mNINT32(scale * priority * machpriorg.width()) );

    return machprio += iswin ? machpriorg.stop : machpriorg.start;
}



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


OS::MachineCommand::MachineCommand( const MachineCommand& oth, bool isolated )
{
    if ( &oth == this )
	return;

    if ( isolated )
    {
	setIsolated( oth.program() );
	addArgs( oth.args() );
    }
    else
	*this = MachineCommand( oth );
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

    const char* interp = 0;

    if ( execnm.contains(".csh") || execnm.contains(".CSH") )
	interp = "tcsh.exe";
    else if ( execnm.contains(".sh") || execnm.contains(".SH") ||
	      execnm.contains(".bash") || execnm.contains(".BASH") )
	interp = "sh.exe";
    else if ( execnm.contains(".awk") || execnm.contains(".AWK") )
	interp = "awk.exe";
    else if ( execnm.contains(".sed") || execnm.contains(".SED") )
	interp = "sed.exe";
    else if ( File::exists( execnm ) )
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

    FilePath interpfp;
    const char* cygdir = getCygDir();
    if ( cygdir && *cygdir )
    {
	interpfp.set( cygdir );
	interpfp.add( "bin" ).add( interp );
    }

    if ( !File::exists( interpfp.fullPath() ) )
    {
	interpfp.set( GetSoftwareDir(true) );
	interpfp.add("bin").add("win").add("sys").add(interp);
    }

    ret.set( interpfp.fullPath() );
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
#ifdef __win__
    if ( prognm_ == StringView("cmd") && args_.size() > 0 &&
	 *args_.first() == StringView("/c") )
#else
    if ( prognm_ == StringView("/bin/sh") && args_.size() > 0 &&
	 *args_.first() == StringView("-c") )
#endif
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
#ifdef __win__
    args_.insertAt( new BufferString("/c"), 0 );
    prognm_.set( "cmd" );
#else
    prognm_.set( "/bin/sh" );
    for ( auto arg : args_ )
    {
	if ( arg->find(' ') && arg->firstChar() != '\'' )
	    arg->quote();
    }
    const BufferString cmdstr( args_.cat(" ") );
    args_.setEmpty();
    args_.add( "-c" ).add( cmdstr.buf() );
    // The whole command as one arguments. Quotes will be added automatically
#endif
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
    return CommandLauncher(*this).execute( lt, workdir );
}


bool OS::MachineCommand::execute( BufferString& out, BufferString* err,
				  const char* workdir )
{
    return CommandLauncher(*this).execute( out, err, workdir );
}


bool OS::MachineCommand::execute( const CommandExecPars& execpars )
{
    return CommandLauncher(*this).execute( execpars );
}



BufferString OS::MachineCommand::runAndCollectOutput( BufferString* errmsg )
{
    BufferString ret;
    if ( !CommandLauncher(*this).execute(ret,errmsg) )
	ret.setEmpty();
    return ret;
}


// OS::CommandLauncher

static HiddenParam<OS::CommandLauncher,int> clexitcodehpmgr_(mUdf(int));

OS::CommandLauncher::CommandLauncher( const OS::MachineCommand& mc )
    : odprogressviewer_(FilePath(GetExecPlfDir(),sODProgressViewerProgName)
			    .fullPath())
    , process_(nullptr)
    , stderror_(nullptr)
    , stdoutput_(nullptr)
    , stdinput_(nullptr)
    , stderrorbuf_(nullptr)
    , stdoutputbuf_(nullptr)
    , stdinputbuf_(nullptr)
    , pid_(0)
    , machcmd_(mc)
{
    clexitcodehpmgr_.setParam( this, mUdf(int) );
    reset();
}


OS::CommandLauncher::~CommandLauncher()
{
#ifndef OD_NO_QT
    reset();
#endif
    clexitcodehpmgr_.removeParam( this );
}


PID_Type OS::CommandLauncher::processID() const
{
#ifndef OD_NO_QT

    if ( !process_ )
	return pid_;
#if QT_VERSION >= QT_VERSION_CHECK(5,3,0)
    const qint64 pid = process_->processId();
    return mCast( PID_Type, pid );
#else
# ifdef __win__
    const PROCESS_INFORMATION* pi = (PROCESS_INFORMATION*) process_->pid();
    return pi->dwProcessId;
# else
    return process_->pid();
# endif
#endif

#else
    return 0;
#endif
}


int OS::CommandLauncher::exitCode() const
{
    return clexitcodehpmgr_.getParam( this );
}


void OS::CommandLauncher::reset()
{
    deleteAndNullPtr( stderror_ );
    deleteAndNullPtr( stdoutput_ );
    deleteAndNullPtr( stdinput_ );

    stderrorbuf_ = nullptr;
    stdoutputbuf_ = nullptr;
    stdinputbuf_ = nullptr;

#ifndef OD_NO_QT
    if ( process_ && process_->state()!=QProcess::NotRunning )
    {
	manageQProcess( process_ );
	process_ = nullptr;
    }
    deleteAndNullPtr( process_ );
#endif
    errmsg_.setEmpty();
    monitorfnm_.setEmpty();
    progvwrcmd_.setEmpty();
    clexitcodehpmgr_.setParam( this, mUdf(int) );
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
    execpars.createstreams( true );
    if ( workdir && *workdir )
	execpars.workingdir( workdir );

    const bool res = execute( execpars );
    if ( getStdOutput() )
	getStdOutput()->getAll( out );
    if ( err && getStdError() )
	getStdError()->getAll( *err );

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
				       const char* stderrfnm, double waittm )
{
    CommandExecPars execpars( RunInBG );
    execpars.createstreams( true );
	// this has to be done otherwise we cannot pick up any error messages
    if ( stdoutfnm )
	execpars.stdoutfnm( stdoutfnm );
    if ( stderrfnm )
	execpars.stderrfnm( stderrfnm );

    uiRetVal ret;
    pid_ = -1;
    if ( ispyth )
    {
	if ( !OD::PythA().execute(machcmd_,execpars,ret,&pid_) )
	    pid_ = -1;
    }
    else
    {
	if ( !execute(execpars) )
	    pid_ = -1;
    }

    if ( pid_ < 1 )
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
    process_ = wt4finish || createstreams ? new QProcess : nullptr;

    if ( createstreams )
    {
	if ( !monitorfnm_.isEmpty() )
	    process_->setStandardOutputFile( monitorfnm_.buf() );
	else
	{
	    stdinputbuf_ = new qstreambuf( *process_, false, false );
	    stdinput_ = new od_ostream( new oqstream( stdinputbuf_ ) );
	}

	if ( pars.stdoutfnm_.isEmpty() )
	{
	    stdoutputbuf_ = new qstreambuf( *process_, false, false  );
	    stdoutput_ = new od_istream( new iqstream( stdoutputbuf_ ) );
	}
	else
	{
	    const QString filenm(
		pars.stdoutfnm_ == od_ostream::nullStream().fileName()
		? QProcess::nullDevice() : QString(pars.stdoutfnm_) );
	    process_->setStandardOutputFile( filenm );
	}

	if ( pars.stderrfnm_.isEmpty() )
	{
	    stderrorbuf_ = new qstreambuf( *process_, true, false  );
	    stderror_ = new od_istream( new iqstream( stderrorbuf_ ) );
	}
	else
	{
	    const QString filenm(
		pars.stderrfnm_ == od_ostream::nullStream().fileName()
		? QProcess::nullDevice() : QString(pars.stderrfnm_));
	    process_->setStandardErrorFile( filenm );
	}
    }

    const BufferString& workingdir = pars.workingdir_;
    if ( process_ )
    {
	if ( !workingdir.isEmpty() )
	{
	    const QString qworkdir( workingdir );
	    process_->setWorkingDirectory( qworkdir );
	}
	//TODO: use inconsole on Windows ?
	const QString qprog( mc.program() );
	QStringList qargs;
	mc.args().fill( qargs );
	process_->start( qprog, qargs, QIODevice::ReadWrite );
    }
    else
    {
	const bool res = startDetached( mc, pars.isconsoleuiprog_,
					workingdir );
	return res;
    }

    if ( !process_->waitForStarted(10000) ) //Timeout of 10 secs
    {
	return !catchError();
    }

    if ( wt4finish )
    {
	startMonitor();
	if ( process_->state()==QProcess::Running )
	    process_->waitForFinished(-1);

	bool res = process_->exitStatus() == QProcess::NormalExit;
	if ( res )
	{
	    const int exitcode = process_->exitCode();
	    clexitcodehpmgr_.setParam( this, exitcode );
	    res = exitcode == 0;
	}

	if ( createstreams )
	{
	    stderrorbuf_->detachDevice( true );
	    stdoutputbuf_->detachDevice( true );
	    stdinputbuf_->detachDevice( false );
	}

	deleteAndNullPtr( process_ );

	return res;
    }
#endif

    return true;
}


#ifndef OD_NO_QT
#if QT_VERSION < QT_VERSION_CHECK(5,10,0)

namespace OS {

static bool startDetachedLegacy( const OS::MachineCommand& mc,
	    bool inconsole, const char* workdir, PID_Type& pid )
{
#ifdef __win__
    BufferString comm( mc.program() );
    if ( !mc.args().isEmpty() )
    {
	BufferStringSet args( mc.args() );
	for ( auto arg : args )
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
    const QString qprog( mc.program() );
    QStringList qargs;
    mc.args().fill( qargs );
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


bool OS::CommandLauncher::startDetached( const OS::MachineCommand& mc,
					 bool inconsole, const char* workdir )
{
    if ( mc.isBad() )
	return false;

#ifndef OD_NO_QT

#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)

    const QString qprog( mc.program() );
    QStringList qargs;
    mc.args().fill( qargs );
    const QString qworkdir( workdir );
    qint64 qpid = 0;

    QProcess qproc;
    qproc.setProgram( qprog );
    qproc.setArguments( qargs );
    if ( !qworkdir.isEmpty() )
	qproc.setWorkingDirectory( qworkdir );

#ifdef __win__
    if ( inconsole )
    {
	qproc.setCreateProcessArgumentsModifier(
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

    if ( !qproc.startDetached(&qpid) )
	return false;

    pid_ = (PID_Type)qpid;
    return true;

#else

    return startDetachedLegacy( mc, inconsole, workdir, pid_ );

#endif

#else

    return false;

#endif
}


int OS::CommandLauncher::catchError()
{
    if ( !process_ )
	return 0;

    if ( !errmsg_.isEmpty() )
	return 1;

#ifndef OD_NO_QT
    switch ( process_->error() )
    {
	case QProcess::FailedToStart :
	    errmsg_ = tr("Cannot start process %1.");
	    break;
	case QProcess::Crashed :
	    errmsg_ = tr("%1 crashed.");
	    break;
	case QProcess::Timedout :
	    errmsg_ = tr("%1 timeout");
	    break;
	case QProcess::ReadError :
	    errmsg_ = tr("Read error from process %1");
	    break;
	case QProcess::WriteError :
	    errmsg_ = tr("Write error from process %1");
	    break;
	default :
	    break;
    }

    if ( !errmsg_.isEmpty() )
    {
	BufferString argstr( machcmd_.program() );
	if ( machcmd_.hasHostName() )
	    argstr.add( " @ " ).add( machcmd_.hostName() );
	errmsg_.arg( argstr );
	return 1;
    }
    return process_->exitCode();
#else

    return 0;
#endif
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
    if ( errmsg && cl.getStdError() )
	cl.getStdError()->getAll( *errmsg );

    return res;
}


void OD::DisplayErrorMessage( const char* msg )
{
    OS::MachineCommand machcomm( "od_DispMsg" );
    machcomm.addKeyedArg( "err", msg );
    machcomm.execute( OS::RunInBG );
}
