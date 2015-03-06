/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "oscommand.h"

#include "file.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "filepath.h"
#include "perthreadrepos.h"
#include "fixedstring.h"
#include "separstr.h"
#include "iopar.h"

#ifndef OD_NO_QT
# include "qstreambuf.h"
# include <QProcess>
#endif

#include <iostream>

#ifdef __win__
# include "winutils.h"
# include <windows.h>
#include <stdlib.h>
#endif

BufferString OS::MachineCommand::defremexec_( "ssh" );
static const char* sODProgressViewerProgName = "od_ProgressViewer";
static const char* sKeyExecPars = "ExecPars";
static const char* sKeyMonitor = "Monitor";
static const char* sKeyProgType = "ProgramType";
static const char* sKeyPriorityLevel = "PriorityLevel";


void OS::CommandExecPars::usePar( const IOPar& iop )
{
    IOPar* subpar = iop.subselect( sKeyExecPars );
    if ( !subpar || subpar->isEmpty() )
	{ delete subpar; return; }

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


OS::MachineCommand::MachineCommand( const char* comm )
    : remexec_(defremexec_)
{
    setFromSingleStringRep( comm, false );
}


OS::MachineCommand::MachineCommand( const char* comm, const char* hostnm )
    : hname_(hostnm)
    , remexec_(defremexec_)
{
    setFromSingleStringRep( comm, true );
}


bool OS::MachineCommand::setFromSingleStringRep( const char* inp,
						 bool ignorehostname )
{
    comm_.setEmpty();
    if ( !ignorehostname )
	hname_.setEmpty();

    BufferString inpcomm( inp );
    inpcomm.trimBlanks();
    if ( inpcomm.isEmpty() )
	return false;

    char* ptr = inpcomm.getCStr();
    if ( *ptr == '@' )
	ptr++;
    mSkipBlanks( ptr );
    comm_ = ptr;

    BufferString hnm;
    const char* realcmd = extractHostName( comm_, hnm );
    if ( !ignorehostname )
	hname_ = hnm;
    mSkipBlanks( realcmd );
    if ( *realcmd == '@' ) realcmd++;
    BufferString tmp( realcmd );
    comm_ = tmp;

    return !isBad();
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


const char* OS::MachineCommand::getSingleStringRep() const
{
    mDeclStaticString( ret );
    ret.setEmpty();

    if ( !hname_.isEmpty() )
    {
#ifdef __win__
	ret.add( "\\\\" ).add( hname_ );
#else
	ret.add( hname_ ).add( ":" );
#endif
    }
    ret.add( comm_ );

    return ret.buf();
}


#ifdef __win__

static const char* getCmd( const char* fnm )
{
    BufferString execnm( fnm );

    char* ptr = execnm.find( ':' );

    if ( !ptr )
	return fnm;

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
	return fnm;

    if ( execnm.contains(".exe") || execnm.contains(".EXE")
       || execnm.contains(".bat") || execnm.contains(".BAT")
       || execnm.contains(".com") || execnm.contains(".COM") )
	return fnm;

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
	    return fnm;

	char buf[41];
	strm.getC( buf, 40 );
	BufferString line( buf );

	if ( !line.contains("#!") && !line.contains("# !") )
	    return fnm;

	if ( line.contains("csh") )
	    interp = "tcsh.exe";
	else if ( line.contains("awk") )
	    interp = "awk.exe";
	else if ( line.contains("sh") )
	    interp = "sh.exe";
    }

    if ( !interp )
	return fnm;

    mDeclStaticString( fullexec );

    fullexec = "\"";
    FilePath interpfp;

    if ( getCygDir() )
    {
	interpfp.set( getCygDir() );
	interpfp.add("bin").add(interp);
    }

    if ( !File::exists( interpfp.fullPath() ) )
    {
	interpfp.set( GetSoftwareDir(true) );
	interpfp.add("bin").add("win").add("sys").add(interp);
    }

    fullexec.add( interpfp.fullPath() ).add( "\" '" )
	.add( FilePath(execnm).fullPath(FilePath::Unix) ).add( "'" );
    if ( args && *args )
	fullexec.add( " " ).add( args );

    return fullexec.buf();
}

#endif


#ifdef __win__
# define mFullCommandStr getCmd( comm_ )
#else
# define mFullCommandStr comm_.buf()
#endif

BufferString OS::MachineCommand::getLocalCommand() const
{
    BufferString ret;
	    //TODO handle hname_ != local host on Windows correctly
    if ( hname_.isEmpty() || __iswin__ )
	ret = mFullCommandStr;
    else
	ret.set( remexec_ ).add( " " ).add( hname_ ).add( " " ).add( comm_ );
    return ret;
}


// OS::CommandLauncher

OS::CommandLauncher::CommandLauncher( const OS::MachineCommand& mc)
    : odprogressviewer_(FilePath(GetBinPlfDir(),sODProgressViewerProgName)
			    .fullPath())
    , process_( 0 )
    , stderror_( 0 )
    , stdoutput_( 0 )
    , stdinput_( 0 )
    , stderrorbuf_( 0 )
    , stdoutputbuf_( 0 )
    , stdinputbuf_( 0 )
{
    set( mc );
}


OS::CommandLauncher::~CommandLauncher()
{
#ifndef OD_NO_QT
    if ( process_ && process_->state()!=QProcess::NotRunning )
	process_->waitForFinished();

    reset();
#endif
}


int OS::CommandLauncher::processID() const
{
#ifndef OD_NO_QT
    if ( !process_ )
	return 0;
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
	{ errmsg_ = "Command is invalid"; return false; }

    BufferString localcmd = machcmd_.getLocalCommand();
    if ( localcmd.isEmpty() )
	{ errmsg_ = "Empty command to execute"; return false; }

    bool ret = false;
    if ( pars.isconsoleuiprog_ )
    {
#ifndef __win__
	FilePath fp( GetSoftwareDir(true), "bin", "od_exec_consoleui.scr " );
	localcmd.insertAt( 0, fp.fullPath() );
#endif
	return doExecute( localcmd, pars.launchtype_==Wait4Finish, true );
    }

    if ( pars.needmonitor_ )
    {
	monitorfnm_ = pars.monitorfnm_;

#ifdef __win__
	if ( monitorfnm_.isEmpty() )
	{
	    monitorfnm_ = FilePath::getTempName("txt");
	    redirectoutput_ = true;
	}
#endif
	
	if ( File::exists(monitorfnm_) )
	    File::remove(monitorfnm_);

#ifndef __win__
	monitorfnm_.quote( '\"' );
	localcmd.add( " --needmonitor" );
	if ( !monitorfnm_.isEmpty() )
	    localcmd.add( " --monitorfnm " ).add( monitorfnm_.buf() );

	BufferString launchercmd( "\"",
		FilePath(GetBinPlfDir(),"od_batch_launcher").fullPath() );
	launchercmd.add( "\" " ).add( localcmd );
	return doExecute( launchercmd, pars.launchtype_==Wait4Finish );
#endif
    
    }

    ret = doExecute( localcmd, pars.launchtype_==Wait4Finish );
    if ( !ret )
	return false;

    if ( !monitorfnm_.isEmpty() )
    {
	progvwrcmd_.set( "\"" ).add( odprogressviewer_ )
	    .add( "\" --inpfile " ).add( monitorfnm_ )
	    .add( " --pid " ).add( processID() );

	redirectoutput_ = false;
	if ( !doExecute(progvwrcmd_,false) )
	    ErrMsg("Cannot launch progress viewer");
			// sad ... but the process has been launched
    }

    return ret;
}


void OS::CommandLauncher::addShellIfNeeded( BufferString& cmd )
{
    const bool needsshell = cmd.find('|') || cmd.find('<') || cmd.find( '>' );
    if ( needsshell )
    {
	if ( cmd.find( "\"" ) )
	{
	    pFreeFnErrMsg("Commands with quote-signs not supported",
		          "addShellIfNeeded" );
	}

	const BufferString comm = cmd;
#ifdef __msvc__
	cmd = "cmd -c \"";
#else
	cmd = "sh -c \"";
#endif
	cmd.add( comm );
	cmd.add( "\"" );
    }
}


bool OS::CommandLauncher::doExecute( const char* comm, bool wt4finish,
				     bool inconsole )
{
    if ( *comm == '@' )
	comm++;
    if ( !*comm )
	{ errmsg_ = tr( "Command is empty" ); return false; }

    if ( process_ )
    {
	errmsg_ = tr( "Command is already running" );
	return false;
    }

    BufferString cmd = comm;
    addShellIfNeeded( cmd );

#ifdef __debug__
    od_cout() << "About to execute:\n" << cmd << od_endl;
#endif

#ifndef OD_NO_QT
    process_ = new QProcess;

    stdinputbuf_ = new qstreambuf( *process_, false, false );
    stdinput_ = new od_ostream( new oqstream( stdinputbuf_ ) );

    stdoutputbuf_ = new qstreambuf( *process_, false, false  );
    stdoutput_ = new od_istream( new iqstream( stdoutputbuf_ ) );

    stderrorbuf_ = new qstreambuf( *process_, true, false  );
    stderror_ = new od_istream( new iqstream( stderrorbuf_ ) );

    process_->start( cmd.buf(), QIODevice::ReadWrite );
    if ( !process_->waitForStarted(10000) ) //Timeout of 10 secs
    {
	return !catchError();
    }

    if ( wt4finish )
    {
	if ( process_->state()==QProcess::Running )
	    process_->waitForFinished(-1);

	const bool res = process_->exitStatus()==QProcess::NormalExit;

	stderrorbuf_->detachDevice( true );
	stdoutputbuf_->detachDevice( true );
	stdinputbuf_->detachDevice( false );

	deleteAndZeroPtr( process_ );

	return res;
    }
#endif

    return true;
}


int OS::CommandLauncher::catchError()
{
    if ( !process_ )
	return 0;

    if ( errmsg_.isSet() )
	return 1;

#ifndef OD_NO_QT
    switch ( process_->error() )
    {
	case QProcess::FailedToStart :
	    errmsg_ = tr( "Cannot start process %1." );
	    break;
	case QProcess::Crashed :
	    errmsg_ = tr( "%1 crashed." );
	    break;
	case QProcess::Timedout :
	    errmsg_ = tr( "%1 timeout" );
	    break;
	case QProcess::ReadError :
	    errmsg_ = tr( "Read error from process %1");
	    break;
	case QProcess::WriteError :
	    errmsg_ = tr( "Read error from process %1");
	    break;
	default :
	    break;
    }

    if ( errmsg_.isSet() )
    {
	return process_->exitCode();
    }
#endif

    return 0;
}


static bool doExecOSCmd( const char* cmd, OS::LaunchType ltyp, bool isodprog,
			 BufferString* stdoutput, BufferString* stderror )
{
    const OS::MachineCommand mc( cmd );
    OS::CommandLauncher cl( mc );
    OS::CommandExecPars cp( isodprog );
    cp.launchtype( ltyp );
    const bool ret = cl.execute( cp );
    if ( stdoutput )
	cl.getStdOutput()->getAll( *stdoutput );

    if ( stderror )
	cl.getStdError()->getAll( *stderror );

    return ret;
}


bool OS::ExecCommand( const char* cmd, OS::LaunchType ltyp, BufferString* out,
		      BufferString* err )
{
    if ( ltyp!=Wait4Finish )
	out = 0;

    return doExecOSCmd( cmd, ltyp, false, out, err );
}


bool ExecODProgram( const char* prognm, const char* args, OS::LaunchType ltyp )
{
    const BufferString cmd( prognm, " ", args );
    return doExecOSCmd( cmd, ltyp, true, 0, 0 );
}
