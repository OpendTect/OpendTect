/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "errh.h"

#include "debug.h"
#include "strmprov.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "envvars.h"
#include "od_ostream.h"
#include <iostream>

#ifdef HAS_BREAKPAD
#include "client\windows\handler\exception_handler.h"
#include <QString>
#endif

namespace OD { Export_Basic od_ostream& logMsgStrm(); }
bool ErrMsgClass::printProgrammerErrs =
# ifdef __debug__
    true;
# else
    false;
# endif
Export_Basic int gLogFilesRedirectCode = -1;
// -1 == Not set. 0 = stderr, 1 = log file

static bool crashonprogerror = false;


namespace OD {

Export_Basic od_ostream& logMsgStrm()
{
    static od_ostream* strm = 0;
    if ( strm )
	return *strm;

    BufferString errmsg;
    if ( gLogFilesRedirectCode > 0 && !GetEnvVarYN("OD_LOG_STDERR") )
    {
	const char* basedd = GetBaseDataDir();
	if ( !File::isDirectory(basedd) )
	    errmsg = "Directory for data storage is invalid";
	else
	{
	    FilePath fp( basedd, "LogFiles" );
	    const BufferString dirnm = fp.fullPath();
	    if ( !File::exists(dirnm) )
		File::createDir( dirnm );
	    if ( !File::isDirectory(dirnm) )
		errmsg = "Cannot create proper directory for log file";
	    else
	    {
		const FilePath pfp( GetPersonalDir() );
		BufferString fnm( pfp.fileName() );
		const char* odusr = GetSoftwareUser();
		if ( odusr && *odusr )
		    { fnm += "_"; fnm += odusr; }
		BufferString datestr = Time::getDateTimeString();
		replaceString( datestr.buf(), ", ", "-" );
		replaceCharacter( datestr.buf(), ':', '.' );
		replaceCharacter( datestr.buf(), ' ', '_' );
		fnm += "_"; fnm += datestr.buf();
		fnm += ".txt";

		fp.add( fnm );
		BufferString logmsgfnm = fp.fullPath();
		strm = new od_ostream( logmsgfnm );
		if ( !strm->isOK() )
		{
		    errmsg.set( "Cannot create log file '" )
			  .add( logmsgfnm ).add( "'" );
		    delete strm; strm = 0;
		}
	    }
	}
    }

    if ( !strm )
    {
#	ifdef __win__
	    strm = &od_ostream::nullStream();
#	else
	    strm = new od_ostream( std::cerr );
#	endif
	if ( !errmsg.isEmpty() )
	    *strm << errmsg;
    }

    return *strm;
}

Export_Basic void programmerErrMsg( const char* inpmsg, const char* cname,
				    const char* fname, int linenr )
{
    BufferString msg( cname, " | " );
    msg.add( fname ).add( ":" ).add( linenr )
	.add( " >> " ).add( inpmsg ).add( " <<" );
    ErrMsg( msg.buf(), true );
}


}


void UsrMsg( const char* msg, MsgClass::Type t )
{
    if ( !MsgClass::theCB().willCall() )
	OD::logMsgStrm() << msg << od_endl;
    else
    {
	MsgClass obj( msg, t );
	MsgClass::theCB().doCall( &obj );
    }
}


void SetCrashOnProgrammerError( int yn )
{
    crashonprogerror = yn;
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
	    OD::logMsgStrm() << start  << msg << od_endl;
	}
    }
    else
    {
	ErrMsgClass obj( msg, progr );
	MsgClass::theCB().doCall( &obj );
    }
    
    if ( progr && crashonprogerror )
    {
	DBG::forceCrash( false );
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

#ifdef mUseCrashDumper

using namespace System;

//Crashdumper stuff
CrashDumper::CrashDumper()
    : sendappl_( sSenderAppl() )
    , handler_(0)
{
    init();
}


CrashDumper* CrashDumper::theinst_ = 0;


static bool MinidumpCB(const wchar_t* dump_path, const wchar_t *id,
                     void *context, EXCEPTION_POINTERS *exinfo,
                     MDRawAssertionInfo *assertion,
                     bool succeeded)
{ 
    const QString path = QString::fromWCharArray( dump_path );
    const QString mndmpid = QString::fromWCharArray( id );
    const BufferString dmppath ( path.toAscii().constData() );
    const BufferString dmpid ( mndmpid.toAscii().constData(), ".dmp" );
    const FilePath dmpfp( dmppath, dmpid );
    System::CrashDumper::getInstance().sendDump( dmpfp.fullPath() );
    return succeeded;
}


void CrashDumper::init()
{
    if ( !handler_ )
    {
	const QString dmppath = FilePath::getTempDir();
	const std::wstring wpath = dmppath.toStdWString();
	handler_ = new google_breakpad::ExceptionHandler(
			wpath,
			NULL, MinidumpCB, NULL,
			google_breakpad::ExceptionHandler::HANDLER_ALL );
    }
}


void CrashDumper::sendDump( const char* filename )
{
    if ( sendappl_.isEmpty() || !File::exists(filename) )
	return;
    
    const BufferString cmd( sendappl_, " --binary ", filename );
    StreamProvider(cmd).executeCommand( true, true );
}


CrashDumper& CrashDumper::getInstance()
{
    if ( !theinst_ )
    {
	theinst_ = new CrashDumper;
    }
    
    return *theinst_;
}


FixedString CrashDumper::sSenderAppl()
{ return FixedString("od_ReportIssue" ); }


FixedString CrashDumper::sUiSenderAppl()
{ return FixedString( "od_uiReportIssue" ); }


#endif
