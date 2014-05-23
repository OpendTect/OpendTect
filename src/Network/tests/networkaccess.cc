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


FilePath tempfile;

bool testPing()
{
    const char* url = "http://opendtect.org";
    uiString err;
	
    mRunStandardTestWithError( Network::ping(url,err),
				"Ping existant URL", err.getFullString() );

    const char* missingurl = "http://opendtect.org/thisfiledoesnotexist";
    mRunStandardTestWithError( Network::ping(missingurl,err)==false,
	"Ping non-existant URL", err.getFullString() );

    return true;
}


bool testDownloadToBuffer()
{
    const char* url = "http://opendtect.org/dlsites.txt";
    DataBuffer* db = new DataBuffer(1000,4);
    uiString err;

    mRunStandardTestWithError( Network::downloadToBuffer( url, db, err ),
		      "Download to buffer", err.getFullString() );

    mRunStandardTest( db->size()==23,
		      "Download to buffer size" );

    return true;
}


bool testDownloadToFile()
{
    const char* url = "http://opendtect.org/dlsites.txt";
    uiString err;
    mRunStandardTestWithError(
	    Network::downloadFile( url, tempfile.fullPath(), err ),
	    "Download to file", err.getFullString() );
	    
    return true;
}


bool testFileUpload()
{
    const char* url =
		    "http://dgbindia2/testing/ctest/php_do_not_delete_it.php";
    const char* remotefn("test_file");
    uiString err;
    IOPar postvars;
    mRunStandardTestWithError(
	    Network::uploadFile(url, tempfile.fullPath(), remotefn, "dumpfile",
				postvars, err ),
	    "Upload file", err.getFullString());

    return true;
}


bool testQueryUpload()
{
    const char* report = "This is test report";
    IOPar querypars;
    querypars.set( "report", report );
    const char* url =
		    "http://dgbindia2/testing/ctest/php_do_not_delete_it_2.php";
    uiString err;
    mRunStandardTestWithError( Network::uploadQuery( url, querypars, err ),
				"UploadQuery", err.getFullString() );

    return true;
}


bool testFileSizes()
{
    od_int64 sizeremotefile=0,sizeofuploadedfile=0;
    const char* url = "http://dgbindia2/testing/ctest/test_file";
    uiString err;
    Network::getRemoteFileSize( url, sizeremotefile, err );
    url = "http://dgbindia2/testing/ctest/dumpuploads/test_file";
    Network::getRemoteFileSize( url, sizeofuploadedfile, err );

   
    mRunStandardTestWithError(
	    sizeofuploadedfile >= 0 && sizeremotefile >= 0 &&
	    sizeofuploadedfile == sizeremotefile,
				       "TestFileSizes", err.getFullString() );
    return true;
}


int main(int argc, char** argv)
{
    QCoreApplication app( argc, argv );
    mInitTestProg();

    tempfile = FilePath::getTempDir();
    mRunStandardTest( !tempfile.isEmpty(), "Temp-dir generation" );

    BufferString filename( toString(GetPID()), "_dlsites.txt" );
    tempfile.add( filename );

    bool res = true;

    if ( res && !testPing() )
	res = false;

    if ( res && !testDownloadToBuffer() )
	res = false;

    if ( res && !testDownloadToFile() )
	res = false;

    if ( res && !testFileUpload() )
	res = false;

    if ( res && !testQueryUpload() )
	res = false;

    if ( res && !testFileSizes() )
	res = false;

    File::remove( tempfile.fullPath() );

    ExitProgram( res ? 0 : 1 );
}
