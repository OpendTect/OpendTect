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
#include "od_istream.h"
#include "ascstream.h"
#include "odnetworkaccess.h"
#include "perthreadrepos.h"
#include "separstr.h"
#include "systeminfo.h"
#include "testprog.h"


static FilePath tempfile_;
static FilePath tempfile1_;
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
				BufferString(prefix_, "Ping existing URL"),
				toString(err) );

    url.set( "https://nonexistingurl.www" ).add( "/thisfiledoesnotexist" );
    mRunStandardTestWithError( Network::ping(url.str(),err)==false,
	BufferString(prefix_, "Ping non-existing URL"), toString(err) );

    url.set( "https://cdash.dgbes.com" );
    mRunStandardTestWithError( Network::ping(url.str(),err)==false,
	BufferString(prefix_, "Ping existing URL - invalid certificate"),
	toString(err) );

    return true;
}


bool testDownloadToBuffer()
{
    const char* url = "https://opendtect.org/dlsites.txt";
    DataBuffer db( 1000, 4 );
    uiString err;

    uiRetVal uirv = Network::downloadToBuffer_( url, db );
    mRunStandardTestWithError( uirv.isOK(),
	    BufferString(prefix_, "downloadToBuffer_(): Download to buffer"),
	    uirv.getText() );
    mRunStandardTestWithError( Network::downloadToBuffer( url, db, err ),
	    BufferString(prefix_, "downloadToBuffer(): Download to buffer"),
	    uirv.getText() );
    mRunStandardTest( db.size()==54,
		      BufferString(prefix_, "Download to buffer size") );

    return true;
}


bool testGetUrls( BufferStringSet& pkgurls, const bool& setfail )
{
    const BufferString verurl(
		"https://download.opendtect.org/relman/0.0.0/versions.txt" );
    FilePath verfp( tempfile1_, "versions.txt" );

    const uiRetVal uirv = Network::downloadFile_( verurl, verfp.fullPath() );
    mRunStandardTestWithError( uirv.isOK(),
		BufferString(prefix_, "Read Package urls"), uirv.getText() );

    BufferString osstr;
    if ( __islinux__ )
	osstr.add( "_lux64" );
    if ( __iswin__ )
	osstr.set( "_win64" );
    if ( __ismac__ )
	osstr.set( "_mac" );

    BufferStringSet pkgnms( "base", "basedata", "demosurvey" );
    od_istream istrm( verfp );
    ascistream ascstrm( istrm, false );

    mRunStandardTestWithError( ascstrm.isOK(),
	"Open stream versions.txt", "Failed to open stream for version.txt" );

    IOPar verspar( ascstrm );

    mRunStandardTestWithError( !verspar.isEmpty(),
	"Reading versions.txt", "Version.txt is empty" );

    for ( const auto* pkgnm : pkgnms )
    {
	FilePath baseurl( "https://download.opendtect.org/relman/0.0.0" );
	BufferString nm( *pkgnm );

	if ( pkgnm->isEqual("base") )
	{
	    nm.add( osstr );
	    if ( setfail )
		baseurl.add( "fail" ); // canfailurl
	}
	else if ( __ismac__ )
	    nm.add( osstr );

	FileMultiString pkgsepar;
	const bool isver = verspar.get( nm, pkgsepar );
	if ( !isver || pkgsepar.isEmpty() )
	    continue;

	const OD::String& pkgver = pkgsepar.first();
	nm.add( ".zip" );
	baseurl.add( *pkgnm ).add( pkgver ).add( nm );
	pkgurls.add( baseurl.fullPath() );
    }

    mRunStandardTestWithError( !pkgurls.isEmpty(), "Package urls",
						   "Package urls is empty" );

    return true;
}


bool testDownloadToFile()
{
    const char* url = "https://opendtect.org/dlsites.txt";
    const uiRetVal uirv = Network::downloadFile_( url, tempfile_.fullPath() );
    const uiString err = uirv.messages().cat();
    uiString err1;
    mRunStandardTestWithError( Network::downloadFile(url,
		tempfile_.fullPath(), err1),
		BufferString(prefix_, "downloadFile(): Download to file"),
		toString(err) );
    mRunStandardTestWithError( uirv.isOK(),
		BufferString(prefix_, "downloadFile_(): Download to file"),
		toString(err) );

    return true;
}


bool testDownloadFiles()
{
    BufferStringSet pkgurls;
    BufferStringSet failpkgurls;

    if ( !testGetUrls(pkgurls, false) )
	return false;

    if ( !testGetUrls(failpkgurls, true) )
	return false;

    const uiRetVal uirv1 = Network::downloadFiles_( pkgurls,
				tempfile1_.fullPath() );
    const uiRetVal uirv2 = Network::downloadFiles_( failpkgurls,
				tempfile1_.fullPath(), nullptr, true );
    const uiRetVal uirv3 = Network::downloadFiles_( failpkgurls,
				tempfile1_.fullPath() );
    uiString err;

    mRunStandardTestWithError( Network::downloadFiles( pkgurls,
		tempfile1_.fullPath(), err ),
		BufferString(prefix_, "downloadFiles(), Good urls"),
		toString(err) );
    mRunStandardTestWithError( !Network::downloadFiles( failpkgurls,
		tempfile1_.fullPath(), err ),
		BufferString(prefix_, "downloadFiles(), Fail urls"),
		toString(err) );
    mRunStandardTestWithError( uirv1.isOK(),
	BufferString(prefix_, "Download files, Good urls, CanFail: False"),
		     uirv1.getText() );
    mRunStandardTestWithError( !uirv2.isOK(),
	BufferString(prefix_, "Download files, Fail urls, CanFail: True"),
		     uirv2.getText() );
    mRunStandardTestWithError( !uirv3.isOK(),
	BufferString(prefix_, "Download files, Fail urls, CanFail: False"),
		     uirv3.getText() );

    if ( File::exists(tempfile1_.fullPath()) )
	File::removeDir( tempfile1_.fullPath() );

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
			"dumpfile", postvars, err),
	    BufferString(prefix_, "Upload file "), toString(err) );

    return true;
}


bool testQueryUpload()
{
    const char* report = "This is test report";
    IOPar querypars;
    querypars.set( "report", report );
    const BufferString url = intranetUrl( "query_uploadtest.php" );
    uiString err;
    mRunStandardTestWithError( Network::uploadQuery(url, querypars, err),
		BufferString(prefix_, "UploadQuery"), toString(err) );

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

    if ( !testDownloadFiles() )
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
    tempfile1_ = tempfile_;
    mRunStandardTest( !tempfile_.isEmpty(), "Temp-dir generation" );

    const BufferString filename( toString(GetPID()), "_dlsites.txt" );
    tempfile_.add( filename );
    const BufferString filename1( toString(GetPID()), "_dlsites" );
    tempfile1_.add( filename1 );

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
