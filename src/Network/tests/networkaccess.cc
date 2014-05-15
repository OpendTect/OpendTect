/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odnetworkaccess.h"
#include "testprog.h"

#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include <QCoreApplication>


#define returnError \
{ \
    od_cout() << err.buf() << od_endl; \
    return false; \
}


bool testPing()
{
    const char* url = "http://opendtect.org";
    BufferString err;
    if ( !Network::ping(url,err) )
	returnError

    const char* missingurl = "http://opendtect.org/thisfiledoesnotexist";
    if ( Network::ping(missingurl,err) )
    {
	od_cout() << "Ping returns success for non-existant URL" << od_endl;
	return false;
    }

    return true;
}


bool testDownloadToBuffer()
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


bool testDownloadToFile()
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


bool testFileUpload()
{
    const char* url =
		    "http://dgbindia2/testing/ctest/php_do_not_delete_it.php";
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


bool testQueryUpload()
{
    const char* report = "This is test report";
    IOPar querypars;
    querypars.set( "report", report );
    const char* url =
		    "http://dgbindia2/testing/ctest/php_do_not_delete_it_2.php";
    BufferString err;
    if ( !Network::uploadQuery( url, querypars, err ) )
	returnError

    return true;
}


bool testFileSizes()
{
    od_int64 sizeremotefile=0,sizeofuploadedfile=0;
    const char* url = "http://dgbindia2/testing/ctest/test_file";
    BufferString err;
    Network::getRemoteFileSize( url, sizeremotefile, err );
    url = "http://dgbindia2/testing/ctest/dumpuploads/test_file";
    Network::getRemoteFileSize( url, sizeofuploadedfile, err );

    if ( sizeofuploadedfile < 0 || sizeremotefile < 0 ||
	 sizeofuploadedfile != sizeremotefile )
	 returnError

    return true;
}


int main(int argc, char** argv)
{
    QCoreApplication app( argc, argv );
    mInitTestProg();

    if ( !testPing() )
	ExitProgram(1);

    if ( !testDownloadToBuffer() )
	ExitProgram(1);

    if ( !testDownloadToFile() )
	ExitProgram(1);

    if ( !testFileUpload() )
	ExitProgram(1);

    if ( !testQueryUpload() )
	ExitProgram(1);

    if ( !testFileSizes() )
	ExitProgram(1);

    return ExitProgram(0);
}
