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

#include <csignal>

static const char* sKeySurvey = "F3_Test_Survey";

static BufferString workdir_;
static BufferString zipfnm_;
static BufferString zipfilename_;
static BufferString zipfilename2_;
static BufferString outputdir_;


void cleanup()
{
    if ( File::exists(workdir_.buf()) )
	File::removeDir( workdir_.buf() );

    if ( File::exists(zipfnm_.buf()) )
	File::remove( zipfnm_.buf() );

    if ( File::exists(zipfilename_.buf()) )
	File::remove( zipfilename_.buf() );

    if ( File::exists(zipfilename2_.buf()) )
	File::remove( zipfilename2_.buf() );

    if ( File::exists(outputdir_.buf()) )
	File::removeDir( outputdir_.buf() );
}

void signalHandler( int signum )
{
    errStream() << "Interrupt signal (" << signum << ") received." << od_endl;
    exit( signum );
}


static bool checkDataIntegrity( const char* srcdir, const char* destdir,
				const char* relpart1,const char* relpart2 )
{
    const FilePath src( srcdir, relpart1, relpart2 );
    const FilePath dest( destdir, sKeySurvey, relpart1, relpart2 );
    BufferString descstr( "Data integrity check for: ", relpart1, "/" );
    descstr.add( relpart2 );
    mRunStandardTest( File::getFileSize(dest.fullPath()) ==
		      File::getFileSize(src.fullPath()), descstr.str() );

    return true;
}


static bool testLongPath()
{
    const BufferString tempdirfp = FilePath::getTempDir();;
    FilePath fp( tempdirfp.str(), "od_test_zip_longpath" );
    workdir_ = fp.fullPath();

    if ( File::exists(workdir_.str()) )
    {
	mRunStandardTest( File::removeDir(workdir_.str()),
			  "Remove existing long directory" );
    }

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

    FilePath zipfp( workdir_.str() );
    zipfp.setExtension( "zip" );
    zipfnm_ = zipfp.fullPath();

    uiString err;
    mRunStandardTestWithError(
	ZipUtils::makeZip( workdir_.str(), tempdirfp.str(), zipfnm_.str(), err),
	"Zip archive with long path", toString( err ) );

    mRunStandardTest( File::removeDir( workdir_.str() ),
		      "Remove directory before unzipping" );

    mRunStandardTestWithError(
	ZipUtils::unZipArchive( zipfnm_.str(), tempdirfp.str(), err ),
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

    mRunStandardTest( File::remove( zipfnm_.buf() ),
		      "Remove zip file after last test" );

    mRunStandardTest( File::removeDir( workdir_.str() ),
		      "Remove directory after last test" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    signal( SIGINT, signalHandler );
    signal( SIGTERM, signalHandler );
    atexit( cleanup );

    if ( __iswin__ && !testLongPath() )
	return 1;

    BufferString basedir;
    clParser().getVal("datadir", basedir );
    const FilePath tozip( basedir.buf(), sKeySurvey );
    const FilePath zipfilename(
		FilePath::getTempFullPath("zip_test","zip") );
    const FilePath zipfilename2(
		FilePath::getTempFullPath("zip_test2","zip") );

    FilePath outputdir( zipfilename.pathOnly() );
    outputdir.add( sKeySurvey );

    zipfilename_ = zipfilename.fullPath();
    zipfilename2_ = zipfilename2.fullPath();
    outputdir_ = outputdir.fullPath();

    TextTaskRunner taskrun( logStream() );

    uiString err;
    mRunStandardTestWithError(
		ZipUtils::makeZip( tozip.fullPath(), nullptr,
				   zipfilename_.buf(), err, &taskrun ),
		"Zipping", err.getFullString() );
    mRunStandardTestWithError(
		ZipUtils::makeZip( tozip.fullPath(), nullptr,
				   zipfilename2_.buf(), err, &taskrun ),
		"Zipping (copy)", err.getFullString() );
    mRunStandardTestWithError(
		ZipUtils::unZipArchive( zipfilename_.buf(),
					outputdir.pathOnly(), err, &taskrun ),
		"Unzipping", err.getFullString() );
    mRunStandardTestWithError(
		ZipUtils::unZipArchive( zipfilename2_.buf(),
					outputdir.pathOnly(), err, &taskrun ),
		"Unzipping (copy)", err.getFullString() );

    const BufferString tozipfnm = tozip.fullPath();
    const BufferString zipbasefp = zipfilename.pathOnly();
    if ( !checkDataIntegrity(tozipfnm.buf(),zipbasefp.buf(),
			     "Seismics","Seismic.cbvs") ||
	 !checkDataIntegrity(tozipfnm.buf(),zipbasefp.buf(),
			     "Misc",".omf") ||
	 !checkDataIntegrity(tozipfnm.buf(),zipbasefp.buf(),
			     "Surfaces","horizonrelations.txt") ||
	 !checkDataIntegrity(tozipfnm.buf(),zipbasefp.buf(),
			     "WellInfo","F03-4^5.wll") )
	return 1;

    return 0;
}
