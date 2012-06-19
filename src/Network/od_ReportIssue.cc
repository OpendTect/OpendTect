/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: od_ReportIssue.cc,v 1.1 2012-06-19 12:07:05 cvskris Exp $";

#include "odhttp.h"
#include "fixedstring.h"
#include "iopar.h"
#include "file.h"

#include "prog.h"
#include <fstream>
#include <iostream>

#include "QCoreApplication"

int main( int argc, char ** argv )
{
    if ( argc<2 )
    {
	std::cerr << "Usage: " << argv[0] << " <filename> [--host <hostname>]"
		     " [--path <path>]\n";
	ExitProgram( 1 );
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
    
    if ( filename.isEmpty() )
    {
	std::cerr << "No file specified\n";
	return 1;
    }

    if ( !File::exists( filename ) )
    {
	std::cerr << "File " << filename.buf() << " does not exist\n";
	return 1;
    }
    
    BufferString filecontents;
    std::ifstream fstream( filename );
    
#define mBufSize 10000
    char buf[mBufSize+1];
    while ( fstream.read( buf, mBufSize ) )
    {
	buf[fstream.gcount()] = 0;
	filecontents +=  buf;
    }
    
    buf[fstream.gcount()] = 0;
    filecontents +=  buf;
    
    IOPar postvars;
    postvars.set( "report", filecontents.buf() );
    
    
    QCoreApplication app( argc, argv );
    ODHttp request;
    request.setHost( hostname );
    if ( request.post( path, postvars )==-1 )
    {
	std::cerr << "Cannot connect to " << hostname << '\n';
	return 1;
    }
    
    std::cerr << "Report submitted\n";
    
    return 0;
}
