/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "issuereporter.h"

#include "commandlineparser.h"
#include "iopar.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odinst.h"
#include "odnetworkaccess.h"
#include "od_istream.h"
#include "odplatform.h"
#include "odsysmem.h"
#include "separstr.h"
#include "thread.h"
#include "uistrings.h"
#include "winutils.h"
#include "systeminfo.h"

#include <fstream>

namespace System
{
    static const char* sKeyReportHost()
    { return "http://backend.opendtect.org"; }

    static const char* sKeyReportScript()
    { return "/backendscripts/crashreport.php"; }
} // namespace System

System::IssueReporter::IssueReporter( const char* host, const char* path )
    : host_(host)
    , path_(path)
{
    if ( host_.isEmpty() )
	host_ = sKeyReportHost();

    if ( path_.isEmpty() )
	path_ = sKeyReportScript();
}


System::IssueReporter::~IssueReporter()
{
}


#define mTestMatch( matchstr ) \
    if ( line.matches( "*" matchstr "*" ) ) \
	continue;

bool System::IssueReporter::readReport( const char* filename )
{
    if ( !File::exists( filename ) )
    {
	errmsg_ = uiStrings::phrDoesntExist(toUiString(filename));
	return false;
    }

#define mStreamError( op ) \
{ \
    errmsg_ = errmsg.arg( op ).arg( filename ).arg( fstream.errMsg() ); \
    return false; \
}

    uiString errmsg = tr( "Cannot %1 %2. Reason: %3");

    od_istream fstream( filename );
    if ( fstream.isBad() )
	mStreamError( tr("open") );

    report_.add( "User: ").add( GetSoftwareUser() ).add( "\n\n" );

    BufferString unfilteredreport;

    if ( !fstream.getAll( unfilteredreport ) )
	mStreamError( tr("read") );

    report_.add( unfilteredreport.buf() );

    return true;
}


void System::IssueReporter::fillBasicReport( const char* /* filename */ )
{
    return fillBasicReport();
}


void System::IssueReporter::fillBasicReport()
{
    report_.setEmpty();
    StringPairSet infoset;
    infoset.add( "OpendTect version", ODInst::getPkgVersion("base") );
    infoset.add( "Platform", OD::Platform::local().longName() );
    infoset.add( "Operating System", System::productName() );

#ifdef __win__
    infoset.add( "Windows OS version", getFullWinVersion() );
#endif

    infoset.add( "Nr. of processors", Threads::getNrProcessors() );

    StringPairSet meminfo;
    OD::dumpMemInfo( meminfo );
    infoset.add( meminfo );
    infoset.dumpPretty( report_ );
}


bool System::IssueReporter::setDumpFileName( const char* filename )
{
    if ( !File::exists( filename ) )
    {
	errmsg_ = uiStrings::phrDoesntExist(toUiString(filename));
	return false;
    }

    crashreportpath_ = filename;
    return true;
}


bool System::IssueReporter::send()
{
    BufferString url( host_);
    url.add( path_ );
    IOPar postvars;
    postvars.set( "report", report_.buf() );
    postvars.setYN( "return_text", true );

    BufferString remotefname ( OD::Platform::local().shortName(), "_" );
    remotefname.add( ODInst::getPkgVersion ("base") );
    remotefname.add( "_crash_report.dmp" );

    const char* filetype = "dumpfile";
    uiString errmsg;
    if ( crashreportpath_.isEmpty() )
    {
	if ( Network::uploadQuery( url.buf(), postvars, errmsg, 0, &message_ ) )
	    return true;
    }
    else if ( Network::uploadFile( url.buf(), crashreportpath_,
	      remotefname, filetype, postvars, errmsg, 0, &message_ ) )
	return true;

    errmsg_ = tr("Cannot connect to %1\n%2").arg( host_ ).arg( errmsg );
    return false;
}


bool System::IssueReporter::use( const char* filename, bool isbinary )
{
    if ( !File::exists(filename) )
	return false;

    isbinary_ = isbinary;
    if ( !setDumpFileName(filename) )
	return false;

    fillBasicReport();

    return isbinary_ ? true : readReport( filename );
}


bool System::IssueReporter::parseCommandLine()
{
    CommandLineParser parser;
    const char* hostkey = "host";
    const char* pathkey = "path";
    parser.setKeyHasValue( hostkey );
    parser.setKeyHasValue( pathkey );

    BufferStringSet normalargs;
    bool syntaxerror = false;
    parser.getNormalArguments( normalargs );
    if ( normalargs.size()!=1 )
	syntaxerror = true;

    if ( syntaxerror || parser.nrArgs()<1 )
    {
	errmsg_ = tr("Usage: %1 --%2 <filename> [--%3] [--%4 <hostname>] "
		     "[--%5 <path>]").arg( parser.getExecutable() )
		     .arg( CrashDumper::sKeyDumpFile() ).arg( sKey::Binary() )
		     .arg( hostkey ).arg( pathkey );
	return false;
    }

    const BufferString& filename = normalargs.get( 0 );

    parser.getVal( hostkey, host_ );
    parser.getVal( pathkey, path_ );
    isbinary_ = parser.hasKey( sKey::Binary() );

    if ( !setDumpFileName(filename) )
	return false;

    fillBasicReport();
    return isbinary_ ? true : readReport( filename );
}
