/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/

#include "Pmacros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>

#ifdef __win__
# include <windows.h>
# include <istream>
# ifdef __msvc__
#  define popen _popen
#  define pclose _pclose
#  define fileno(s) _fileno(s)
# include "errh.h"
# endif
#endif

#if __GNUC__ > 2
# include <ext/stdio_filebuf.h>
# define mStdIOFileBuf __gnu_cxx::stdio_filebuf<char>
#endif

#include "strmprov.h"
#include "filegen.h"
#include "string2.h"
#include "binidsel.h"
#include "strmoper.h"
#include "callback.h"
#include "uidobj.h"


static const char* rcsID = "$Id: strmprov.cc,v 1.48 2003-12-10 09:56:55 arend Exp $";

static FixedString<1024> oscommand;

const char* StreamProvider::sStdIO = "Std-IO";
const char* StreamProvider::sStdErr = "Std-Err";

bool ExecOSCmd( const char* comm, bool inbg )
{
    if ( !comm || !*comm ) return false;

#ifdef __win__
/*
    if ( !inbg )
    {
	int res = system(comm);
	return !res;
    }
*/
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Start the child process. 
    int res = CreateProcess( NULL,	// No module name (use command line). 
        const_cast<char*>( comm ),	// Command line. 
        NULL,				// Process handle not inheritable. 
        NULL,				// Thread handle not inheritable. 
        FALSE,				// Set handle inheritance to FALSE. 
        inbg ? DETACHED_PROCESS : 0,	// Creation flags. 
        NULL,				// Use parent's environment block. 
        NULL,       			// Use parent's starting directory. 
        &si,				// Pointer to STARTUPINFO structure.
        &pi );             // Pointer to PROCESS_INFORMATION structure.


    if ( res )
    {
	if ( !inbg )  WaitForSingleObject( pi.hProcess, INFINITE );

	// Close process and thread handles. 
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

#else

    int res = system(comm);
    return !res;

#endif
}


StreamData& StreamData::operator =( const StreamData& sd )
{
    if ( this != &sd )
	copyFrom( sd );
    return *this;
}


void StreamData::close()
{
    if ( istrm && istrm != &cin )
	delete istrm;
    if ( ostrm )
    {
	ostrm->flush();
	if ( ostrm != &cout && ostrm != &cerr )
	    delete ostrm;
    }

    if ( fp && fp != stdin && fp != stdout && fp != stderr )
	{ if ( ispipe ) pclose(fp); else fclose(fp); }

    initStrms();
}


bool StreamData::usable() const
{
    return ( istrm || ostrm ) && ( !ispipe || fp );
}


void StreamData::copyFrom( const StreamData& sd )
{
    istrm = sd.istrm; ostrm = sd.ostrm;
    fp = sd.fp; ispipe = sd.ispipe;
    setFileName( sd.fnm );
}


void StreamData::transferTo( StreamData& sd )
{
    sd.copyFrom( *this );
    initStrms();
}


void StreamData::setFileName( const char* f )
{
    delete [] fnm;
    fnm = f ? new char [strlen(f)+1] : 0;
    if ( fnm ) strcpy( fnm, f );
}


StreamProvider::StreamProvider( const char* devname )
    	: rshcomm("rsh")
{
    set( devname );
}


StreamProvider::StreamProvider( const char* hostnm, const char* fnm,
				StreamConn::Type typ )
	: isbad(false)
	, type_(typ)
	, hostname(hostnm)
	, fname(fnm?fnm:sStdIO)
    	, rshcomm("rsh")
{
    if ( fname == "" ) isbad = true;
}


void StreamProvider::set( const char* devname )
{
    type_ = StreamConn::File;
    isbad = false;
    blocksize = 0;
    hostname = "";

    if ( !devname || !strcmp(devname,sStdIO) || !strcmp(devname,sStdErr) )
    {
	type_ = StreamConn::File;
	fname = devname ? devname : sStdIO;
	return;
    }
    else if ( !*devname )
    {
	isbad = true; fname = "";
	return;
    }

    char* ptr = (char*)devname;
    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }
    fname = ptr;

#ifndef __msvc__

    // separate hostname from filename
    ptr = strchr( fname.buf(), ':' );
    if ( ptr )
    {
#ifdef __win__ // non-msvc compiler, like MinGw
	// if only one char before the ':', it must be a drive letter.
	if ( ptr == fname.buf() + 1 
		|| ( *fname.buf()=='\"' && (ptr == fname.buf()+2) ) 
		|| ( *fname.buf()=='\'' && (ptr == fname.buf()+2) ) )
	    ptr = fname.buf();
	else
#endif
	{
	    *ptr++ = '\0';
	    hostname = fname;
	}
    }
    else
#endif
	ptr = fname.buf();

    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }

    char* ptrname = fname.buf();
    while ( *ptr ) *ptrname++ = *ptr++;
    *ptrname = '\0';

    if ( type_ != StreamConn::Command && matchString( "/dev/", fname ) )
	type_ = StreamConn::Device;
}


