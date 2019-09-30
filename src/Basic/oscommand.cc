/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/


#include "oscommand.h"

#include "file.h"
#include "genc.h"
#include "commandlineparser.h"
#include "envvars.h"
#include "filepath.h"
#include "fixedstring.h"
#include "iopar.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "pythonaccess.h"
#include "staticstring.h"
#include "separstr.h"
#include "uistrings.h"

#ifndef OD_NO_QT
# include "qstreambuf.h"
# include <QProcess>
#endif

#include <iostream>

#ifdef __win__
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

static PtrMan<QProcessManager> processmanager = 0;
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


void OS::CommandExecPars::usePar( const IOPar& iop )
{
    IOPar* subpar = iop.subselect( sKeyExecPars );
    if ( !subpar || subpar->isEmpty() )
	{ delete subpar; return; }

    FileMultiString fms;
    subpar->get( sKeyMonitor, fms );
    auto sz = fms.size();
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
	launchtype_ = *fms[0] == 'W' ? Wait4Finish : RunInBG;
	isconsoleuiprog_ = *fms[0] == 'C';
    }

    subpar->get( sKeyPriorityLevel, prioritylevel_ );

    delete subpar;
}


void OS::CommandExecPars::fillPar( IOPar& iop ) const
{
    FileMultiString fms;
    fms += needmonitor_ ? "Yes" : "No";
    fms += monitorfnm_;
    iop.set( IOPar::compKey(sKeyExecPars,sKeyMonitor), fms );

    fms = launchtype_ == Wait4Finish ? "Wait" : "BG";
    fms += isconsoleuiprog_ ? "ConsoleUI" : "";
    iop.set( IOPar::compKey(sKeyExecPars,sKeyProgType), fms );

    iop.set( IOPar::compKey(sKeyExecPars,sKeyPriorityLevel), prioritylevel_ );
}


