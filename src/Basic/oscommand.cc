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
#include "od_ostream.h"
#include "filepath.h"
#include "perthreadrepos.h"
#include "fixedstring.h"

#ifdef __win__
# include "winutils.h"
# include "od_istream.h"
# include <windows.h>
#include <stdlib.h>

# ifdef __msvc__
#  define popen _popen
#  define pclose _pclose
#  define fileno(s) _fileno(s)
#  include "winstreambuf.h"
# endif
#endif

BufferString OS::MachineCommand::defremexec_( "ssh" );
static const char* sODProgressViewerProgName = "od_ProgressViewer";


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

    while ( *ptr && !isspace(*ptr) && *ptr != ':' )
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
{
    set( mc );
}


void OS::CommandLauncher::reset()
{
    processid_ = 0;
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
	if ( monitorfnm_.isEmpty() )
	{
	    monitorfnm_ = FilePath::getTempName("txt");
	    redirectoutput_ = true;
	    localcmd.add( __iswin__ ? "" : BufferString(" >",monitorfnm_) );
	}
    }

    ret = doExecute( localcmd, pars.launchtype_==Wait4Finish );
    if ( !ret )
	return false;

    if ( !monitorfnm_.isEmpty() )
    {
	progvwrcmd_.set( odprogressviewer_ )
	    .add( " --inpfile " ).add( monitorfnm_ )
	    .add( " --pid " ).add( processID() );

	redirectoutput_ = false;
	if ( !doExecute(progvwrcmd_,false) )
	    ErrMsg("Cannot launch progress viewer");
			// sad ... but the process has been launched
    }

    return ret;
}


bool OS::CommandLauncher::doExecute( const char* comm, bool wt4finish,
								bool inconsole )
{
    if ( *comm == '@' )
	comm++;
    if ( !*comm )
	{ errmsg_.set( "Command is empty" ); return false; }

# ifndef __win__

    BufferString oscmd( comm );
    if ( !wt4finish )
	oscmd += "&";
    int res = system( oscmd );
    return !res;

# else

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory( &pi, sizeof(pi) );
    si.cb = sizeof(STARTUPINFO);

    if ( redirectoutput_ )
    {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE hlog = CreateFile( monitorfnm_,
				  FILE_APPEND_DATA,
				  FILE_SHARE_WRITE | FILE_SHARE_READ,
				  &sa,
				  OPEN_ALWAYS,
				  FILE_ATTRIBUTE_NORMAL,
				  NULL );

	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdError = hlog;
	si.hStdOutput = hlog;
    }

    const bool HINH = !monitorfnm_.isEmpty();
    DWORD FLAG = inconsole ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW;

    //Start the child process.
    int res = CreateProcess( NULL,	// No module name (use command line).
			     const_cast<char*>( comm ),
			     NULL,	// Process handle not inheritable.
			     NULL,	// Thread handle not inheritable.
			     HINH,	// Set handle inheritance.
			     FLAG,	// Creation flags.
			     NULL,	// Use parent's environment block.
			     NULL,	// Use parent's starting directory.
			     &si, &pi );

    if ( res )
    {
	if ( wt4finish )  WaitForSingleObject( pi.hProcess, INFINITE );
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
	processid_ = pi.dwProcessId;
    }
    else
    {
	char *errmsg = 0;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
		       FORMAT_MESSAGE_FROM_SYSTEM,
		       0, GetLastError(), 0, (char*)&errmsg, 1024, NULL) ;

	errmsg_ = errmsg;
	LocalFree(errmsg);
    }

    return res;

# endif
}


static bool doExecOSCmd( const char* cmd, OS::LaunchType ltyp, bool isodprog )
{
    const OS::MachineCommand mc( cmd );
    OS::CommandLauncher cl( mc );
    OS::CommandExecPars cp( isodprog );
    cp.launchtype( ltyp );
    return cl.execute( cp );
}


bool OS::ExecCommand( const char* cmd, OS::LaunchType ltyp )
{
    return doExecOSCmd( cmd, ltyp, false );
}


bool ExecODProgram( const char* prognm, const char* args, OS::LaunchType ltyp )
{
    const BufferString cmd( prognm, " ", args );
    return doExecOSCmd( cmd, ltyp, true );
}