bool StreamProvider::isNormalFile() const
{
    return type_ == StreamConn::File && hostname == "";
}


bool StreamProvider::rewind() const
{
    if ( isbad ) return false;
    else if ( type_ != StreamConn::Device ) return true;

    if ( hostname[0] )
	sprintf( oscommand.buf(), "%s %s \"mt -f %s rewind\"",
		 (const char*)rshcomm,
		 (const char*)hostname, (const char*)fname );
    else
	sprintf( oscommand.buf(), "mt -f %s rewind", (const char*)fname );
    return ExecOSCmd(oscommand);
}


bool StreamProvider::offline() const
{
    if ( isbad ) return false;
    else if ( type_ != StreamConn::Device ) return true;

    if ( hostname[0] )
	sprintf( oscommand.buf(), "%s %s \"mt -f %s offline\"",
				  (const char*)rshcomm,
				  (const char*)hostname, (const char*)fname );
    else
	sprintf( oscommand.buf(), "mt -f %s offline", (const char*)fname );
    return ExecOSCmd(oscommand);
}


bool StreamProvider::skipFiles( int nr ) const
{
    if ( isbad ) return false;
    if ( type_ != StreamConn::Device ) return false;

    if ( hostname[0] )
	sprintf( oscommand.buf(), "%s %s \"mt -f %s fsf %d\"",
				(const char*)rshcomm,
				(const char*)hostname, (const char*)fname, nr );
    else
	sprintf( oscommand.buf(), "mt -f %s fsf %d", (const char*)fname, nr );
    return ExecOSCmd(oscommand);
}


const char* StreamProvider::fullName() const
{
    oscommand = "";
    if ( type_ == StreamConn::Command )
	oscommand += "@";
    if ( hostname[0] ) 
    {
	oscommand += hostname;
	oscommand += ":";
    }
    oscommand += (const char*)fname;

    return oscommand;
}


void StreamProvider::addPathIfNecessary( const char* path )
{
    if ( isbad ) return;
    if ( type_ != StreamConn::File
      || !path || ! *path
      || fname == sStdIO || fname == sStdErr
      || File_isAbsPath(fname) )
	return;

    BufferString pth( path );
    fname = File_getFullPath( pth, fname );
}


StreamData StreamProvider::makeIStream( bool inbg ) const
{
    StreamData sd; sd.setFileName( fname );
    if ( isbad || !*(const char*)fname )
	return sd;
    if ( fname == sStdIO || fname == sStdErr )
    {
	sd.istrm = &cin;
	return sd;
    }
    if ( type_ != StreamConn::Command && !hostname[0] )
    {
#ifdef __win__
	if ( File_isLink( fname ) )
	    sd.istrm = new ifstream( File_linkTarget(fname),
					    ios_base::in | ios_base::binary );
	else
#endif
	if ( File_exists(fname) )
	    sd.istrm = new ifstream( fname, ios_base::in | ios_base::binary );
	return sd;
    }

#ifdef __msvc__

    pErrMsg("Pipes are not supported on windows platform");

/*
    MSVC chokes on the following lines:
	sd.istrm = new ifstream( fileno(sd.fp) );
	sd.ostrm = new ofstream( fileno(sd.fp) );

    This means you can not open a fstream with just a file pointer. You would
    need a 'filedesc' for that. You can't make that from a file pointer, 
    however.
    Fortunately, this behavior is only required for more advanced useage, so
    we leave it out for now.

    A possible solution might be using named pipes. 
    See http://www.codeguru.com/console/dualmode.html

*/

#else

    mkOSCmd( true, inbg );

    sd.fp = popen( oscommand, "r" );
    sd.ispipe = true;

    if ( sd.fp )
    {
# if __GNUC__ > 2
	//TODO change StreamData to include filebuf?
	mStdIOFileBuf* stdiofb = new mStdIOFileBuf( sd.fp, ios_base::in );
	sd.istrm = new istream( stdiofb );
# else
	sd.istrm = new ifstream( fileno(sd.fp) );
# endif
#endif
    }

    return sd;
}


