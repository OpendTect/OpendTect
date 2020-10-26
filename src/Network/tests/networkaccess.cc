/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Nov 2013
-*/


#include "testprog.h"

#include "applicationdata.h"
#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "odnetworkaccess.h"
#include "staticstring.h"
#include "systeminfo.h"


static File::Path tempfile;
static BufferString prefix_;

static BufferString intranetHost()
{
    mDeclStaticString(res);
    if ( res.isEmpty() )
    {
	res.set( "intranet" );
	const BufferString addr( System::hostAddress(res.buf()) );
	if ( addr.isEmpty() )
	    res.set( "192.168.0.245" );
    }

    return res;
}

static BufferString intranetUrl( const char* url )
{
    BufferString res( "http://", intranetHost(), "/" );
    res.add( "testing/ctest/" ).add( url );
    return res;
}


bool testPing()
{
    BufferString url( "http://dgbes.com" );
    uiString err;

    mRunStandardTestWithError( Network::ping(url.str(),err),
				BufferString( prefix_, "Ping existant URL"),
				toString(err) );

    url.add( "/thisfiledoesnotexist" );
    mRunStandardTestWithError( Network::ping(url.str(),err)==false,
	BufferString( prefix_, "Ping non-existent URL"), toString(err) );

    return true;
}


bool testDownloadToBuffer()
{
    const char* url = "https://opendtect.org/dlsites.txt";
    DataBuffer db( 1000, 4 );
    uiString err;

    mRunStandardTestWithError( Network::downloadToBuffer( url, db, err ),
	    BufferString( prefix_, "Download to buffer"), toString(err) );

    mRunStandardTest( db.size()==54,
		      BufferString( prefix_, "Download to buffer size") );

    return true;
}


bool testDownloadToFile()
{
    const char* url = "https://opendtect.org/dlsites.txt";
    uiString err;
    mRunStandardTestWithError(
	    Network::downloadFile( url, tempfile.fullPath(), err ),
	    BufferString( prefix_, "Download to file"), toString(err) );

    return true;
}


bool testFileUpload()
{
    const BufferString url = intranetUrl( "php_do_not_delete_it.php" );
    const char* remotefn( "test_file" );
    uiString err;
    IOPar postvars;
    mRunStandardTestWithError(
	    Network::uploadFile(url.buf(), tempfile.fullPath(), remotefn, "dumpfile",
				postvars, err ),
	    BufferString( prefix_, "Upload file "), toString(err) );

    return true;
}


bool testQueryUpload()
{
    const char* report = "This is test report";
    IOPar querypars;
    querypars.set( "report", report );
    const BufferString url = intranetUrl( "php_do_not_delete_it_2.php" );
    uiString err;
    mRunStandardTestWithError( Network::uploadQuery( url, querypars, err ),
		BufferString( prefix_, "UploadQuery"), toString(err) );

    return true;
}


bool testFileSizes()
{
    const char* url = "https://opendtect.org/dlsites.txt";
    od_int64 sizeremotefile=0,sizeofuploadedfile=0;
    uiString err;
    Network::getRemoteFileSize( url, sizeremotefile, err );
    tstStream() << url << " is " << sizeremotefile << " bytes" << od_endl;
    const BufferString url2 = intranetUrl( "test_file" );
    Network::getRemoteFileSize( url2, sizeremotefile, err );
    const BufferString url3 = intranetUrl( "dumpuploads/test_file" );
    Network::getRemoteFileSize( url3, sizeofuploadedfile, err );


    mRunStandardTestWithError(
	    sizeofuploadedfile >= 0 && sizeremotefile >= 0 &&
	    sizeofuploadedfile == sizeremotefile,
	       BufferString( prefix_, "TestFileSizes"), toString(err) );
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


int mTestMainFnName(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app;

    prefix_ = "[Without eventloop] ";

    tempfile = File::Path::getTempDir();
    mRunStandardTest( !tempfile.isEmpty(), "Temp-dir generation" );

    BufferString filename( toString(GetPID()), "_dlsites.txt" );
    tempfile.add( filename );

    bool res = runTests();
    File::remove( tempfile.fullPath() );
    if ( !res )
	return 1;

    Threads::Thread thread( mSCB( threadCB ), "test_networkaccess thread" );
    thread.waitForFinish();

    if ( !threadres )
	return 1;

    const CallBack loopcb( mSCB(loopCB) );
    CallBack::addToMainThread( loopcb );
    const int retval = app.exec();
    CallBack::removeFromThreadCalls( loopcb.cbObj() );

    return retval;
}
