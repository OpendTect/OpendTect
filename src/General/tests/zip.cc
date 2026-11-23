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
static BufferString asciidir_;
static BufferString asciizipfnm_;
static BufferString mixedzipfnm_;


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

    if ( File::exists(asciidir_.buf()) )
	File::removeDir( asciidir_.buf() );

    if ( File::exists(asciizipfnm_.buf()) )
	File::remove( asciizipfnm_.buf() );

    if ( File::exists(mixedzipfnm_.buf()) )
	File::remove( mixedzipfnm_.buf() );

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


struct ZipEntryFlags
{
    BufferString	name_;
    od_uint16		centralflag_	= 0;
    od_uint16		localflag_	= 0;
};


static od_uint32 getLittleEndian( const unsigned char* buf, int nrbytes )
{
    od_uint32 ret = 0;
    for ( int idx=nrbytes-1; idx>=0; idx-- )
	ret = (ret << 8) | buf[idx];

    return ret;
}


static bool hasNonAsciiChars( const char* str )
{
    for ( const unsigned char* ptr = rCast(const unsigned char*,str);
	  *ptr; ptr++ )
    {
	if ( *ptr > 127 )
	    return true;
    }

    return false;
}


static bool readEntries( od_istream& strm, od_int64 zipsz,
			 ManagedObjectSet<ZipEntryFlags>& entries )
{
    const int tailsz = sCast( int, mMIN(zipsz,65557) );
    TypeSet<unsigned char> tail( tailsz, 0 );
    strm.setReadPosition( zipsz-tailsz );
    if ( !strm.getBin(tail.arr(),tailsz) )
	return false;

    int eocd = tailsz - 22;
    while ( eocd >= 0 && getLittleEndian(tail.arr()+eocd,4) != 0x06054b50 )
	eocd--;

    if ( eocd < 0 )
	return false;

    const int nrentries = sCast( int, getLittleEndian(tail.arr()+eocd+10,2) );
    const int cdsize = sCast( int, getLittleEndian(tail.arr()+eocd+12,4) );
    TypeSet<unsigned char> cd( cdsize, 0 );
    strm.setReadPosition( getLittleEndian(tail.arr()+eocd+16,4) );
    if ( !strm.getBin(cd.arr(),cdsize) )
	return false;

    int pos = 0;
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const unsigned char* hdr = cd.arr() + pos;
	if ( pos+46 > cdsize || getLittleEndian(hdr,4) != 0x02014b50 )
	    return false;

	const int namelen = sCast( int, getLittleEndian(hdr+28,2) );
	unsigned char localhdr[8];
	strm.setReadPosition( getLittleEndian(hdr+42,4) );
	if ( !strm.getBin(localhdr,8) ||
	     getLittleEndian(localhdr,4) != 0x04034b50 )
	    return false;

	auto* entry = new ZipEntryFlags;
	entry->centralflag_ = sCast( od_uint16, getLittleEndian(hdr+8,2) );
	entry->localflag_ = sCast( od_uint16, getLittleEndian(localhdr+6,2) );
	for ( int ich=0; ich<namelen; ich++ )
	    entry->name_.add( sCast(char,hdr[46+ich]) );

	entries += entry;
	pos += 46 + namelen + sCast(int,getLittleEndian(hdr+30,2)) +
	       sCast(int,getLittleEndian(hdr+32,2));
    }

    return true;
}


static bool readEntryFlags( const char* zipfnm,
			    ManagedObjectSet<ZipEntryFlags>& entries )
{
    const od_int64 zipsz = File::getFileSize( zipfnm );
    if ( zipsz < 22 )
	return false;

    od_istream strm( zipfnm );
    const bool isread = strm.isOK() && readEntries( strm, zipsz, entries );
    strm.close();
    return isread;
}


