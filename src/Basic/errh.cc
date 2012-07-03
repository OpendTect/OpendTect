/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/

static const char* rcsID mUnusedVar = "$Id: errh.cc,v 1.6 2012-07-03 15:39:50 cvsnanne Exp $";

#include "errh.h"
#include "strmprov.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "envvars.h"
#include <iostream>
#include <fstream>

mGlobal const char* logMsgFileName();
mGlobal std::ostream& logMsgStrm();
bool ErrMsgClass::printProgrammerErrs =
# ifdef __debug__
    true;
# else
    false;
# endif
static BufferString logmsgfnm;
mBasicGlobal int gLogFilesRedirectCode = -1;
// Not set. 0 = stderr, 1 = log file


mBasicGlobal const char* logMsgFileName()
{
    return logmsgfnm.buf();
}


#ifndef __win__
	#define mErrRet(s){ strm = &std::cerr;  \
	*strm << "Cannot open file for log messages:\n\t" << s << std::endl; \
	return *strm; } 
#else 
	#define mErrRet(s){ strm = new std::ofstream( "nul" ); \
	*strm << "Cannot open file for log messages:\n\t" << s << std::endl; \
	return *strm; }
#endif
	
  
mBasicGlobal std::ostream& logMsgStrm()
{
    if ( gLogFilesRedirectCode < 1 )
	return std::cerr;

    static std::ostream* strm = 0;
    if ( strm ) return *strm;

    if ( GetEnvVarYN("OD_LOG_STDERR") )
	{ strm = &std::cerr; return *strm; }

    const char* basedd = GetBaseDataDir();
    if ( !File::isDirectory(basedd) )
	mErrRet( "Directory for data storage is invalid" )

    FilePath fp( basedd, "LogFiles" );
    const BufferString dirnm = fp.fullPath();
    if ( !File::exists(dirnm) )
	File::createDir( dirnm );
    if ( !File::isDirectory(dirnm) )
	mErrRet( "Cannot create proper directory for log file" )

    const FilePath pfp( GetPersonalDir() );
    BufferString fnm( pfp.fileName() );
    const char* odusr = GetSoftwareUser();
    if ( odusr && *odusr )
	{ fnm += "_"; fnm += odusr; }
    BufferString datestr = Time::getDateTimeString();
    replaceCharacter( datestr.buf(), ' ', '-' );
    replaceCharacter( datestr.buf(), ':', '.' );
    fnm += "_"; fnm += datestr.buf();
    fnm += ".txt";

    fp.add( fnm );
    logmsgfnm = fp.fullPath();
    StreamData sd = StreamProvider( logmsgfnm ).makeOStream( false );
    if ( !sd.usable() )
    {
	BufferString msg( "Cannot create log file '" );
	msg += logmsgfnm; msg += "'";
	logmsgfnm = "";
	mErrRet( msg );
    }

    strm = sd.ostrm;
    return *strm;
}


void UsrMsg( const char* msg, MsgClass::Type t )
{
    if ( !MsgClass::theCB().willCall() )
	logMsgStrm() << msg << std::endl;
    else
    {
	MsgClass obj( msg, t );
	MsgClass::theCB().doCall( &obj );
    }
}


void ErrMsg( const char* msg, bool progr )
{
    if ( !ErrMsgClass::printProgrammerErrs && progr ) return;

    if ( !MsgClass::theCB().willCall() )
    {
	if ( progr )
	    std::cerr << "(PE) " << msg << std::endl;
	else if ( msg && *msg )
	{
	    const char* start = *msg == '[' ? "" : "Err: ";
	    logMsgStrm() << start << msg << std::endl;
	}
    }
    else
    {
	ErrMsgClass obj( msg, progr );
	MsgClass::theCB().doCall( &obj );
    }
}


CallBack& MsgClass::theCB( const CallBack* cb )
{
    static CallBack thecb;
    if ( cb ) thecb = *cb;
    return thecb;
}


const char* MsgClass::nameOf( MsgClass::Type typ )
{
    static const char* strs[] =
    	{ "Information", "Message", "Warning", "Error", "PE", 0 };
    return strs[ (int)typ ];
}


//Crashdumper stuff
struct CrashDumper
{
    CrashDumper( const char* path, const char* sendappl )
    : path_( path )
    , sendappl_( sendappl )
    {
	init();
    }
    
    void		sendDump(const char* filename);
    
    void		init();
    
    BufferString	sendappl_;
    BufferString	path_;
    
#ifdef __msvc__
//    ExceptionHandler*	handler_;
#endif
};


CrashDumper* theinst mUnusedVar = 0;


#ifdef __msvc__
void CrashDumper::init()
{
    
}
#else
void CrashDumper::init()
{
    
}

#endif


void CrashDumper::sendDump( const char* filename )
{
    if ( sendappl_.isEmpty() )
	return;
    
    if ( !File::exists( sendappl_ ) || !File::exists( filename ) )
	return;
    
    const BufferString cmd( sendappl_, " ", filename );
    StreamProvider(cmd).executeCommand( true, false );
}


bool initCrashDumper( const char* dumpdir, const char* sendappl )
{
    if ( !theinst )
	theinst = new CrashDumper( dumpdir, sendappl );
    else
	pFreeFnErrMsg("initCrashDumper", "CrashDumper installed before");
    
    return true;
}


FixedString sSenderAppl()
{ return FixedString("od_ReportIssue" ); }


FixedString sUiSenderAppl()
{ return FixedString( "od_uiReportIssue" ); } 