StreamData StreamProvider::makeOStream( bool inbg ) const
{
    StreamData sd; sd.setFileName( fname );
    if ( isbad ||  !*(const char*)fname )
	return sd;
    else if ( fname == sStdIO )
    {
	sd.ostrm = &cout;
	return sd;
    }
    else if ( fname == sStdErr )
    {
	sd.ostrm = &cerr;
	return sd;
    }
    if ( type_ != StreamConn::Command && !hostname[0] )
    {
	sd.ostrm = new ofstream( fname, ios_base::out|ios_base::binary );
	return sd;
    }

#ifdef __msvc__

    pErrMsg("Pipes are not supported on windows platform");

#else

    mkOSCmd( false, inbg );

    sd.fp = popen( oscommand, "w" );
    sd.ispipe = true;

    if ( sd.fp )
    {
# if __GNUC__ > 2
	mStdIOFileBuf* stdiofb = new mStdIOFileBuf( sd.fp, ios_base::out );
	sd.ostrm = new ostream( stdiofb );
# else
	sd.ostrm = new ofstream( fileno(sd.fp) );
# endif
#endif
    }

    return sd;
}


bool StreamProvider::executeCommand( bool inbg ) const
{
    mkOSCmd( true, inbg );
    return ExecOSCmd( oscommand, inbg );
}


#ifdef __win__
static const char* getCmd( const char* fnm )
{
    BufferString execnm( fnm );

    char* ptr = strchr( execnm.buf() , ':' );

    if ( !ptr )
	return fnm;

    char* args=0;

    // if only one char before the ':', it must be a drive letter.
    if ( ptr == execnm.buf() + 1 )
    {
	ptr = strchr( ptr , ' ' );
	if ( ptr ) { *ptr = '\0'; args = ptr+1; }
    }
    else if ( ptr == execnm.buf()+2) 
    {
	char sep = *execnm.buf();
	if ( sep == '\"' || sep == '\'' )
	{
	    execnm=fnm+1;
	    ptr = strchr( execnm.buf() , sep );
	    if ( ptr ) { *ptr = '\0'; args = ptr+1; }
	}
    }
    else
	return fnm;

    if ( strstr(execnm,".exe") || strstr(execnm,".EXE") 
       || strstr(execnm,".bat") || strstr(execnm,".BAT")
       || strstr(execnm,".com") || strstr(execnm,".COM") )
	return fnm;

    const char* interp = 0;

    if ( strstr(execnm,".csh") || strstr(execnm,".CSH") )
	interp = "\\bin\\win\\sys\\csh.exe";
    else if ( strstr(execnm,".sh") || strstr(execnm,".SH") ||
	      strstr(execnm,".bash") || strstr(execnm,".BASH") )
	interp = "\\bin\\win\\sys\\sh.exe";
    else if ( strstr(execnm,".awk") || strstr(execnm,".AWK") )
	interp = "\\bin\\win\\sys\\awk.exe";
    else if ( strstr(execnm,".sed") || strstr(execnm,".SED") )
	interp = "\\bin\\win\\sys\\sed.exe";
    else if ( File_exists( execnm ) )
    {
	// We have a full path to a file with no known extension,
	// but it exists. Let's peek inside.

	StreamData sd = StreamProvider( execnm ).makeIStream();
	if ( !sd.usable() )
	    return fnm;

	BufferString line;
	sd.istrm->getline( line.buf(), 40 ); sd.close();

	if ( !strstr(line,"#!") && !strstr(line,"# !") )
	    return fnm;

	if ( strstr(line,"csh") )
	    interp = "\\bin\\win\\sys\\csh.exe";
	else if ( strstr(line,"awk") )
	    interp = "\\bin\\win\\sys\\awk.exe";
	else if ( strstr(line,"sh") )
	    interp = "\\bin\\win\\sys\\sh.exe";
    }
    
    if ( interp )
    {
	static BufferString fullexec;

	fullexec = "\"";
	fullexec += File_getFullPath( GetSoftwareDir(), interp );
	fullexec += "\" '";
	fullexec += execnm;
	fullexec += "'";

	if ( args && *args )
	{
	    fullexec += " ";
	    fullexec += args;
	}

	return fullexec;
    }

    return fnm;
}
#endif

void StreamProvider::mkOSCmd( bool forread, bool inbg ) const
{
    if ( !hostname[0] )
#ifdef __win__
	oscommand = getCmd(fname);
#else
	oscommand = (const char*)fname;
#endif
    else
    {
	switch ( type_ )
	{
	case StreamConn::Device:
	    if ( forread )
	    {
		if ( blocksize )
		    sprintf( oscommand.buf(), "%s %s dd if=%s ibs=%ld",
				(const char*)rshcomm, (const char*)hostname,
				(const char*)fname, blocksize );
		else
		    sprintf( oscommand.buf(), "%s %s dd if=%s",
				(const char*)rshcomm,
				(const char*)hostname, (const char*)fname );
	    }
	    else
	    {
		if ( blocksize )
		    sprintf( oscommand.buf(), "%s %s dd of=%s obs=%ld",
				  (const char*)rshcomm, (const char*)hostname,
				  (const char*)fname, blocksize );
		else
		    sprintf( oscommand.buf(), "%s %s dd of=%s",
				  (const char*)rshcomm,
				  (const char*)hostname, (const char*)fname );
	    }
	break;
	case StreamConn::Command:
#ifdef __win__
	    sprintf( oscommand.buf(), "%s %s %s",
				      (const char*)rshcomm,
				      (const char*)hostname,
				      getCmd(fname) );
#else
	    sprintf( oscommand.buf(), "%s %s %s",
				      (const char*)rshcomm,
				      (const char*)hostname,
				      (const char*)fname );
	break;
#endif
	case StreamConn::File:
	    if ( forread )
		sprintf( oscommand.buf(), "%s %s cat %s",
				(const char*)rshcomm,
				(const char*)hostname, (const char*)fname );
	    else
		sprintf( oscommand.buf(), "%s %s tee %s > /dev/null",
				(const char*)rshcomm,
				(const char*)hostname, (const char*)fname );
	break;
	}
    }

#ifndef __win__

    if ( inbg ) 
	oscommand += "&";

#endif
}

