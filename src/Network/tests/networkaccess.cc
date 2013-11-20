/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odnetworkaccess.h"

#include "commandlineparser.h"
#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "od_istream.h"
#include "od_ostream.h"

#include <iostream>
#include <QCoreApplication>

using namespace std;

#define returnError \
{ \
    std::cout<< err.buf(); \
    return false; \
}


bool testPing( const bool quiet )
{
    const char* url = "http://opendtect.org";
    BufferString err;
    if ( !Network::ping( url, err ) )
	returnError

    return true;
}


bool testDownloadToBuffer( const bool quiet )
{
    const char* url = "http://opendtect.org/dlsites.txt";
    DataBuffer* db = new DataBuffer(1000,4);
    BufferString err;
    if ( !Network::downloadToBuffer( url, db, err ) )
	returnError

    if ( db->size() != 23 )
    {
	err.add( "Downloaded file is corrupt. File size did not match." );
	returnError
    }

    return true;
}


bool testDownloadToFile( const bool quiet )
{
    const char* url = "http://opendtect.org/dlsites.txt";
    BufferString err;
    FilePath outpath( FilePath::getTempDir() );
    if ( outpath.isEmpty() )
    {
	err.add( "Temp directory path is empty." );
	returnError
    }

    outpath.add( "dlsites.txt" );
    if ( !Network::downloadFile( url, outpath.fullPath(), err ) )
	returnError

    return true;
}


bool testFileUpload( const bool quiet )
{
    const char* url = 
		    "http://dgbindia1/testing/ctest/php_do_not_delete_it.php";
    FilePath localfp( FilePath::getTempDir() );
    localfp.add( "dlsites.txt" );
    const char* remotefn("test_file");
    BufferString err;
    IOPar postvars;
    if ( !Network::uploadFile(url, localfp.fullPath(), remotefn, "dumpfile", 
			      postvars, err) )
	  returnError

    
    return true; 
}


bool testQueryUpload( const bool quiet )
{
    const char* report = "This is test report";
    IOPar querypars;
    querypars.set( "report", report );
    const char* url =
		    "http://dgbindia1/testing/ctest/php_do_not_delete_it_2.php";
    BufferString err;
    if ( !Network::uploadQuery( url, querypars, err ) )
	returnError

    return true;
}


bool testFileSizes( const bool quiet )
{
    od_int64 sizeremotefile=0,sizeofuploadedfile=0;
    const char* url = "http://dgbindia1/testing/ctest/test_file";
    BufferString err;
    Network::getRemoteFileSize( url, sizeremotefile, err );
    url = "http://dgbindia1/testing/ctest/dumpuploads/test_file";
    Network::getRemoteFileSize( url, sizeofuploadedfile, err );

    if ( sizeofuploadedfile < 0 || sizeremotefile < 0 || 
	 sizeofuploadedfile != sizeremotefile )
	 returnError

    return true;
}


int main(int argc, char** argv)
{
    QCoreApplication app( argc, argv );

    od_init_test_program( argc, argv );
    CommandLineParser clparser;
    const bool quiet = clparser.hasKey( sKey::Quiet() );

    if ( !testPing(quiet) )
	ExitProgram(1);

    if ( !testDownloadToBuffer(quiet) )
	ExitProgram(1);

    if ( !testDownloadToFile(quiet) )
	ExitProgram(1);

    if ( !testFileUpload(quiet) )
	ExitProgram(1);

    if ( !testQueryUpload(quiet) )
	ExitProgram(1);

    if ( !testFileSizes(quiet) )
	ExitProgram(1);

    ExitProgram(0);
}
