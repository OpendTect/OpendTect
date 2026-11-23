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
#include "math2.h"
#include "timefun.h"

#include <csignal>

static const char* sKeySurvey = "F3_Test_Survey";

static BufferString workdir_;
static BufferString zipfnm_;
static BufferString zipfilename_;
static BufferString zipfilename2_;
static BufferString outputdir_;
static BufferString utf8dir_;
static BufferString utf8zipfnm_;


void cleanup()
{
    if ( File::exists(workdir_.buf()) )
	File::removeDir( workdir_.buf() );

    if ( File::exists(zipfnm_.buf()) )
	File::remove( zipfnm_.buf() );

    if ( File::exists(utf8dir_.buf()) )
	File::removeDir( utf8dir_.buf() );

    if ( File::exists(utf8zipfnm_.buf()) )
	File::remove( utf8zipfnm_.buf() );

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

    const StringBase& lastdir = fp.dir( fp.nrLevels()-2 );
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


static bool hasEntry( const BufferStringSet& entries, const char* fnm )
{
    for ( const auto* entry : entries )
    {
	if ( FilePath(entry->buf()).fileName() == fnm )
	    return true;
    }

    return false;
}


static bool hasUtf8NamesFlag( const char* zipfnm )
{
    od_istream strm( zipfnm );
    unsigned char header[8];
    if ( !strm.isOK() || !strm.getBin(header,8) )
	return false;

    const bool islocalheader = header[0]==0x50 && header[1]==0x4b &&
			       header[2]==0x03 && header[3]==0x04;
    return islocalheader && (header[7] & 0x08);
}


struct FileState
{
    od_int64		size_	= -1;
    File::Permissions	perms_;
    od_int64		modtime_ = -1;
};


static bool getFileState( const char* fnm, FileState& state )
{
    if ( !File::isFile(fnm) )
	return false;

    state.size_ = File::getFileSize( fnm );
    state.perms_ = File::getPermissions( fnm );
    state.modtime_ = File::getTimeInSeconds( fnm );
    return state.size_ >= 0 && !state.perms_.isUdf() &&
	   !mIsUdf(state.modtime_);
}


static bool checkUnzippedFile( const char* fnm, const FileState& before,
			       const char* desc )
{
    FileState after;
    mRunStandardTest( getFileState(fnm,after),
		      BufferString("Unzipped UTF-8 path is a file: ",desc) );
    mRunStandardTest( after.size_ == before.size_,
		      BufferString("Unzipped file size: ",desc) );
    mRunStandardTestWithError( after.perms_ == before.perms_,
		      BufferString("Unzipped file permissions: ",desc),
		      BufferString("Got ",after.perms_.asInt(),", expected ")
			.add(before.perms_.asInt()) );
    mRunStandardTestWithError( Math::Abs(after.modtime_-before.modtime_) <= 2,
		      BufferString("Unzipped file modification time: ",desc),
		      BufferString("Got ",after.modtime_,", expected ")
			.add(before.modtime_) );
    return true;
}


static bool testUtf8Names()
{
    const char* const dirnm =
	"\xC3\xB8y_\xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7";
    const char* const subdirnm =
	"\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
    const char* const fnm =
	"J\xC3\xBCrgen_\xE4\xBD\xA0\xE5\xA5\xBD.txt";
    const char* const subfnm =
	"\xCE\xB3\xCE\xB5\xCE\xB9\xCE\xB1_"
	"\xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF.dat";
    const char* const content =
	"\xC3\xB8y j\xC3\xBCrgen "
	"\xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7 "
	"\xCE\xB3\xCE\xB5\xCE\xB9\xCE\xB1 "
	"\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 "
	"\xE4\xBD\xA0\xE5\xA5\xBD";
    const char* const subcontent = "\xC3\x98Y STRA\xC3\x9F" "E";

    const BufferString tempdir = FilePath::getTempDir();
    const FilePath dirfp( tempdir.buf(), dirnm );
    const FilePath subdirfp( dirfp, subdirnm );
    const FilePath filefp( dirfp, fnm );
    const FilePath subfilefp( subdirfp, subfnm );
    utf8dir_ = dirfp.fullPath();
    FilePath zipfp( dirfp );
    zipfp.setExtension( "zip" );
    utf8zipfnm_ = zipfp.fullPath();
    const BufferString filenm = filefp.fullPath();
    const BufferString subfilenm = subfilefp.fullPath();

    if ( File::exists(utf8dir_.buf()) )
    {
	mRunStandardTest( File::removeDir(utf8dir_.buf()),
			  "Remove existing UTF-8 directory" );
    }

    mRunStandardTest( File::createDir(subdirfp.fullPath()),
		      BufferString("Create UTF-8 directory: ",
				   subdirfp.fullPath()) );
    mRunStandardTest( File::putContent(BufferString(content),filenm.buf()) &&
		      File::putContent(BufferString(subcontent),
				       subfilenm.buf()),
		      "Write files with UTF-8 names and content" );

    const od_int64 oldtime = 1577880000;
    std::timespec oldtimespec;
    oldtimespec.tv_sec = sCast(std::time_t,oldtime);
    oldtimespec.tv_nsec = 0;
    Time::FileTimeSet oldtimes;
    oldtimes.setModificationTime( oldtimespec );
    mRunStandardTestWithError(
		File::setTimes(filenm.buf(),oldtimes,false) &&
		File::setTimes(subfilenm.buf(),oldtimes,false) &&
		File::getTimeInSeconds(filenm.buf())==oldtime &&
		File::getTimeInSeconds(subfilenm.buf())==oldtime,
		"Set an old modification time on the UTF-8 files",
		BufferString("Got ",File::getTimeInSeconds(filenm.buf()),
			     ", expected ").add(oldtime) );
    mRunStandardTest( File::setReadOnly(subfilenm.buf(),false) &&
		      !File::isWritable(subfilenm.buf()),
		      "Make a UTF-8 file read-only before zipping" );

    FileState filestate, subfilestate;
    mRunStandardTest( getFileState(filenm.buf(),filestate) &&
		      getFileState(subfilenm.buf(),subfilestate),
		      "Size, permissions and time of the UTF-8 files" );

    uiString err;
    mRunStandardTestWithError(
	ZipUtils::makeZip(utf8dir_.buf(),tempdir.buf(),utf8zipfnm_.buf(),err),
	"Zip archive with UTF-8 names", toString(err) );

    BufferStringSet entries;
    mRunStandardTestWithError(
	ZipUtils::makeFileList(utf8zipfnm_.buf(),entries,err),
	"List the UTF-8 archive", toString(err) );
    mRunStandardTest( hasEntry(entries,fnm) && hasEntry(entries,subfnm),
		      BufferString("UTF-8 names in the archive: ",
				   entries.cat(", ")) );

    mRunStandardTest( File::setWritable(subfilenm.buf(),true,false) &&
		      File::removeDir(utf8dir_.buf()),
		      "Remove UTF-8 directory before unzipping" );
    mRunStandardTestWithError(
	ZipUtils::unZipArchive(utf8zipfnm_.buf(),tempdir.buf(),err),
	"Unzip archive with UTF-8 names", toString(err) );

    if ( !checkUnzippedFile(filenm.buf(),filestate,fnm) ||
	 !checkUnzippedFile(subfilenm.buf(),subfilestate,subfnm) )
	return false;

    mRunStandardTest( !File::isWritable(subfilenm.buf()),
		      "Unzipped UTF-8 file is still read-only" );
    mRunStandardTest( File::setWritable(subfilenm.buf(),true,false),
		      "Make the unzipped UTF-8 file writable again" );

    BufferString recovered;
    mRunStandardTest( File::getContent(filenm.buf(),recovered) &&
		      recovered == content,
		      BufferString("Unzipped UTF-8 content: ",recovered) );
    recovered.setEmpty();
    mRunStandardTest( File::getContent(subfilenm.buf(),recovered) &&
		      recovered == subcontent,
		      BufferString("Unzipped UTF-8 content in subdirectory: ",
				   recovered) );
    mRunStandardTest( hasUtf8NamesFlag(utf8zipfnm_.buf()),
		      "Archive marks its names as UTF-8 (bit 11)" );

    mRunStandardTest( File::remove(utf8zipfnm_.buf()) &&
		      File::removeDir(utf8dir_.buf()),
		      "Remove UTF-8 archive and directory" );

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

    if ( !testUtf8Names() )
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