static bool checkEntryFlags( const char* zipfnm, const char* desc,
			     bool& hasutf8, bool& hasascii )
{
    ManagedObjectSet<ZipEntryFlags> entries;
    mRunStandardTest( readEntryFlags(zipfnm,entries) && !entries.isEmpty(),
		      BufferString("Read the headers of all entries: ",desc) );

    hasutf8 = hasascii = false;
    BufferString wrongmark, mismatch, unreadable;
    for ( const auto* entry : entries )
    {
	const bool nonascii = hasNonAsciiChars( entry->name_.buf() );
	if ( nonascii )
	    hasutf8 = true;
	else
	    hasascii = true;

	const bool marked = entry->centralflag_ & 0x0800;
	if ( marked != nonascii )
	    wrongmark.add( entry->name_ ).add( " " );

	if ( entry->centralflag_ != entry->localflag_ )
	    mismatch.add( entry->name_ ).add( " " );

	if ( !nonascii && entry->localflag_ > 14 )
	    unreadable.add( entry->name_ ).add( " " );
    }

    mRunStandardTestWithError(
		wrongmark.isEmpty(),
		BufferString("UTF-8 mark only on non-ASCII names: ",desc),
		wrongmark );
    mRunStandardTestWithError(
		mismatch.isEmpty(),
		BufferString("Local and central headers agree: ",desc),
		mismatch );
    mRunStandardTestWithError(
		unreadable.isEmpty(),
		BufferString("ASCII entries readable by older versions: ",desc),
		unreadable );
    return true;
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
    mRunStandardTestWithError(
		after.perms_ == before.perms_,
		BufferString("Unzipped file permissions: ",desc),
		BufferString("Got ",after.perms_.asInt(),", expected ")
		    .add(before.perms_.asInt()) );
    mRunStandardTestWithError(
		Math::Abs(after.modtime_-before.modtime_) <= 2,
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
    bool hasutf8 = false, hasascii = false;
    if ( !checkEntryFlags(utf8zipfnm_.buf(),"UTF-8 archive",hasutf8,hasascii) )
	return false;

    mRunStandardTest( hasutf8 && !hasascii,
		      "The UTF-8 archive has only non-ASCII names" );

    const FilePath asciidirfp( tempdir.buf(), "od_test_zip_ascii" );
    asciidir_ = asciidirfp.fullPath();
    asciizipfnm_ =
		FilePath( tempdir.buf(), "od_test_zip_ascii.zip" ).fullPath();
    mixedzipfnm_ =
		FilePath( tempdir.buf(), "od_test_zip_mixed.zip" ).fullPath();
    const FilePath asciifilefp( asciidirfp, "ascii_only.txt" );
    mRunStandardTest( File::createDir(asciidir_.buf()) &&
		      File::putContent(BufferString("ascii"),
				       asciifilefp.fullPath()),
		      "Write a file with an ASCII name" );

    mRunStandardTestWithError(
		ZipUtils::makeZip(asciidir_.buf(),tempdir.buf(),
				  asciizipfnm_.buf(),err),
		"Zip archive with only ASCII names", toString(err) );
    if ( !checkEntryFlags(asciizipfnm_.buf(),"ASCII archive",
			  hasutf8,hasascii) )
	return false;

    mRunStandardTest( !hasutf8 && hasascii,
		      "The ASCII archive has no non-ASCII names" );

    BufferStringSet mixedsrcs;
    mixedsrcs.add( asciidir_ ).add( utf8dir_ );
    mRunStandardTestWithError(
		ZipUtils::makeZip(mixedsrcs,tempdir.buf(),
				  mixedzipfnm_.buf(),err),
		"Zip archive with ASCII and UTF-8 names", toString(err) );
    if ( !checkEntryFlags(mixedzipfnm_.buf(),"mixed archive",
			  hasutf8,hasascii) )
	return false;

    mRunStandardTest( hasutf8 && hasascii,
		      "The mixed archive has ASCII and non-ASCII names" );

    mRunStandardTest( File::remove(utf8zipfnm_.buf()) &&
		      File::remove(asciizipfnm_.buf()) &&
		      File::remove(mixedzipfnm_.buf()) &&
		      File::removeDir(asciidir_.buf()) &&
		      File::removeDir(utf8dir_.buf()),
		      "Remove the archives and directories" );

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
    bool hasutf8 = false, hasascii = false;
    if ( !checkEntryFlags(zipfilename_.buf(),sKeySurvey,hasutf8,hasascii) )
	return 1;

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
