/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "applicationdata.h"
#include "databuf.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "moddepmgr.h"
#include "odnetworkaccess.h"
#include "perthreadrepos.h"
#include "systeminfo.h"
#include "testprog.h"


static FilePath tempfile_;
static BufferString prefix_;

static BufferString intranetHost()
{
    mDeclStaticString(res);
    if ( res.isEmpty() )
    {
	res.set( "oldintranet.dgbes.com" );
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
    BufferString url( "https://dgbes.com" );
    uiString err;

    mRunStandardTestWithError( Network::ping(url.str(),err),
				BufferString( prefix_, "Ping existing URL"),
				toString(err) );

    url.set( "http://nonexistingurl.www" ).add( "/thisfiledoesnotexist" );
    mRunStandardTestWithError( Network::ping(url.str(),err)==false,
	BufferString( prefix_, "Ping non-existing URL"), toString(err) );

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
	    Network::downloadFile( url, tempfile_.fullPath(), err ),
	    BufferString( prefix_, "Download to file"), toString(err) );

    return true;
}


bool testFileUpload()
{
    const BufferString url = intranetUrl( "file_uploadtest.php" );
    const char* remotefn( "test_file" );
    uiString err;
    IOPar postvars;
    mRunStandardTestWithError(
    Network::uploadFile( url.buf(), tempfile_.fullPath(), remotefn,
			"dumpfile", postvars, err ),
	    BufferString( prefix_, "Upload file "), toString(err) );

    return true;
}


bool testQueryUpload()
{
    const char* report = "This is test report";
    IOPar querypars;
    querypars.set( "report", report );
    const BufferString url = intranetUrl( "query_uploadtest.php" );
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
    File::remove( tempfile_.fullPath() );
}


void loopCB(CallBacker*)
{
    prefix_ = "[With eventloop] ";
    const bool res = runTests();
    File::remove( tempfile_.fullPath() );
    ApplicationData::exit( res ? 0 : 1 );
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    ApplicationData app;
    OD::ModDeps().ensureLoaded( "Network" );

    tempfile_ = FilePath::getTempDir();
    mRunStandardTest( !tempfile_.isEmpty(), "Temp-dir generation" );

    BufferString filename( toString(GetPID()), "_dlsites.txt" );
    tempfile_.add( filename );

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
