/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/

#include "gendefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>

#ifdef __win__
# include "winutils.h"
# include <windows.h>
# include <istream>


# ifdef __msvc__
#  define popen _popen
#  define pclose _pclose
#  define fileno(s) _fileno(s)
#  include "errh.h"
# endif
#endif

#ifndef __msvc__
# include <ext/stdio_filebuf.h>
# define mStdIOFileBuf __gnu_cxx::stdio_filebuf<char>
#endif

#include "strmprov.h"
#include "filegen.h"
#include "filepath.h"
#include "string2.h"
#include "strmoper.h"
#include "callback.h"
#include "namedobj.h"
#include "debugmasks.h"
#include "oddirs.h"
#include "errh.h"


static const char* rcsID = "$Id: strmprov.cc,v 1.79 2008-12-31 13:08:46 cvsbert Exp $";

static BufferString oscommand( 2048, false );

const char* StreamProvider::sStdIO()	{ return "Std-IO"; }
const char* StreamProvider::sStdErr()	{ return "Std-Err"; }

bool ExecOSCmd( const char* comm, bool inbg )
{
    if ( !comm || !*comm ) return false;

#ifndef __win__

    BufferString oscmd(comm);

    if ( inbg ) 
	oscmd += "&";

    int res = system( oscmd );
    return !res;

#else

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
        0,				// Creation flags. 
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

#endif
}


const char* GetExecCommand( const char* prognm, const char* filenm )
{
    static BufferString cmd;
    cmd = "@";
    cmd += mGetExecScript(); cmd += " "; cmd += prognm;

    BufferString fnm( filenm );
    replaceCharacter( fnm.buf(), ' ', (char)128 );
    FilePath fp( fnm );
    cmd += " \'"; cmd += fp.fullPath( FilePath::Unix ); cmd += "\' ";
    return cmd;
}


bool ExecuteScriptCommand( const char* prognm, const char* filenm )
{
    static BufferString cmd;
    cmd = GetExecCommand( prognm, filenm );
    StreamProvider strmprov( cmd );
#if defined( __win__ ) || defined( __mac__ )
    bool inbg = true;
#else
    bool inbg = false;
#endif
    if ( !strmprov.executeCommand(inbg) )
    {
	BufferString s( "Failed to submit command '" );
	s += strmprov.command(); s += "'";
	ErrMsg( s );
	return false;
    }

    return true;
}


StreamData& StreamData::operator =( const StreamData& sd )
{
    if ( this != &sd )
	copyFrom( sd );
    return *this;
}


void StreamData::close()
{
    if ( istrm && istrm != &std::cin )
	delete istrm;
    if ( ostrm )
    {
	ostrm->flush();
	if ( ostrm != &std::cout && ostrm != &std::cerr )
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
	, fname(fnm?fnm:sStdIO())
    	, rshcomm("rsh")
{
    if ( fname.isEmpty() ) isbad = true;
}


void StreamProvider::set( const char* devname )
{
    type_ = StreamConn::File;
    isbad = false;
    blocksize = 0;
    hostname = "";

    if ( !devname || !strcmp(devname,sStdIO()) || !strcmp(devname,sStdErr()) )
    {
	type_ = StreamConn::File;
	fname = devname ? devname : sStdIO();
	return;
    }
    else if ( !*devname )
    {
	isbad = true; fname = "";
	return;
    }

    char* ptr = (char*)devname;
    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }
    mSkipBlanks( ptr );
    fname = ptr;

    // separate hostname from filename
    ptr = strchr( fname.buf(), ':' );
    if ( ptr ) 
    {   // check for win32 drive letters.

	bool isdrive = false;
	// if only one char before the ':', it must be a drive letter.
	if ( ptr == fname.buf() + 1 
		|| ( *fname.buf()=='\"' && (ptr == fname.buf()+2) ) 
		|| ( *fname.buf()=='\'' && (ptr == fname.buf()+2) ) )
	{
	    isdrive = true;
	}
	else
	{
	    BufferString buf(fname.buf());
	    char* ptr2 = strchr( buf.buf(), ':' );
	    *ptr2='\0';

	    ptr2 = buf.buf();
	    mSkipBlanks( ptr2 );

	    // If spaces are found, it cannot come from a hostname.
	    // So, further up in the string must be a drive letter
	    isdrive = strchr( ptr2, ' ' );
	}

	if( isdrive )
	    ptr = fname.buf();
	else
	{
	    *ptr++ = '\0';
	    hostname = fname;
	}
    }
    else
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
    return type_ == StreamConn::File && hostname.isEmpty();
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
      || fname == sStdIO() || fname == sStdErr() )
	return;

    FilePath fp( fname );
    if ( fp.isAbsolute() )
	return;

    fp.insert( path );
    fname = fp.fullPath();
}