#define mRemoteTest(act) \
    FILE* fp = popen( oscommand, "r" ); \
    char c; fscanf( fp, "%c", &c ); \
    pclose( fp ); \
    act c == '1'


bool StreamProvider::exists( int fr ) const
{
    if ( isbad ) return false;

    if ( type_ == StreamConn::Command )
	return fr;

    if ( !hostname[0] )
	return fname == sStdIO || fname == sStdErr ? true
	     : File_exists( (const char*)fname );

    sprintf( oscommand.buf(), "%s %s 'test -%c %s && echo 1'",
		  (const char*)rshcomm, (const char*)hostname,
		  fr ? 'r' : 'w', (const char*)fname );
    mRemoteTest(return);
}


bool StreamProvider::remove( bool recursive ) const
{
    if ( isbad || type_ != StreamConn::File ) return false;

    if ( !hostname[0] )
	return fname == sStdIO || fname == sStdErr ? false :
		File_remove( (const char*)fname, recursive );

    sprintf( oscommand.buf(), "%s %s '/bin/rm -%s %s && echo 1'",
	      (const char*)rshcomm,
	      (const char*)hostname, recursive ? "r" : "",
	      (const char*)fname );

    mRemoteTest(return);
}


bool StreamProvider::setReadOnly( bool yn ) const
{
    if ( isbad || type_ != StreamConn::File ) return false;

    if ( !hostname[0] )
	return fname == sStdIO || fname == sStdErr ? false :
		File_makeWritable( (const char*)fname, NO, !yn );

    sprintf( oscommand.buf(), "%s %s 'chmod %s %s && echo 1'",
	      (const char*)rshcomm,
	      (const char*)hostname, yn ? "a-w" : "ug+w",
	      (const char*)fname );

    mRemoteTest(return);
}


bool StreamProvider::isReadOnly() const
{
    if ( isbad || type_ != StreamConn::File ) return true;

    if ( !hostname[0] )
	return fname == sStdIO || fname == sStdErr ? false :
		!File_isWritable( (const char*)fname );

    sprintf( oscommand.buf(), "%s %s 'test -w %s && echo 1'",
	      (const char*)rshcomm,
	      (const char*)hostname,
	      (const char*)fname );

    mRemoteTest(return !);
}


static void mkRelocMsg( const char* oldnm, const char* newnm,BufferString& msg )
{
    msg = "Relocating <";
    while ( *oldnm && *newnm && *oldnm == *newnm )
	{ oldnm++; newnm++; }
    msg += oldnm; msg += "> to <"; msg += newnm; msg += "> ...";
}


void StreamProvider::sendCBMsg( const CallBack* cb, const char* msg )
{
    UserIDObject uidobj( msg );
    CBCapsule<const char*> caps( ((const char*)msg), &uidobj );
    CallBack(*cb).doCall( &caps );
}


bool StreamProvider::rename( const char* newnm, const CallBack* cb )
{
    bool rv = false;
    const bool issane = newnm && *newnm && !isbad && type_ == StreamConn::File;

    if ( cb && cb->willCall() )
    {
	BufferString msg;
	if ( issane )
	    mkRelocMsg( fname, newnm, msg );
	else if ( type_ != StreamConn::File )
	    msg = "Cannot rename commands or devices";
	else
	{
	    if ( isbad )
		msg = "Cannot rename invalid filename";
	    else
		msg = "No filename provided for rename";
	}
	sendCBMsg( cb, msg );
    }

    if ( issane )
    {
	if ( !hostname[0] )
	    rv = fname == sStdIO || fname == sStdErr ? true :
		    File_rename( (const char*)fname, newnm );
	else
	{
	    sprintf( oscommand.buf(), "%s %s '/bin/mv -f %s %s && echo 1'",
		      (const char*)rshcomm, (const char*)hostname,
		      (const char*)fname, newnm );
	    mRemoteTest(rv =);
	}
    }

    if ( rv )
	set( newnm );
    return rv;
}
