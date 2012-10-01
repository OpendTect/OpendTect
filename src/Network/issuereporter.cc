/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: issuereporter.cc,v 1.3 2012/07/05 13:53:31 cvskris Exp $";

#include "issuereporter.h"

#include "odhttp.h"
#include "iopar.h"
#include "file.h"

#include <fstream>
#include "separstr.h"
#include "oddirs.h"


System::IssueReporter::IssueReporter( const char* host, const char* path )
    : host_( host )
    , path_( path )
{}

#define mTestStart( startstr ) \
    { const BufferString test( startstr ); \
	if ( test.isStartOf( line.buf() ) ) \
	    continue; \
    }

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



bool System::IssueReporter::send()
{
    IOPar postvars;
    postvars.set( "report", report_.buf() );
    
    
    ODHttp request;
    request.setHost( host_ );
    if ( request.post( path_, postvars )==-1 )
    {
	errmsg_ = "Cannot connect to ";
	errmsg_.add( host_ );
	return false;
    }
    
    return true;
}


bool System::IssueReporter::parseCommandLine( int argc, char** argv )
{
    if ( argc<2 )
    {
	errmsg_.add( "Usage: " ).add( argv[0] )
	       .add( " <filename> [--host <hostname>]" )
	       .add( " [--path <path>]" );
	return false;
    }
    
    BufferString hostname = "www.opendtect.org";
    BufferString path = "/relman/scripts/crashreport.php";
    BufferString filename;
    
    int argidx = 1;
    
    while ( argc > argidx )
    {
	const FixedString arg = argv[argidx];
	const int argvaridx = argidx+1;
	
	if ( arg == "--host" && argc>argvaridx )
	{
	    hostname = argv[argvaridx];
	    argidx++;
	}
	else if ( arg=="--path" && argc>argvaridx )
	{
	    path = argv[argvaridx];
	    argidx++;
	}
	else
	{
	    filename = arg;
	}
	
	argidx++;
    }
    
    host_ = hostname;
    path_ = path;
    
    return readReport( filename );
}
