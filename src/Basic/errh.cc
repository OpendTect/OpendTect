/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "errh.h"
#include "strmprov.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "envvars.h"
#include <iostream>
#include <fstream>

#ifdef HAS_BREAKPAD
#include "client\windows\handler\exception_handler.h"
#include <QString>
#endif

Export_Basic const char* logMsgFileName();
Export_Basic std::ostream& logMsgStrm();
bool ErrMsgClass::printProgrammerErrs =
# ifdef __debug__
    true;
# else
    false;
# endif
static BufferString logmsgfnm;
Export_Basic int gLogFilesRedirectCode = -1;
// Not set. 0 = stderr, 1 = log file

Export_Basic const char* logMsgFileName()
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
	
  
Export_Basic std::ostream& logMsgStrm()
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
namespace google_breakpad{ class ExceptionHandler; }
struct CrashDumper
{
    CrashDumper( const char* path, const char* sendappl )
    : path_( path )
    , sendappl_( sendappl )
    , handler_(0)
    {
	init();
    }
    
    void		sendDump(const char* filename);
    
    void		init();
    
    BufferString	sendappl_;
    BufferString	path_;
    
    google_breakpad::ExceptionHandler*	handler_;
};


CrashDumper* theinst mUnusedVar = 0;

#if defined ( __msvc__ )  && defined ( HAS_BREAKPAD )

static bool MinidumpCB(const wchar_t* dump_path, const wchar_t *id,
                     void *context, EXCEPTION_POINTERS *exinfo,
                     MDRawAssertionInfo *assertion,
                     bool succeeded)
{ 
    QString path = QString::fromWCharArray( dump_path );
    QString mndmpid = QString::fromWCharArray( id );
    BufferString dmppath ( path.toAscii().constData() );
    BufferString dmpid ( mndmpid.toAscii().constData(), ".dmp" );
    FilePath dmpfp( dmppath, dmpid );
    if ( theinst )
	theinst->sendDump( dmpfp.fullPath() );
    return succeeded;
}

void CrashDumper::init()
{
    if ( !handler_ )
    {
	BufferString path = FilePath::getTempDir();
	QString dmppath( path );
	int dmplen = dmppath.length();
	int cordmplen = dmplen - 2;
	wchar_t* wpath = new wchar_t[ cordmplen ];
	dmppath.toWCharArray( wpath );
	handler_ = new google_breakpad::ExceptionHandler(
			wpath,
			NULL, MinidumpCB, NULL,
			google_breakpad::ExceptionHandler::HANDLER_ALL );
    }
}
#else
void CrashDumper::init()
{
    
}

#endif


void CrashDumper::sendDump( const char* filename )
{
    if ( sendappl_.isEmpty() || !File::exists(filename) )
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

