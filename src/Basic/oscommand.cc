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
#include <stdio.h>
#include <stdlib.h>
#include <QProcess>

#ifdef __win__
# include "winutils.h"
# include "od_istream.h"
# include <windows.h>

# ifdef __msvc__
#  define popen _popen
#  define pclose _pclose
#  define fileno(s) _fileno(s)
#  include "winstreambuf.h"
# endif
#endif

BufferString OSCommand::defremexec_( "ssh" );
static const char* sODProgressViewerProgName = "od_ProgressViewer";

//#define __USE_QPROCESS__ 1

bool ExecOSCmd( const char* comm, bool inconsole, bool inbg )
{
    OSCommand oscmd( comm );
    if ( oscmd.isBad() )
	return false;

    return oscmd.execute( inconsole, inbg );
}


static bool doExecOSCmd( const char* comm, bool inconsole, bool inbg )
{
    if ( !comm || !*comm )
	return false;
    if ( *comm == '@' )
	comm++;
    if ( !*comm )
	return false;

#ifndef __win__

    BufferString oscmd( comm );
    if ( inbg )
	oscmd += "&";
    int res = system( oscmd );
    return !res;

#else

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory( &pi, sizeof(pi) );
    si.cb = sizeof(STARTUPINFO);

    if ( !inconsole )
    {
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
	si.wShowWindow = SW_HIDE;
    }

   //Start the child process.
    int res = CreateProcess( NULL,	// No module name (use command line).
        const_cast<char*>( comm ),
        NULL,				// Process handle not inheritable.
        NULL,				// Thread handle not inheritable.
        FALSE,				// Set handle inheritance to FALSE.
        0,				// Creation flags.
        NULL,				// Use parent's environment block.
        NULL,			// Use parent's starting directory.
        &si, &pi );

    if ( res )
    {
	if ( !inbg )  WaitForSingleObject( pi.hProcess, INFINITE );
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
    }
    else
    {
	char *ptr = NULL;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM,
	    0, GetLastError(), 0, (char *)&ptr, 1024, NULL);

	fprintf(stderr, "\nError: %s\n", ptr);
	LocalFree(ptr);
    }

    return res;

#endif
}


bool ExecODProgram( const char* prognm, const char* filenm, int nicelvl,
		    const char* args )
{
    const bool scriptinbg = __iswin__ || __ismac__;

#ifdef __win__

    BufferString cmd( prognm );
    if ( filenm && *filenm )
	cmd.add( " \"" ).add( filenm ).add( "\"" );
    cmd.add( args );

    return ExecOSCmd( cmd, true, scriptinbg );

#else

    BufferString cmd( mGetExecScript(), " --inbg" );
#ifdef __debug__
    cmd.add( " --debug" );
#endif
    if ( nicelvl )
	cmd.add( " --nice " ).add( nicelvl );

    cmd.add( " " ).add( prognm );
    if ( filenm && *filenm )
    {
	const FilePath fp( filenm );
	cmd.add( " \'" ).add( fp.fullPath(FilePath::Unix) ).add( "\'" );
    }
    if ( args && *args )
	cmd.add( " " ).add( args );

    OSCommand oscmd( cmd );
    return oscmd.execute( true, scriptinbg );


#endif

}



OSCommand::OSCommand( const char* comm, const char* hostnm )
    : hname_(hostnm)
    , remexec_(defremexec_)
{
    set( comm, hname_.isEmpty() );
}


bool OSCommand::set( const char* inp, bool lookforhostname )
{
    comm_.setEmpty();
    if ( lookforhostname )
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

    if ( lookforhostname )
    {
	const char* realcmd = extractHostName( comm_, hname_ );
	mSkipBlanks( realcmd );
	if ( *realcmd == '@' ) realcmd++;
	BufferString tmp( realcmd );
	comm_ = tmp;
    }

    return !isBad();
}


const char* OSCommand::extractHostName( const char* str, BufferString& hnm )
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


const char* OSCommand::get() const
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


bool OSCommand::execute( bool inconsole, bool inbg ) const
{
    BufferString cmd;

#ifdef __win__
    if ( inconsole )
	mkConsoleCmd( cmd );
    else
#endif
	mkOSCmd( true, cmd );

    return doExecOSCmd( cmd, inconsole, inbg );
}


bool OSCommand::execute( const OSCommandExecPars& pars, bool isodprog ) const
{
    CommandLauncher cl( comm_, "" );
    return cl.execute( pars, isodprog );
}

#ifdef __win__

void OSCommand::mkConsoleCmd( BufferString& comm ) const
{
    const BufferString batchfnm(
		FilePath(FilePath::getTempDir(),"odtmp.bat").fullPath() );
    od_ostream batchstream( batchfnm );
    batchstream << "@echo off\n" << comm_.buf() << "\npause\n";
    comm = batchfnm;
}


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
	interpfp.set( GetSoftwareDir(0) );
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

