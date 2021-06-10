/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2003
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
#include "odver.h"
#include "separstr.h"
#include "thread.h"
#include "uistrings.h"
#include "winutils.h"
#include "systeminfo.h"

#include <fstream>

System::IssueReporter::IssueReporter( const char* host, const char* path )
    : host_( host )
    , path_( path )
    , isbinary_(false)
{}


#define mTestMatch( matchstr ) \
    if ( line.matches( "*" matchstr "*" ) ) \
	continue;

bool System::IssueReporter::readReport( const char* filename )
{
    if ( !File::exists( filename ) )
    {
	errmsg_ = uiStrings::phrFileDoesNotExist( filename );
	return false;
    }

#define mStreamError() \
{ \
    errmsg_ = errmsg.arg( filename ).arg( fstream.errMsg() ); \
    return false; \
}

    uiString errmsg = uiStrings::phrCannotOpen(tr(" %2. Reason: %3"));

    od_istream fstream( filename );
    if ( fstream.isBad() )
	mStreamError();


    report_.add( "User: ").add( GetSoftwareUser() ).add( "\n\n" );

    BufferString unfilteredreport;

    errmsg = uiStrings::phrCannotRead(tr(" %2. Reason: %3"));

    if ( !fstream.getAll( unfilteredreport ) )
	mStreamError();

    report_.add( unfilteredreport.buf() );

    return true;
}


void System::IssueReporter::fillBasicReport( const char* filename )
{
    report_.setEmpty();
    BufferString unfilteredreport;
    unfilteredreport.add(  "The file path of crash report is....\n" );
    unfilteredreport.add( filename );

    unfilteredreport.add( "\n\nOpendTect's Version Name is :  " );
    unfilteredreport.add( GetFullODVersion() );
    unfilteredreport.add( "\nUser's platform is : " );
    unfilteredreport.add( OD::Platform::local().longName() );

    unfilteredreport.add( "\nMAC Address Hash: ");
    unfilteredreport.add( System::macAddressHash() );

    IOPar dumppar; OD::dumpMemInfo( dumppar );
    BufferString dumpmemstr;
    dumppar.dumpPretty( dumpmemstr );
    unfilteredreport.add( "\n" ).add( dumpmemstr );

    unfilteredreport.add( "Nr. of Processors : " );
    unfilteredreport.add( Threads::getNrProcessors() );
    #ifdef __win__
    unfilteredreport.add( "\nWindows OS Version : " );
    unfilteredreport.add( WinUtils::getFullWinVersion() );
    #endif

    SeparString sep( unfilteredreport.buf(), '\n' );

    for ( int idx=0; idx<sep.size(); idx++ )
    {
	BufferString line = sep[idx];
	report_.add( line ).add( "\n" );
    }
}


bool System::IssueReporter::setDumpFileName( const char* filename )
{
    if ( !File::exists( filename ) )
    {
	errmsg_ = uiStrings::phrFileDoesNotExist( filename );
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
    remotefname.add( GetFullODVersion() );
    remotefname.add( "_" ).add( "crash_report.dmp" );

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
	errmsg_ = tr("Usage: %1 <filename> [--host <hostname>] [--binary]"
		     "[--path <path>]").arg( parser.getExecutable() );
	return false;
    }

    host_ = "http://backend.opendtect.org";
    path_ = "/backendscripts/crashreport.php";

    const BufferString& filename = normalargs.get( 0 );

    parser.getKeyedInfo( hostkey, host_ );
    parser.getKeyedInfo( pathkey, path_ );
    isbinary_ = parser.hasKey( "binary" );

    if ( !setDumpFileName(filename) )
	return false;

    fillBasicReport( filename );
    return isbinary_ ? true : readReport( filename );
}