void OS::CommandExecPars::removeFromPar( IOPar& iop ) const
{
    iop.removeWithKeyPattern( BufferString(sKeyExecPars,".*") );
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


void OS::MachineCommand::setIsolated( const char* prognm )
{
    BufferString scriptcmd( GetODExternalScript() );
    CommandLauncher::addQuotesIfNeeded( scriptcmd );
    prognm_.set( scriptcmd );
    addArg( prognm );
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


static char* parseNextWord( char* ptr, BufferString& arg )
{
    arg.setEmpty();
    if ( !ptr || !*ptr )
	return ptr;
    mSkipBlanks( ptr );
    if ( !*ptr )
	return ptr;

    bool inquotes = *ptr == '"';
    if ( inquotes )
	ptr++;

    for ( ; *ptr; ptr++ )
    {
	if ( *ptr == '"' )
	    { inquotes = !inquotes; continue; }

	const bool isescaped = *ptr == '\\';
	if ( isescaped )
	{
	    ptr++;
	    if ( !*ptr )
		break;
	    if ( *ptr == '\\' )
		arg.add( *ptr );
	}
	if ( *ptr == ' ' && !isescaped && !inquotes )
	    { ptr++; break; }

	arg.add( *ptr );
    }
    return ptr;
}


bool OS::MachineCommand::setFromSingleStringRep( const char* inp,
						 bool ignorehostname )
{
    BufferString& pnm = const_cast<BufferString&>( prognm_ );
    pnm.setEmpty();
    args_.setEmpty();
    if ( !ignorehostname )
	hname_.setEmpty();

    BufferString inpcomm( inp );
    inpcomm.trimBlanks();
    if ( inpcomm.isEmpty() )
	return false;

    char* ptr = inpcomm.getCStr();
    if ( *ptr == '@' )
	ptr++;

    ptr = parseNextWord( ptr, pnm );

    BufferString hnm;
    const char* realcmd = extractHostName( pnm, hnm );
    if ( !ignorehostname )
	hname_ = hnm;
    mSkipBlanks( realcmd );
    if ( *realcmd == '@' ) realcmd++;
    BufferString tmp( realcmd );
    pnm = tmp;

    if ( isBad() )
	return false;

    BufferString arg;
    while ( ptr && *ptr )
    {
	ptr = parseNextWord( ptr, arg );
	if ( !arg.isEmpty() )
	    args_.add( arg );
    }
    return true;
}


BufferString OS::MachineCommand::getSingleStringRep( bool noremote ) const
{
    mDeclStaticString( ret );
    ret.setEmpty();

    if ( !noremote && !hname_.isEmpty() )
    {
#ifdef __win__
	ret.add( "\\\\" ).add( hname_ );
	File::Path fp( prognm_ );
	if ( !fp.isAbsolute() )
	    ret.add( "\\" );
#else
	ret.add( hname_ ).add( ":" );
#endif
    }
    ret.add( prognm_ );

    for ( auto arg : args_ )
    {
	BufferString str( *arg );
	str.replace( " ", "\\ " );
	ret.addSpace().add( str );
    }

    return ret.buf();
}


const char* OS::MachineCommand::extractHostName( const char* str,
						 BufferString& hnm )
{
    hnm.setEmpty();
    if ( !str )
	return str;

    mSkipBlanks( str );
    BufferString inp( str );
    char* ptr = inp.getCStr();
    const char* rest = str;

#ifdef __win__

    if ( *ptr == '\\' && *(ptr+1) == '\\' )
    {
	ptr += 2;
	char* phnend = firstOcc( ptr, '\\' );
	if ( phnend ) *phnend = '\0';
	hnm = ptr;
	rest += hnm.size() + 2;
    }

#else

    while ( *ptr && !iswspace(*ptr) && *ptr != ':' )
	ptr++;

    if ( *ptr == ':' )
    {
	if ( *(ptr+1) == '/' && *(ptr+2) == '/' )
	{
	    inp.add( "\nlooks like a URL. Not supported (yet)" );
	    ErrMsg( inp ); rest += FixedString(str).size();
	}
	else
	{
	    *ptr = '\0';
	    hnm = inp;
	    rest += hnm.size() + 1;
	}
    }

#endif

    return rest;
}


#ifdef __win__

static BufferString getUsableWinCmd( const char* fnm )
{
    BufferString ret( fnm );

    BufferString execnm( fnm );
    char* ptr = execnm.find( ':' );
    if ( !ptr )
	return ret;

    char* args=0;

    // if only one char before the ':', it must be a drive letter.
    if ( ptr == execnm.buf() + 1 )
    {
	ptr = firstOcc( ptr , ' ' );
	if ( ptr ) { *ptr = '\0'; args = ptr+1; }
    }
    else if ( ptr == execnm.buf()+2)
    {
	char sep = *execnm.buf();
	if ( sep == '\"' || sep == '\'' )
	{
	    execnm=fnm+1;
	    ptr = execnm.find( sep );
	    if ( ptr ) { *ptr = '\0'; args = ptr+1; }
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

	char buf[41];
	strm.getC( buf, 40 );
	BufferString line( buf );

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

    ret = "\"";
    File::Path interpfp;

    const char* cygdir = WinUtils::getCygDir();
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

    ret.add( interpfp.fullPath() ).add( "\" '" )
	.add( File::Path(execnm).fullPath(File::Path::Unix) ).add( "'" );
    if ( args && *args )
	ret.add( " " ).add( args );

    return ret;
}

#endif


static BufferString getUsableUnixCmd( const char* fnm )
{
    BufferString ret( fnm );
    if ( File::Path(fnm).isAbsolute() &&
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


static BufferString getUsableCmd( const char* fnm )
{
#ifdef __win__
    return getUsableWinCmd( fnm );
#else
    return getUsableUnixCmd( fnm );
#endif
}


void OS::CommandLauncher::addQuotesIfNeeded( BufferString& word )
{
    if ( word.find(' ') && word.firstChar() != '"' )
	word.quote( '"' );
}


BufferString OS::MachineCommand::getExecCommand() const
{
    BufferString prognm = getUsableCmd( prognm_ );

    BufferString ret;
    const BufferString localhostnm( GetLocalHostName() );
    if ( remexec_.isEmpty() || hname_.isEmpty() || hname_ == localhostnm )
	ret.set( prognm );
    else
    {
	if ( remexec_ != odRemExecCmd() )
	    // Unix to Unix only (ssh/rsh)
	    ret.set( remexec_ ).addSpace().add( hname_ );
	else
	{
	    ret.set( remexec_ )
	        .addSpace()
	        .add( CommandLineParser::createKey(sKeyRemoteHost()) )
	        .addSpace().add( hname_ ).addSpace()
	        .add( CommandLineParser::createKey(sKeyRemoteCmd()) );
	}
	ret.addSpace().add( prognm );
    }

    return ret;
}


bool OS::MachineCommand::execute( LaunchType lt )
{
    return CommandLauncher(*this).execute( lt );
}


bool OS::MachineCommand::execute( const CommandExecPars& execpars )
{
    return CommandLauncher(*this).execute( execpars );
}


bool OS::MachineCommand::execute( BufferString& out, BufferString* err )
{
    return CommandLauncher(*this).execute( out, err );
}


BufferString OS::MachineCommand::runAndCollectOutput( BufferString* errmsg )
{
    BufferString ret;
    if ( !CommandLauncher(*this).execute(ret,errmsg) )
	ret.setEmpty();
    return ret;
}


// OS::CommandLauncher

OS::CommandLauncher::CommandLauncher( const OS::MachineCommand& mc )
    : odprogressviewer_(sODProgressViewerProgName)
    , process_( 0 )
    , stderror_( 0 )
    , stdoutput_( 0 )
    , stdinput_( 0 )
    , stderrorbuf_( 0 )
    , stdoutputbuf_( 0 )
    , stdinputbuf_( 0 )
    , pid_( 0 )
    , machcmd_(mc)
{
    reset();
}


bool OS::CommandLauncher::execute( BufferString& out, BufferString* err )
{
    CommandExecPars execpars( Wait4Finish );
    execpars.createstreams( true );
    if ( !execute(execpars) )
	return false;

    getStdOutput()->getAll( out );
    if ( err )
	getStdError()->getAll( *err );
    return true;
}


OS::CommandLauncher::~CommandLauncher()
{
#ifndef OD_NO_QT
    reset();
#endif
}


OS::CommandLauncher::ProcID OS::CommandLauncher::processID() const
{
#ifndef OD_NO_QT
    if ( !process_ )
	return pid_;
# ifdef __win__
    const PROCESS_INFORMATION* pi = (PROCESS_INFORMATION*) process_->pid();
    return pi->dwProcessId;
# else
    return process_->pid();
# endif
#else
    return 0;
#endif
}


void OS::CommandLauncher::reset()
{
    deleteAndZeroPtr( stderror_ );
    deleteAndZeroPtr( stdoutput_ );
    deleteAndZeroPtr( stdinput_ );

    stderrorbuf_ = 0;
    stdoutputbuf_ = 0;
    stdinputbuf_ = 0;

#ifndef OD_NO_QT
    if ( process_ && process_->state()!=QProcess::NotRunning )
    {
	manageQProcess( process_ );
	process_ = 0;
    }
    deleteAndZeroPtr( process_ );
#endif
    errmsg_.setEmpty();
    monitorfnm_.setEmpty();
    progvwrcmd_.setEmpty();
    redirectoutput_ = false;
}


void OS::CommandLauncher::set( const OS::MachineCommand& cmd )
{
    machcmd_ = cmd;
    reset();
}


bool OS::CommandLauncher::execute( const OS::CommandExecPars& pars )
{
    reset();
    if ( machcmd_.isBad() )
	{ errmsg_ = toUiString("Command is invalid"); return false; }

    if ( FixedString(machcmd_.program()).contains("python") )
    {
	const File::Path pythfp( machcmd_.program() );
	if ( pythfp.nrLevels() < 2 ||
	    (pythfp.exists() && pythfp.fileName().contains("python") ) )
	    pErrMsg("Python commands should be run using OD::PythA().execute");
    }

    MachineCommand mcmd( machcmd_ );
    BufferString toexec = machcmd_.getExecCommand();
    if ( toexec.isEmpty() )
	{ errmsg_ = toUiString("Empty command to execute"); return false; }

    if ( !mIsZero(pars.prioritylevel_,1e-2f) )
	mcmd.addKeyedArg( CommandExecPars::sKeyPriority(), pars.prioritylevel_);

    uiString cannotlaunchstr = toUiString( "Cannot launch '%1'" );
    if ( pars.isconsoleuiprog_ )
    {
#ifndef __win__
	BufferString str =
	    File::Path( GetSoftwareDir(true), "bin",
		    "od_exec_consoleui.scr" ).fullPath();
	addQuotesIfNeeded( str );
	str.add( " " );
	toexec.insertAt( 0, str );
#endif
	const bool res = doExecute( toexec, pars.launchtype_==Wait4Finish, true,
				    pars.createstreams_ );
	if ( errmsg_.isEmpty() )
	    errmsg_.set( cannotlaunchstr.arg( toexec ) );
	return res;
    }

    if ( pars.needmonitor_ )
    {
	monitorfnm_ = pars.monitorfnm_;
	if ( monitorfnm_.isEmpty() )
	{
	    monitorfnm_ = File::Path::getTempFullPath( "mon", "txt" );
	    redirectoutput_ = true;
	}

	if ( File::exists(monitorfnm_) && !File::remove(monitorfnm_) )
	    return false;
    }

    const bool ret = doExecute( toexec, pars.launchtype_==Wait4Finish, false,
				pars.createstreams_ );
    if ( !ret )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_.set( cannotlaunchstr.arg( toexec ) );
	return false;
    }

    if ( pars.launchtype_==RunInBG )
	startMonitor();

    return ret;
}


bool OS::CommandLauncher::startServer( bool ispyth, double waittm )
{
    CommandExecPars execpars( RunInBG );
    execpars.createstreams_ = true;
	// this has to be done otherwise we cannot pick up any error messages
    pid_ = -1;
    if ( ispyth )
    {
	execpars.prioritylevel_ = 0.f;
	if ( !OD::PythA().execute(machcmd_,execpars,&pid_,&errmsg_) )
	    pid_ = -1;
    }
    else
    {
	if ( !execute(execpars) )
	    pid_ = -1;
    }

    if ( pid_ > 0 )
    {
	const BufferString pidfnm( "od_subproc_", pid_, ".pid" );
#ifdef __win__
	const BufferString pidfnmfullpath( "C:\\TEMP\\", pidfnm );
#else
	const BufferString pidfnmfullpath( "/tmp/", pidfnm );
#endif
	if ( File::exists(pidfnmfullpath) )
	{
	    BufferString pidstr;
	    if ( File::getContent(pidfnmfullpath,pidstr) )
		pid_ = pidstr.toInt();
	    File::remove( pidfnmfullpath );
	}
    }

    if ( pid_ < 1 )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = uiStrings::phrCannotStart( machcmd_.getExecCommand());
	return false;
    }

    while ( waittm > 0 )
    {
	waittm -= 1;
	sleepSeconds( 1 );
	if ( isProcessAlive(pid_) )
	    break;
    }

    if ( !isProcessAlive(pid_) )
    {
	if ( ispyth )
	    errmsg_ = toUiString(OD::PythA().lastOutput(true,nullptr));

	if ( errmsg_.isEmpty() )
	    errmsg_ = tr("Server process (%1) exited early")
			    .arg( machcmd_.getExecCommand() );
	return false;
    }

    return true;
}


void OS::CommandLauncher::startMonitor()
{
    if ( monitorfnm_.isEmpty() )
	return;

    const BufferString monitfnmnoquotes = monitorfnm_;
    monitorfnm_.quote( '\"' );
    MachineCommand progvwrcmd( odprogressviewer_ );
    progvwrcmd.addKeyedArg( "inpfile", monitorfnm_ );
    progvwrcmd.addKeyedArg( "pid", processID() );

    OS::CommandLauncher progvwrcl( progvwrcmd );
    if ( !progvwrcl.execute(RunInBG) )
    {
	// sad ... but the process has been launched anyway
	ErrMsg( toUiString( "[Monitoring does not start] %1" )
		.arg( progvwrcl.errorMsg() ) );
    }
}


void OS::CommandLauncher::addShellIfNeeded( BufferString& cmd )
{
    bool needsshell = cmd.find('|') || cmd.find('<') || cmd.find( '>' );
#ifdef __win__
    if ( !needsshell )
	needsshell = cmd.startsWith( "echo ", CaseInsensitive );
#endif
    if ( !needsshell )
	return;

    const bool cmdneedsquotes = cmd.firstChar() != '"';
    BufferString orgcmd = cmd;
#ifdef __win__
    cmd.set( "cmd /c " );
#else
    cmd.set( "/bin/sh -c " );
#endif
    if ( cmdneedsquotes )
	cmd.add( '"' );
    cmd.add( orgcmd );
    if ( cmdneedsquotes )
	cmd.add( '"' );
}


bool OS::CommandLauncher::doExecute( const char* inpcmd, bool wt4finish,
				     bool inconsole, bool createstreams )
{
    if ( *inpcmd == '@' )
	inpcmd++;
    if ( !*inpcmd )
	{ errmsg_ = tr("Command is empty"); return false; }

    if ( process_ )
    {
	errmsg_ = tr("Process is already running ('%1')").arg( inpcmd );
	return false;
    }

    BufferString cmd = inpcmd;
    addShellIfNeeded( cmd );

    DBG::message( BufferString("About to execute:\n",cmd) );

#ifndef OD_NO_QT
    process_ = wt4finish || createstreams ? new QProcess : 0;

    if ( createstreams )
    {
	if ( !monitorfnm_.isEmpty() )
	    process_->setStandardOutputFile( monitorfnm_.buf() );
	else
	{
	    stdinputbuf_ = new qstreambuf( *process_, false, false );
	    stdinput_ = new od_ostream( new oqstream( stdinputbuf_ ) );

	    stdoutputbuf_ = new qstreambuf( *process_, false, false  );
	    stdoutput_ = new od_istream( new iqstream( stdoutputbuf_ ) );

	    stderrorbuf_ = new qstreambuf( *process_, true, false  );
	    stderror_ = new od_istream( new iqstream( stderrorbuf_ ) );
	}
    }

    if ( process_ )
    {
	QStringList qstrlist;
	machcmd_.args().fill( qstrlist );
	process_->start( cmd.str(), qstrlist, QIODevice::ReadWrite );
    }
    else
    {
	BufferString cmdline( cmd );
	for ( auto str : machcmd_.args() )
	{
	    BufferString arg( *str );
	    addQuotesIfNeeded( arg );
	    cmdline.addSpace().add( arg );
	}
	const bool res = startDetached( cmdline, inconsole );
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

	const bool res = process_->exitStatus()==QProcess::NormalExit;

	if ( createstreams )
	{
	    stderrorbuf_->detachDevice( true );
	    stdoutputbuf_->detachDevice( true );
	    stdinputbuf_->detachDevice( false );
	}

	deleteAndZeroPtr( process_ );

	return res;
    }
#endif

    return true;
}


bool OS::CommandLauncher::startDetached( const char* comm, bool inconsole )
{
#ifdef __win__
    if ( !inconsole )
    {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(STARTUPINFO) );
	ZeroMemory( &pi, sizeof(pi) );
	si.cb = sizeof( STARTUPINFO );

	const bool res = CreateProcess( NULL,
				const_cast<char*>(comm),
				NULL,   // Process handle not inheritable.
				NULL,   // Thread handle not inheritable.
				FALSE,  // Set handle inheritance.
				CREATE_NO_WINDOW,   // Creation flags.
				NULL,   // Use parent's environment block.
				NULL,   // Use parent's starting directory.
				&si, &pi );

	if ( res )
	{
	    pid_ = (int)pi.dwProcessId;
	    CloseHandle( pi.hProcess );
	    CloseHandle( pi.hThread );
	}

	return res;
    }
#endif

#ifndef OD_NO_QT

    CommandLineParser parser( comm );
    if ( parser.getExecutable().isEmpty() )
	return false;

    QStringList args;
    for ( int idx=0; idx<parser.nrArgs(); idx++ )
    {
	args.append( QString(parser.getArg(idx).str()) );
    }

    qint64 qpid = 0;
    if ( !QProcess::startDetached(parser.getExecutable().str(),args,"",&qpid) )
	return false;

    pid_ = (int)qpid;
    return true;

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
	    errmsg_ = tr("Read error from process %1");
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


bool OS::Unsafe__use_MachineCommand_instead( const char* cmd, LaunchType lt )
{
    BufferString prognm( cmd );
    char* args = prognm.getCStr();
    mSkipBlanks( args ); mSkipNonBlanks( args );
    *args++ = '\0';
    mSkipBlanks( args );
    OS::MachineCommand machcomm( prognm, args );
    return machcomm.execute( lt );
}


void OD::DisplayErrorMessage( const char* msg )
{
    OS::MachineCommand machcomm( "od_DispMsg" );
    machcomm.addKeyedArg( "err", msg );
    machcomm.execute( OS::RunInBG );
}
