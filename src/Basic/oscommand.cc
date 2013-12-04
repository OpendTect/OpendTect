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
#include "filepath.h"
#include "staticstring.h"
#include <stdio.h>
#include <stdlib.h>

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
    if ( inpcomm.isEmpty() )
	return false;

    char* ptr = inpcomm.buf();
    mSkipBlanks( ptr );
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
    char* ptr = inp.buf();
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


#ifdef __win__

void OSCommand::mkConsoleCmd( BufferString& comm ) const
{
    const BufferString fnm(
		FilePath(FilePath::getTempDir(),"odtmp.bat").fullPath() );

    FILE *fp = fopen( fnm, "wt" );
    fprintf( fp, "@echo off\n%s\npause\n", comm.buf() );
    fclose( fp );

    comm = fnm;
}


static const char* getCmd( const char* fnm )
{
    BufferString execnm( fnm );

    char* ptr = firstOcc( execnm.buf() , ':' );

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
	    ptr = firstOcc( execnm.buf() , sep );
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
	sprintf( cmd.buf(), "%s %s %s",
			    remexec_.buf(), hname_.buf(), comm_.buf() );
}