#ifndef __win__

#define mkUnLinked(fnm) fnm

#else

static const char* mkUnLinked( const char* fnm )
{
    if ( !fnm || !*fnm )
	return fnm;

    // Maybe the file itself is a link
    static BufferString ret; ret = File_linkTarget(fnm);
    if ( File_exists(ret) )
	return ret.buf();

    // Maybe there are links in the directories
    FilePath fp( fnm );
    int nrlvls = fp.nrLevels();
    for ( int idx=0; idx<nrlvls; idx++ )
    {
	BufferString dirnm = fp.dirUpTo( idx );
	const bool islink = File_isLink(dirnm);
	if ( islink )
	    dirnm = File_linkTarget( fp.dirUpTo(idx) );
	if ( !File_exists(dirnm) )
	    return fnm;

	if ( islink )
	{
	    FilePath fp2( dirnm );
	    for ( int ilvl=idx+1; ilvl<nrlvls; ilvl++ )
		fp2.add( fp.dir(ilvl) );

	    fp = fp2;
	    nrlvls = fp.nrLevels();
	}
    }

    ret = fp.fullPath();
    return ret.buf();
}

#endif


StreamData StreamProvider::makeIStream( bool binary ) const
{
    StreamData sd;
    sd.setFileName( mkUnLinked(fname) );
    if ( isbad || !*(const char*)fname )
	return sd;

    if ( fname == sStdIO() || fname == sStdErr() )
    {
	sd.istrm = &std::cin;
	return sd;
    }
    if ( type_ != StreamConn::Command && !hostname[0] )
    {
	bool doesexist = File_exists( sd.fileName() );
	if ( !doesexist )
	{
	    FilePath fp( fname );
	    BufferString fullpath = fp.fullPath( FilePath::Local, true );
	    if ( !File_exists(fullpath) )
		fullpath = fp.fullPath( FilePath::Local, false );
	    // Sometimes the filename _is_ weird, and the cleanup is wrong
	    doesexist = File_exists( fullpath );
	    if ( doesexist )
		sd.setFileName( fullpath );
	}

	sd.istrm = new std::ifstream( sd.fileName(),
		      binary ? std::ios_base::in | std::ios_base::binary 
			     : std::ios_base::in );
	if ( doesexist ? sd.istrm->bad() : !sd.istrm->good() )
	    { delete sd.istrm; sd.istrm = 0; }
	return sd;
    }

    mkOSCmd( true );

    sd.fp = popen( oscommand, "r" );
    sd.ispipe = true;

    if ( sd.fp )
    {
#ifdef __msvc__
	// TODO
	pErrMsg( "Not implemented yet" )
#else
# if __GNUC__ > 2
	//TODO change StreamData to include filebuf?
	mStdIOFileBuf* stdiofb = new mStdIOFileBuf( sd.fp, std::ios_base::in );
	sd.istrm = new std::istream( stdiofb );
# else
	sd.istrm = new std::ifstream( fileno(sd.fp) );  
# endif
#endif
    }

    return sd;
}


