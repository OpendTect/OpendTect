/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Nov 2013
-*/


#include "odnetworkaccess.h"
#include "testprog.h"

#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "applicationdata.h"


static FilePath tempfile;
static BufferString prefix_;

bool testPing()
{
    const char* url = "http://opendtect.org";
    uiString err;

    mRunStandardTestWithError( Network::ping(url,err),
				BufferString( prefix_, "Ping existant URL"),
				err.getFullString() );

    const char* missingurl = "http://opendtect.org/thisfiledoesnotexist";
    mRunStandardTestWithError( Network::ping(missingurl,err)==false,
	BufferString( prefix_, "Ping non-existant URL"), err.getFullString() );

    return true;
}


bool testDownloadToBuffer()
{
    const char* url = "http://opendtect.org/dlsites.txt";
    DataBuffer db( 1000, 4 );
    uiString err;

    mRunStandardTestWithError( Network::downloadToBuffer( url, db, err ),
	    BufferString( prefix_, "Download to buffer"), err.getFullString() );

    mRunStandardTest( db.size()==23,
		      BufferString( prefix_, "Download to buffer size") );

    return true;
}


bool testDownloadToFile()
{
    const char* url = "http://opendtect.org/dlsites.txt";
    uiString err;
    mRunStandardTestWithError(
	    Network::downloadFile( url, tempfile.fullPath(), err ),
	    BufferString( prefix_, "Download to file"), err.getFullString() );

    return true;
}


bool testFileUpload()
{
    const char* url =
		    "http://intranet/testing/ctest/php_do_not_delete_it.php";
    const char* remotefn("test_file");
    uiString err;
    IOPar postvars;
    mRunStandardTestWithError(
	    Network::uploadFile(url, tempfile.fullPath(), remotefn, "dumpfile",
				postvars, err ),
	    BufferString( prefix_, "Upload file"), err.getFullString());

    return true;
}


bool testQueryUpload()
{
    const char* report = "This is test report";
    IOPar querypars;
    querypars.set( "report", report );
    const char* url =
		    "http://intranet/testing/ctest/php_do_not_delete_it_2.php";
    uiString err;
    mRunStandardTestWithError( Network::uploadQuery( url, querypars, err ),
		BufferString( prefix_, "UploadQuery"), err.getFullString() );

    return true;
}


bool testFileSizes()
{
    const char* url = "http://opendtect.org/dlsites.txt";
    od_int64 sizeremotefile=0,sizeofuploadedfile=0;
    uiString err;
    Network::getRemoteFileSize( url, sizeremotefile, err );
    tstStream() << url << " is " << sizeremotefile << " bytes" << od_endl;
    url = "http://intranet/testing/ctest/test_file";
    Network::getRemoteFileSize( url, sizeremotefile, err );
    url = "http://intranet/testing/ctest/dumpuploads/test_file";
    Network::getRemoteFileSize( url, sizeofuploadedfile, err );


    mRunStandardTestWithError(
	    sizeofuploadedfile >= 0 && sizeremotefile >= 0 &&
	    sizeofuploadedfile == sizeremotefile,
	       BufferString( prefix_, "TestFileSizes"), err.getFullString() );
    return true;
}

bool runTests()
{
    if ( !testPing() )
	return false;

    if ( !testDownloadToBuffer() )
	return false;

    if ( !testDownloadToFile() )
	return false;

    if ( !testFileUpload() )
	return false;

    if ( !testQueryUpload() )
	return false;

    if ( !testFileSizes() )
	return false;

    return true;
}

static bool threadres;

void threadCB(CallBacker*)
{
    prefix_ = "[From Thread] ";
    threadres = runTests();
    File::remove( tempfile.fullPath() );
}


void loopCB(CallBacker*)
{
    prefix_ = "[With eventloop] ";
    const bool res = runTests();
    File::remove( tempfile.fullPath() );
    ApplicationData::exit( res ? 0 : 1 );
}


int main(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app;

    prefix_ = "[Without eventloop] ";

    tempfile = FilePath::getTempDir();
    mRunStandardTest( !tempfile.isEmpty(), "Temp-dir generation" );

    BufferString filename( toString(GetPID()), "_dlsites.txt" );
    tempfile.add( filename );

    bool res = runTests();
    File::remove( tempfile.fullPath() );
    if ( !res )
	ExitProgram( 1 );

    Threads::Thread thread( mSCB( threadCB ), "test_networkaccess thread" );
    thread.waitForFinish();

    if ( !threadres )
	ExitProgram( 1 );

    CallBack::addToMainThread( mSCB( loopCB ) );
    const int retval = app.exec();
    ExitProgram( retval );
}
