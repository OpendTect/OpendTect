/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "issuereporter.h"

#include "odhttp.h"
#include "commandlineparser.h"
#include "iopar.h"
#include "file.h"
#include "filepath.h"

#include <fstream>
#include "separstr.h"
#include "oddirs.h"
#include "odinst.h"


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
	errmsg_ = filename;
	errmsg_.add( " does not exist" );
	return false;
    }
    
    std::ifstream fstream( filename );
    if ( !fstream )
    {
	errmsg_ = "Cannot open ";
	errmsg_.add( filename ).add( ".");
	return false;
    }
    
    report_.setEmpty();
    
    report_.add( "User: ").add( GetSoftwareUser() ).add( "\n\n" );

    
#define mBufSize 10000
    BufferString unfilteredreport;
    char buf[mBufSize+1];
    while ( fstream.read( buf, mBufSize ) )
    {
	buf[fstream.gcount()] = 0;
	unfilteredreport +=  buf;
    }
    
    buf[fstream.gcount()] = 0;
    unfilteredreport +=  buf;
    
    
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
	errmsg_ = filename;
	errmsg_.add( " does not exist" );
	return false;
    }

    report_.setEmpty();
    BufferString unfilteredreport;
    unfilteredreport.add(  "The file path of crash report is....\n" );
    unfilteredreport.add( filename );

    unfilteredreport.add( "\n\nOpendTect's Version Name is :  " );
    unfilteredreport.add( ODInst::getPkgVersion ( "base" ) );

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
    ODHttp request;
    request.setHost( host_ );
    IOPar postvars;
    postvars.set( "report", report_.buf() );
    
    if ( crashreportpath_.isEmpty() )
    {
	if ( request.post( path_, postvars )!=-1 )
	    return true;
    }
    else if ( request.postFile( path_, crashreportpath_, postvars )!=-1 )
    {
	return true;
    }
    
    errmsg_ = "Cannot connect to ";
    errmsg_.add( host_ );
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
	errmsg_.add( "Usage: " ).add( parser.getExecutable() )
	       .add( " <filename> [--host <hostname>] [--binary]" )
	       .add( " [--path <path>]" );
	return false;
    }
    
    host_ = "www.opendtect.org";
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