void OSCommand::mkOSCmd( bool forread, BufferString& cmd ) const
{
	    //TODO handle hname_ != local host on Windows correctly
    if ( hname_.isEmpty() || __iswin__ )
	cmd = mFullCommandStr;
    else
	sprintf( cmd.getCStr(), "%s %s %s",
			    remexec_.buf(), hname_.buf(), comm_.buf() );
}


// CommandLauncher

CommandLauncher::CommandLauncher( const char* command, const char* param )
    : command_(command)
    , parameters_(param)
    , processid_(0)
{
    makeFullCommand();
}


CommandLauncher::~CommandLauncher()
{}


void CommandLauncher::makeFullCommand()
{
    FilePath cmdfp( command_ );
    fullcommand_.add( cmdfp.isAbsolute() ? command_ 
				: FilePath(GetBinPlfDir(),command_).fullPath() )
				  .add( " " ).add( parameters_ );

    odprogressviewer_ =
	FilePath(GetBinPlfDir(),sODProgressViewerProgName).fullPath();
}


bool CommandLauncher::execute( const OSCommandExecPars& pars, bool isODprogram )
{
    if ( fullcommand_.isEmpty() )
    { errmsg_ = "Command is empty"; return false; }

    bool ret = false;
    if ( isODprogram )
    {
	if ( pars.inprogresswindow_ )
	{
	    FilePath tempfp( FilePath::getTempName("txt") );
	    fullcommand_.add( ">" ).add( tempfp.fullPath() );
	    BufferString finalcmd( __iswin__ ? "cmd /c " : "", fullcommand_ );
	    ret = doExecute( finalcmd, false, pars.waitforfinish_ );
	    if ( !ret )
		return false;
	    
	    odprogressviewer_.add( " --logfile " ).add( tempfp.fullPath() )
			    .add( " --pid " ).add( getProcessID() );
	    if ( !doExecute(odprogressviewer_,true,false) )
	    { errmsg_ = "Failed to launch progress"; return false; }
	}
	else if ( pars.logfname_.isEmpty() )
	{
	    ret = doExecute( fullcommand_, true, pars.waitforfinish_ );
	}
	else
	{
	    fullcommand_.add( ">" ).add( pars.logfname_ );
	    ret = doExecute( fullcommand_, false, pars.waitforfinish_ );
	}
    }
    else
    {
	if ( pars.inprogresswindow_ )
	{
	   ret = doExecute( fullcommand_, true, pars.waitforfinish_ );
	}
	else if ( pars.logfname_.isEmpty() )
	{
	    makeConsoleCommand();
	    ret = doExecute( fullcommand_, true, pars.waitforfinish_ );
	}
	else
	{
	    pErrMsg( "Non-OD program should not be run in hidden mode" );
	}
    }
  
    return ret;
}


void CommandLauncher::makeConsoleCommand()
{
#ifndef __win__
    return;
#else
    const BufferString batchfnm(
		FilePath(FilePath::getTempDir(),"odtmp.bat").fullPath() );
    od_ostream batchstream( batchfnm );
    batchstream << "@echo off\n" << fullcommand_.buf() << "\npause\n";
    fullcommand_ = batchfnm;
#endif
}


bool CommandLauncher::doExecute( const char* comm, bool inwindow,
				 bool waitforfinish )
{
    if ( fullcommand_.isEmpty() )
	return false;

#ifdef __USE_QPROCESS__
    QProcess qprocess;
    qprocess.startDetached( QString(fullcommand_) );
    if ( !qprocess.waitForFinished() )
        return false;
    
    return true;
#else

#ifndef __win__

    if ( *comm == '@' )
	comm++;
    if ( !*comm )
	return false;
    
    BufferString oscmd( comm );
    if ( waitforfinish )
	oscmd += "&";
    int res = system( oscmd );
    return !res;

#else

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory( &pi, sizeof(pi) );
    si.cb = sizeof(STARTUPINFO);

    if ( !inwindow )
    {
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
	si.wShowWindow = SW_HIDE;
    }

    //Start the child process.
    int res = CreateProcess( NULL,	// No module name (use command line).
			     const_cast<char*>( comm ),
			     NULL,	// Process handle not inheritable.
			     NULL,	// Thread handle not inheritable.
			     FALSE,	// Set handle inheritance to FALSE.
			     0,		// Creation flags.
			     NULL,	// Use parent's environment block.
			     NULL,	// Use parent's starting directory.
			     &si, &pi );
    
    if ( res )
    {
	if ( waitforfinish )  WaitForSingleObject( pi.hProcess, INFINITE );
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

#endif
#endif
}