StreamData StreamProvider::makeOStream( bool binary ) const
{
    StreamData sd;
    sd.setFileName( mkUnLinked(fname) );
    if ( isbad ||  !*(const char*)fname )
	return sd;

    else if ( fname == sStdIO() )
    {
	sd.ostrm = &std::cout;
	return sd;
    }
    else if ( fname == sStdErr() )
    {
	sd.ostrm = &std::cerr;
	return sd;
    }

    if ( type_ != StreamConn::Command && !hostname[0] )
    {
	sd.ostrm = new std::ofstream( sd.fileName(),
			  binary ? std::ios_base::out | std::ios_base::binary 
				 : std::ios_base::out );
	if ( sd.ostrm->bad() )
	    { delete sd.ostrm; sd.ostrm = 0; }
	return sd;
    }

    mkOSCmd( false );

    sd.fp = popen( oscommand, "w" );
    sd.ispipe = true;

    if ( sd.fp )
    {
#ifdef __msvc__
	// TODO
	pErrMsg( "Not implemented yet" )
#else
# if __GNUC__ > 2
	mStdIOFileBuf* stdiofb = new mStdIOFileBuf( sd.fp, std::ios_base::out );
	sd.ostrm = new std::ostream( stdiofb );
# else
	sd.ostrm = new std::ofstream( fileno(sd.fp) );
# endif
#endif
    }

    return sd;
}


bool StreamProvider::executeCommand( bool inbg ) const
{
    mkOSCmd( true );
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
	interp = "tcsh.exe";
    else if ( strstr(execnm,".sh") || strstr(execnm,".SH") ||
	      strstr(execnm,".bash") || strstr(execnm,".BASH") )
	interp = "sh.exe";
    else if ( strstr(execnm,".awk") || strstr(execnm,".AWK") )
	interp = "awk.exe";
    else if ( strstr(execnm,".sed") || strstr(execnm,".SED") )
	interp = "sed.exe";
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
	    interp = "tcsh.exe";
	else if ( strstr(line,"awk") )
	    interp = "awk.exe";
	else if ( strstr(line,"sh") )
	    interp = "sh.exe";
    }
    
    if ( interp )
    {
	static BufferString fullexec;

	fullexec = "\"";

	FilePath interpfp;

	if ( getCygDir() )
	{
	    interpfp.set( getCygDir() );
	    interpfp.add("bin").add(interp);
	}

	if ( !File_exists( interpfp.fullPath() ) )
	{
	    interpfp.set( GetSoftwareDir() );
	    interpfp.add("bin").add("win").add("sys")
		    .add(interp).fullPath();
	}

	fullexec += interpfp.fullPath();

	fullexec += "\" '";
	FilePath execfp( execnm );
	fullexec += execfp.fullPath( FilePath::Unix );
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


#ifdef __win__
# define mGetCmd(fname) getCmd(fname)
#else
# define mGetCmd(fname) (const char*)(fname)
#endif

void StreamProvider::mkOSCmd( bool forread ) const
{
    if ( !hostname[0] )
	oscommand = mGetCmd(fname);
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
	    sprintf( oscommand.buf(), "%s %s %s", (const char*)rshcomm,
		     (const char*)hostname, mGetCmd(fname) );
	break;
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




    if ( DBG::isOn(DBG_IO) )
    {
	BufferString msg( "Aboute to execute : '" );
	msg += oscommand;
	msg += "'";
	DBG::message( msg );
    }
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
	return fname == sStdIO() || fname == sStdErr() ? true
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
	return fname == sStdIO() || fname == sStdErr() ? false :
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
	return fname == sStdIO() || fname == sStdErr() ? false :
	       File_makeWritable( (const char*)fname, mFile_NotRecursive, !yn );

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
	return fname == sStdIO() || fname == sStdErr() ? false :
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
    NamedObject nobj( msg );
    CBCapsule<const char*> caps( ((const char*)msg), &nobj );
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
	    rv = fname == sStdIO() || fname == sStdErr() ? true :
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
