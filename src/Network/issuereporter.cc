/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "winutils.h"

#include <fstream>

System::IssueReporter::IssueReporter( const char* host, const char* path )
    : host_( host )
    , path_( path )
{}


#define mTestMatch( matchstr ) \
    if ( line.matches( "*" matchstr "*" ) ) \
	continue;

bool System::IssueReporter::readReport( const char* filename )
{
    if ( !File::exists( filename ) )
    {
	errmsg_ = tr( "%1 does not exist" ).arg( filename );
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

    report_.setEmpty();

    report_.add( "User: ").add( GetSoftwareUser() ).add( "\n\n" );

    BufferString unfilteredreport;

    if ( !fstream.getAll( unfilteredreport ) )
	mStreamError( tr("read") );

    SeparString sep( unfilteredreport.buf(), '\n' );

    for ( int idx=0; idx<sep.size(); idx++ )
    {
	BufferString line = sep[idx];

	mTestMatch( "zypper" )
	mTestMatch( "no debugging symbols found" );
	mTestMatch( "Missing separate debuginfo for" );
	mTestMatch( "no loadable sections found in added symbol-file");
	report_.add( line ).add( "\n" );
    }

    return true;
}


bool System::IssueReporter::setDumpFileName( const char* filename )
{
    if ( !File::exists( filename ) )
    {
	errmsg_ = tr( "%1 does not exist" ).arg( filename );
	return false;
    }

    report_.setEmpty();
    BufferString unfilteredreport;
    unfilteredreport.add(  "The file path of crash report is....\n" );
    unfilteredreport.add( filename );

    unfilteredreport.add( "\n\nOpendTect's Version Name is :  " );
    unfilteredreport.add( ODInst::getPkgVersion ( "base" ) );
    unfilteredreport.add( "\nUser's platform is : " );
    unfilteredreport.add( OD::Platform::local().longName() );

    IOPar dumppar; OD::dumpMemInfo( dumppar );
    BufferString dumpmemstr;
    dumppar.dumpPretty( dumpmemstr );
    unfilteredreport.add( "\n" ).add( dumpmemstr );

    unfilteredreport.add( "Nr. of Processors : " );
    unfilteredreport.add( Threads::getNrProcessors() );
    #ifdef __win__
    unfilteredreport.add( "\nWindows OS Version : " );
    unfilteredreport.add( getFullWinVersion() );
    #endif

    SeparString sep( unfilteredreport.buf(), '\n' );

    for ( int idx=0; idx<sep.size(); idx++ )
    {
	BufferString line = sep[idx];
	report_.add( line ).add( "\n" );
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
    
    BufferString remotefname ( OD::Platform::local().shortName(), "_" );
    remotefname.add( ODInst::getPkgVersion ("base") );
    remotefname.add( "_" ).add( "crash.dmp" );

    const char* filetype = "dumpfile";
    uiString errmsg;
    if ( crashreportpath_.isEmpty() )
    {
	if ( Network::uploadQuery( url.buf(), postvars, errmsg ) )
	    return true;
    }
    else if ( Network::uploadFile( url.buf(), crashreportpath_,
	      remotefname, filetype, postvars, errmsg ) )
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
	errmsg_ =
	    uiString( "Usage: %1 <filename> [--host <hostname>] [--binary]"
		     "[--path <path>]" ).arg( parser.getExecutable() );
	return false;
    }

    host_ = "http://opendtect.org";
    path_ = "/relman/scripts/crashreport.php";

    const BufferString& filename = normalargs.get( 0 );

    parser.getVal( hostkey, host_ );
    parser.getVal( pathkey, path_ );
    const bool binary = parser.hasKey( "binary" );

    if ( binary )
    {
	return setDumpFileName( filename );
    }

    return readReport( filename );
}

