/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ziputils.h"

#include "od_iostream.h"
#include "testprog.h"
#include "file.h"
#include "filepath.h"


class FileDisposer
{
public:

    FileDisposer( const char* fnm )
	: fnm_(fnm)
    {}

    ~FileDisposer()
    {
	const char* fnm = fnm_.buf();
	if ( File::isDirectory(fnm) && !File::isSymLink(fnm) )
	    File::removeDir( fnm );
	else
	    File::remove( fnm );
    }

private:
    BufferString fnm_;
};

#define mCleanup() \
    delete taskrun; \
    File::remove( zipfilename.fullPath() ); \
    File::remove( zipfilename2.fullPath() ); \
    File::removeDir( outputdir.fullPath() ); \


#define mRunTest(testname,command) \
{ \
if ( !command ) \
{ \
    errStream() << testname << " failed!\n" << err.getFullString(); \
    mCleanup() \
    return 1; \
} \
else \
    logStream() << testname << " Succeeded!\n"; \
}


#define mCheckDataIntegrity(relpart1,relpart2) \
{ \
    FilePath src( tozip.fullPath() ); \
    src.add( relpart1 ); \
    src.add( relpart2 ); \
    FilePath dest( zipfilename.pathOnly() ); \
    dest.add( "F3_Test_Survey" ); \
    dest.add( relpart1 ); \
    dest.add( relpart2 ); \
    if ( File::getFileSize(dest.fullPath()) != \
					  File::getFileSize(src.fullPath()) ) \
    { \
	errStream() << "Data integrety check failed!\n" \
			       << dest.fullPath(); \
	mCleanup() \
	return 1; \
    } \
    else \
	logStream() << "Data integrety check succeeded!\n"; \
}


static bool testLongPath()
{
    const BufferString tempdirfp = FilePath::getTempDir();;
    FilePath fp( tempdirfp.str(), "od_test_zip_longpath" );
    const BufferString workdir = fp.fullPath();

    if ( File::exists(workdir.str()) )
    {
	mRunStandardTest( File::removeDir(fp.fullPath()),
			  "Remove existing long directory" );
    }

    FileDisposer disposer( workdir.str() );

    int idx = 0;
#ifdef __win__
    const int minpathsz = MAX_PATH + 40;
#else
    const int minpathsz = 300;
#endif

    //Ensure we are above the 8.3 windows shortpath standard: Use 15.5
    const int dirsz = 15; // Size of one folder level;
    while( fp.fullPath().size()<minpathsz )
    {
	BufferString filenm;
	while ( filenm.size() < dirsz )
	{
	    char c = 'a' + idx;
	    filenm.add( c );
	    if ( ++idx >= 26 )
		idx = 0;
	}

	fp.add( filenm.str() );
    }

    const OD::String& lastdir = fp.dir( fp.nrLevels()-2 );
    (char&) lastdir[7] = od_space;
    fp.setExtension( "extxt" ); // extension size: 5
    const BufferString dirnm = fp.pathOnly();
    const BufferString fnm = fp.fullPath();
    mRunStandardTest( File::createDir( dirnm.str() ), "Create long directory" );


    const BufferString contentstr( "Some file content" );
    od_ostream ostrm( fnm.str() );
    mRunStandardTestWithError( ostrm.isOK(),
			       "Open output file stream on long filepath",
			       toString(ostrm.errMsg()) );

    ostrm.add( contentstr.str() ).addNewLine();
    mRunStandardTestWithError( ostrm.isOK(),
			       "Write stream content on long filepath",
			       toString(ostrm.errMsg()) );
    ostrm.close();

    od_istream istrm( fnm.str() );
    mRunStandardTestWithError( istrm.isOK(),
			       "Open input file stream on long filepath",
			       toString(istrm.errMsg()) );
    BufferString retstr;
    mRunStandardTestWithError( istrm.getAll( retstr ),
			       "Read stream content on long filepath",
			       toString(istrm.errMsg()) );
    mRunStandardTest( retstr == contentstr, "File content on long filepath" );
    istrm.close();

    FilePath zipfp( workdir.str() );
    zipfp.setExtension( "zip" );
    const BufferString zipfnm = zipfp.fullPath();
    FileDisposer zipdisposer( zipfnm.str() );

    uiString err;
    mRunStandardTestWithError(
	ZipUtils::makeZip( workdir.str(), tempdirfp.str(), zipfnm.str(), err ),
	"Zip archive with long path", toString( err ) );

    mRunStandardTest( File::removeDir( workdir.str() ),
		      "Remove directory before unzipping" );

    mRunStandardTestWithError(
	ZipUtils::unZipArchive( zipfnm.str(), tempdirfp.str(), err ),
	"Unzip archive with long path", toString( err ) );

    istrm.open( fnm.str() );
    mRunStandardTestWithError( istrm.isOK(),
			       "Re-open input file stream on long filepath",
			       toString(istrm.errMsg()) );

    retstr.setEmpty();
    mRunStandardTestWithError( istrm.getAll( retstr ),
			      "Read unpackaged stream content on long filepath",
			      toString(istrm.errMsg()) );
    mRunStandardTest( retstr == contentstr,
		      "Unpackaged file content on long filepath" );
    istrm.close();

    mRunStandardTest( File::removeDir( workdir.str() ),
		      "Remove directory after last test" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( __iswin__ && !testLongPath() )
	return 1;

    BufferString basedir;
    clParser().getVal("datadir", basedir );
    FilePath tozip( basedir );
    tozip.add( "F3_Test_Survey" );
    const FilePath zipfilename(
		FilePath::getTempFullPath("zip_test","zip") );
    const FilePath zipfilename2(
		FilePath::getTempFullPath("zip_test2","zip") );

    FilePath outputdir( zipfilename.pathOnly() );
    outputdir.add( "F3_Test_Survey" );

    TaskRunner* taskrun = nullptr;
    if ( !quiet_ )
	taskrun = new TextTaskRunner( logStream() );

    uiString err;
    mRunTest("Zipping", ZipUtils::makeZip(tozip.fullPath(),nullptr,
					 zipfilename.fullPath(),err,taskrun) );
    mRunTest("Zipping (copy)", ZipUtils::makeZip(tozip.fullPath(),nullptr,
					 zipfilename2.fullPath(),err,taskrun) );
    mRunTest("Unzipping", ZipUtils::unZipArchive(zipfilename.fullPath(),
					 outputdir.pathOnly(),err,taskrun) );
    mRunTest("Unzipping (copy)", ZipUtils::unZipArchive(zipfilename2.fullPath(),
					 outputdir.pathOnly(),err,taskrun) );

    mCheckDataIntegrity( "Seismics","Seismic.cbvs" );
    mCheckDataIntegrity( "Misc",".omf" );
    mCheckDataIntegrity( "Surfaces","horizonrelations.txt" );
    mCheckDataIntegrity( "WellInfo","F03-4^5.wll" );

    mCleanup();

    return 0;
}
