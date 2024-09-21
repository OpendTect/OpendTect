/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "file.h"
#include "filepath.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "testprog.h"
#include "timefun.h"
#include "winutils.h"

#ifdef __unix__
# include <sys/stat.h>
#endif


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


static bool testEmptyReadContent( const BufferString& tempfile )
{
    od_ostream stream( tempfile );
    stream.close();
    BufferString buf;
    mRunStandardTest(File::getContent(tempfile,buf),"Empty file read")
    mRunStandardTest(buf.isEmpty(),"empty file: No data to be read");
    return true;
}


static bool testNonEmptyReadContent( const BufferString& tempfile )
{
    BufferString buf;
    od_ostream stream( tempfile );
    stream << "test text";
    stream.close();
    mRunStandardTest(File::getContent(tempfile,buf),"Non-empty file read")
    mRunStandardTest(buf.size(),"Valid data read");
    return true;
}


static bool testReadContent()
{
    //Read non existent file - should fail.
    const BufferString tempfile = FilePath::getTempFullPath( "test", "txt" );
    mRunStandardTest(!tempfile.isEmpty(),"Temp filepath created")
    BufferString buf;
    mRunStandardTest((!File::getContent(tempfile,buf) && buf.isEmpty()),
		      "Non-existent file read");

    //Create empty file
    //Read empty file - should work fine.
    if ( !testEmptyReadContent(tempfile) )
    {
	File::remove( tempfile );
	return false;
    }

    //Read non empty file - should work fine.
    if ( !testNonEmptyReadContent(tempfile) )
    {
	File::remove( tempfile );
	return false;
    }

    mRunStandardTest(File::remove(tempfile),"Remove temporary file")
    mRunStandardTest(!File::exists(tempfile),"Temp file removed");

    return true;
}


static bool testIStream( const char* file )
{
    od_istream invalidstream( "IUOIUOUOF");
    mRunStandardTest( !invalidstream.isOK(), "isOK on open non-existing file" );

    od_istream stream( file );
    mRunStandardTest( stream.isOK(), "isOK on open existing file" );

    int i;
    stream.get(i);
    mRunStandardTest( i==1 && stream.isOK(),
		      "Reading positive integer to int" );

    stream.get(i);
    mRunStandardTest( i==-1 && stream.isOK(),
		      "Reading negative integer to int" );

    stream.get(i);
    mRunStandardTest( stream.isOK(), "Reading float into integer" );

    return true;
}


static bool testFilePath( const char* inputpath,
			  const char* prefix, const char* domain,
			  const char* expfullpath,
			  const char* pathonly,
			  const char* localpath,
			  const char* basename,
			  const char* filename,
			  const char* extension,
			  const char* postfix,
			  const char* modifiedfullpath,
			  int nrlevels, bool absolute, bool isuri )
{
    const FilePath path( inputpath );
    const StringView expfp( expfullpath ? expfullpath : inputpath );
    const FilePath::Style st = path.isURI()
			     ? FilePath::Local
			     : ( StringView(path.prefix()) == "C"
				? FilePath::Windows : FilePath::Unix );

    mRunStandardTest( path.isAbsolute()==absolute,
	    BufferString( inputpath, " detects absolute status" ) );

    mRunStandardTest( path.isURI()==isuri,
	BufferString( inputpath, " detects URI status" ) );

    mRunStandardTest( StringView(path.prefix())==prefix,
	BufferString( inputpath, " prefix: ", path.prefix() ) );

    mRunStandardTest( StringView(path.domain())==domain,
	BufferString( inputpath, " domain: ", path.domain() ) );

    mRunStandardTest( path.fullPath(st)==expfp,
	BufferString( inputpath, " fullpath: ", path.fullPath(st) ) );

    mRunStandardTest( path.pathOnly(st)==pathonly,
	BufferString( inputpath, " path: ", path.pathOnly(st) ) );

    mRunStandardTest( path.fileFrom(0,st)==localpath,
	BufferString( inputpath, " local path: ", path.fileFrom(0,st) ) );

    mRunStandardTest( path.baseName() == basename,
	BufferString( inputpath, " basename: ", path.baseName() ) );

    mRunStandardTest( path.fileName()==filename,
	    BufferString( inputpath, " filename: ", path.fileName() ) );

    mRunStandardTest( StringView(path.extension())==extension,
	    BufferString( inputpath, " extension: ", path.extension() ) );

    mRunStandardTest( StringView(path.postfix())==postfix,
	    BufferString( inputpath, " postfix: ", path.postfix() ) );

    mRunStandardTest( path.nrLevels()==nrlevels,
	    BufferString( inputpath, " nrLevels: ", toString(path.nrLevels())));

    FilePath newpath( path );
    newpath.insert( "inserted/folder" );
    mRunStandardTest( newpath.fullPath( st )==modifiedfullpath,
	BufferString( modifiedfullpath, " extended path: ",
		      newpath.fullPath(st) ) );
    mRunStandardTest( newpath.isURI() == path.isURI() &&
		      newpath.isAbsolute() == path.isAbsolute(),
		      BufferString(inputpath,": Paths types remain identical"));

    newpath = StringView(postfix).isEmpty() ? FilePath(filename)
			    : FilePath(BufferString(filename, "?", postfix));
    newpath.insert( pathonly );
    mRunStandardTest( newpath.fullPath(st)==expfp, BufferString(
				"Expected: ", expfp,
				" got: " ).add(newpath.fullPath(st)) );
    mRunStandardTest( newpath.isURI() == path.isURI() &&
		      newpath.isAbsolute() == path.isAbsolute(),
		      BufferString(inputpath,": Paths types remain identical"));

    return true;
}


static bool testFilePathParsing()
{
    if ( !testFilePath( "C:\\path\\to\\me.txt",
			"C", "",	//prefix/domain
			nullptr,	//same fullpath
			"C:\\path\\to", //path
			"path\\to\\me.txt", //local
			"me",		//basename
			"me.txt",	//filename
			"txt",		//extension
			"",		//postfix
			"C:\\inserted\\folder\\path\\to\\me.txt",
			3,		//nrlevels
			true,false) )	//absolute/uri
    {
	return false;
    }

    if ( !testFilePath( "/data/apps/OpendTect 5.0.0/file.txt",
			"", "",		//prefix/domain
			nullptr,	//same fullpath
			"/data/apps/OpendTect 5.0.0", //path
			"data/apps/OpendTect 5.0.0/file.txt", //local
			"file",		//basename
			"file.txt",	//filename
			"txt",		//extension
			"",		//postfix
			"/inserted/folder/data/apps/OpendTect 5.0.0/file.txt",
			4,		//nrlevels
			true,false) )	//absolute/uri
    {
	return false;
    }

    if ( !testFilePath( "C:\\Program Files/OpendTect\\5.0.0/file.txt",
			"C", "",	//prefix/domain
			"C:\\Program Files\\OpendTect\\5.0.0\\file.txt", //full
			"C:\\Program Files\\OpendTect\\5.0.0", //path
			"Program Files\\OpendTect\\5.0.0\\file.txt", //local
			"file",		//basename
			"file.txt",	//filename
			"txt",		//extension
			"",		//postfix
	"C:\\inserted\\folder\\Program Files\\OpendTect\\5.0.0\\file.txt",
			4,		//nrlevels
			true,false) )	//absolute/uri
    {
	return false;
    }

    if ( !testFilePath( "https://dgbes.com/surveys/aap/noot?x=y&&a=b",
			"https", "dgbes.com", //prefix/domain
			nullptr,	//same fullpath
			"https://dgbes.com/surveys/aap", //path
			"surveys/aap/noot", //local
			"noot",		//basename
			"noot",		//filename
			"",		//extension
			"x=y&&a=b",	//postfix
		"https://dgbes.com/inserted/folder/surveys/aap/noot?x=y&&a=b",
			3,		//nrlevels
			true,true) )	//absolute/uri
    {
	return false;
    }

    if ( !testFilePath(
       "https://dgbes.amazon.com/surveys/F3 Demo/Seismics/median_filtered.cbvs",
			"https", "dgbes.amazon.com", //prefix/domain
			nullptr,		//same fullpath
       "https://dgbes.amazon.com/surveys/F3 Demo/Seismics", //path
			"surveys/F3 Demo/Seismics/median_filtered.cbvs",//local
			"median_filtered",	//basename
			"median_filtered.cbvs",	//filename
			"cbvs",		//extension
			"",		//postfix
			"https://dgbes.amazon.com/inserted/folder/"
			"surveys/F3 Demo/Seismics/median_filtered.cbvs",
			4,		//nrlevels
			true,true) )	//absolute/uri
    {
	return false;
    }

    if ( !testFilePath( "s3://dgb-test-bucket/seismics/median_filtered.cbvs",
			"s3", "dgb-test-bucket", //prefix/domain
			nullptr,	//same fullpath
			"s3://dgb-test-bucket/seismics", //path
			"seismics/median_filtered.cbvs", //local
			"median_filtered", //basename
			"median_filtered.cbvs", //filename
			"cbvs",		//extension
			"",		//postfix
			"s3://dgb-test-bucket/inserted/folder/"
			"seismics/median_filtered.cbvs",
			2,		//nrlevels
			true,true) )	//absolute/uri
    {
	return false;
    }

    return true;
}


static bool testCleanPath()
{
    const BufferString uri = "s3://od-awsplugin/vdsdata/VDS_small";
    const BufferString uriunix = getCleanUnixPath( uri.buf() );
    const BufferString uriwin = getCleanWinPath( uri.buf() );
    mRunStandardTest( uriunix==uri, "getCleanUnixPath" );
    mRunStandardTest( uriwin==uri, "getCleanWinPath" );

    const BufferString unixpath = "RawData/file.vds";
    const BufferString winpath = "RawData\\file.vds";
    const BufferString unixpath2win = getCleanWinPath( unixpath.buf() );
    const BufferString winpath2unix = getCleanUnixPath( winpath.buf() );
    mRunStandardTest( unixpath==winpath2unix, "Windows style path to Unix" );
    mRunStandardTest( winpath==unixpath2win, "Unix style path to Windows" );

    const BufferString cwinpath =
		"C:\\Program Files\\OpendTect\\7.0.0\\INSTALL.txt";
    const BufferString cunixpath =
		FilePath(cwinpath.buf()).fullPath( FilePath::Unix );
    const BufferString cunixpathres =
		"C:/Program Files/OpendTect/7.0.0/INSTALL.txt";
    mRunStandardTest( cunixpath==cunixpathres, "Windows path with Unix delim" );

    FilePath fp( cunixpathres );
    mRunStandardTest( cwinpath==fp.fullPath(FilePath::Windows),
		"From Windows path with Unix delim back to Windows delim" );
    return true;
}


static bool testFileReadWrite()
{
    const BufferString filenm =
		FilePath::getTempFullPath( "test with space", "txt" );
    const char* fnm = filenm.str();
    mRunStandardTest( !File::exists(fnm), "Temporary file does not exist" );
    od_ostream strm( fnm );
    strm << "some content";
    mRunStandardTestWithError( strm.isOK(), "test file is created",
			       strm.errMsg().getString() );
    if ( __iswin__ )
	mRunStandardTest( File::isInUse(fnm), "File is being used" );

    strm.close();
    FileDisposer disposer( fnm );

    if ( __iswin__ )
	mRunStandardTest( !File::isInUse(fnm), "File is not being used" );

    mRunStandardTest( File::exists(fnm), "File exists" );
    mRunStandardTest( File::isReadable(fnm), "File is readable" );
    mRunStandardTest( File::setReadOnly(fnm), "File set read-only" );
    mRunStandardTest( !File::isWritable(fnm), "File is read-only" );
    mRunStandardTest( File::setWritable(fnm,true), "File set writable" );
    mRunStandardTest( File::isWritable(fnm), "File is writable" );

    if ( __iswin__ )
    {
	bool mUnusedVar res = File::isWritable( "C:\\temp" );
	res = File::isWritable("C:\\Program Files");
	res = File::isWritable("C:\\Program Files\\OpendTect");
	res = File::isWritable("C:\\Program Files\\OpendTect\\7.0.0");
	res = File::isWritable("C:\\Program Files\\OpendTect\\7.0.0\\bla");
	res = File::isWritable(
	    "C:\\Program Files\\OpendTect\\7.0.0\\relinfo\\ver.base_win64.txt");
	res = File::isWritable("D:\\ODData");
	res = File::isWritable("E:\\surveys");
	res = File::isWritable("F:");
    }

    mRunStandardTest( File::remove(fnm), "Remove file" );
    mRunStandardTest( !File::exists(fnm), "File is removed" );

    return true;
}


static bool isEqual( std::timespec a, std::timespec b )
{
    return a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec;
}


static bool isLarger( std::timespec a, std::timespec b )
{
    if ( a.tv_sec > b.tv_sec )
	return true;

    if ( a.tv_sec < b.tv_sec )
	return false;

    return a.tv_nsec > b.tv_nsec;
}


static bool isEqual( const Time::FileTimeSet& a,
		     const Time::FileTimeSet& b )
{
    if ( !isEqual(a.getModificationTime(),b.getModificationTime()) ||
	 !isEqual(a.getCreationTime(),b.getCreationTime()) )
	return false;

    return __iswin__ ? true
		     : isEqual( a.getAccessTime(), b.getAccessTime() );
}


static bool testFileTime( const char* fnm )
{
    const od_int64 timesec = File::getTimeInSeconds( fnm );
    mRunStandardTest( !mIsUdf(timesec) && timesec > 0,
		      "File time in seconds from path" );
    const od_int64 timeinms = File::getTimeInMilliSeconds( fnm );
    mRunStandardTest( !mIsUdf(timeinms) && timeinms > 0,
		      "File time in milliseconds from path");
    BufferString crtimestr = File::timeCreated( fnm );
    mRunStandardTest( !crtimestr.isEmpty() && crtimestr != "-",
		      "File time created string from path" );
    BufferString modtimestr = File::timeLastModified( fnm );
    mRunStandardTest( !modtimestr.isEmpty() && modtimestr != "-",
		      "File time last modified string from path" );

    Time::FileTimeSet times;
    mRunStandardTest( File::getTimes( fnm, times ),
		      "Get all file timestamps from path" );

    modtimestr.set( Time::getDateTimeString(timeinms) );
    mRunStandardTest( !modtimestr.isEmpty() && modtimestr != "-",
		      "File time last modified string from od_int64" );

    modtimestr.set( Time::getDateTimeString(times.getModificationTime()) );
    mRunStandardTest( !modtimestr.isEmpty() && modtimestr != "-",
		      "File time last modified string from std::timespec" );
    const BufferString acctimestr(
		Time::getDateTimeString(times.getAccessTime()) );
    mRunStandardTest( !acctimestr.isEmpty() && acctimestr != "-",
		      "File time access string from std::timespec" );
    crtimestr.set( Time::getDateTimeString(times.getCreationTime()) );
    mRunStandardTest( !crtimestr.isEmpty() && crtimestr != "-",
		      "File time creation string from std::timespec" );

    FilePath fp( fnm );
    fp.makeCanonical();
    const BufferString realfnm = fp.fullPath();
    const BufferString linknm =
		FilePath::getTempFullPath( "test_file",
			__iswin__ ? "lnk" : fp.extension() );
    FileDisposer disposer1( linknm.buf() );
    mRunStandardTest( File::createLink(realfnm,linknm.buf()),
		      "Created symbolic link" );
    const BufferString linkend = File::linkEnd( linknm.buf() );
    mRunStandardTest( linkend == realfnm,
		      "Retrieve absolule path to a symbolic link" );

    const od_int64 filesz = File::getFileSize( realfnm );
    mRunStandardTest( File::getFileSize(linknm.buf()) == filesz,
		      "File size by following a link" );
    if ( !__iswin__ )
    {
	mRunStandardTest(
		File::getFileSize(linknm.buf(),false) == realfnm.size(),
			 "File size of a symbolic link" );
    }

    Time::FileTimeSet filetimes, linktimes;
    mRunStandardTest( File::getTimes( linknm.buf(), filetimes ),
		      "Get all file timestamps from a link target" );
    mRunStandardTest( File::getTimes( linknm.buf(), linktimes, false ),
		      "Get all file timestamps from a symbolic link" );
    mRunStandardTest( isEqual( filetimes, times ),
		      "File timestamps by following a link" );
    mRunStandardTest( !isEqual( linktimes, times ) &&
	isLarger( linktimes.getCreationTime(), times.getCreationTime() ) &&
	isLarger( linktimes.getModificationTime(),
		  times.getModificationTime() ) &&
	isLarger( linktimes.getAccessTime(), times.getAccessTime() ),
		      "File timestamps of a symbolic link" );
    std::timespec modtime = linktimes.getModificationTime();
    od_int64 timens = modtime.tv_nsec + 5e7; //50ms
    if ( timens > 999999999ULL )
    {
	modtime.tv_sec += 1;
	modtime.tv_nsec = long (timens) - 1e9;
    }
    else
	modtime.tv_nsec = long (timens);

    Time::FileTimeSet linktimesedit( linktimes );
    linktimesedit.setModificationTime( modtime );
    Threads::sleep( 0.1 ); //100ms
    mRunStandardTest( File::setTimes( linknm.buf(), linktimesedit, false ),
		      "Set the modification time of a symbolic link" );
    Threads::sleep( 0.1 ); //100ms
    Time::FileTimeSet linktimesret;
    mRunStandardTest( File::getTimes( linknm.buf(), linktimesret, false ),
		      "Get all file timestamps from a symbolic link" );
    mRunStandardTest( isEqual( linktimesret.getModificationTime(),
			       linktimesedit.getModificationTime() ) &&
		      !isEqual( linktimesret.getModificationTime(),
				linktimes.getModificationTime() ) &&
	isLarger( linktimesret.getModificationTime(),
		  linktimes.getModificationTime() ),
		      "File timestamps of a modified symbolic link" );

    uiString msg;
    const BufferString linkcpnm =
		FilePath::getTempFullPath( "test_file",
				    __iswin__ ? "lnk" : fp.extension() );
    FileDisposer disposer2( linkcpnm );
    mRunStandardTestWithError(
	    File::copy( linknm.buf(), linkcpnm.buf(), true, &msg ) &&
	    File::exists(linkcpnm.buf()) && File::isSymLink(linkcpnm),
	    "Copying a link as a link", msg.getString().buf() );

    msg.setEmpty();
    const BufferString linkdeepcpnm =
		FilePath::getTempFullPath( "test_file", fp.extension() );
    FileDisposer disposer3( linkdeepcpnm );
    mRunStandardTestWithError(
	    File::copy( linknm.buf(), linkdeepcpnm.buf(), false, &msg )
		 && File::exists(linkdeepcpnm.buf()) &&
		      !File::isSymLink(linkdeepcpnm),
		      "Deep copy from a link", msg.getString().buf() );
    const bool isdir = File::isDirectory( fnm );
    if ( isdir )
    {
	mRunStandardTest( File::isDirectory(linkdeepcpnm.buf()),
			  "Deep copied link of a directory" );
    }
    else
    {
	mRunStandardTest( File::isFile(linkdeepcpnm.buf()),
			  "Deep copied link of a file" );
    }

    if ( !isdir )
	return true;

    mRunStandardTest( File::remove( linkcpnm ) &&
		      File::removeDir( linkdeepcpnm ),
		      "Removed temporary files" );
    msg.setEmpty();
    mRunStandardTestWithError(
	    File::copyDir( linknm.buf(), linkcpnm.buf(), true, &msg ) &&
	    File::exists(linkcpnm.buf()) && File::isSymLink(linkcpnm),
	    "Copying a link to a directory as a link", msg.getString().buf() );

    msg.setEmpty();
    mRunStandardTestWithError(
	    File::copyDir( linknm.buf(), linkdeepcpnm.buf(), false, &msg ) &&
	    File::exists( linkdeepcpnm.buf() ) &&
	    !File::isSymLink( linkdeepcpnm ) &&
	    File::isDirectory( linkdeepcpnm.buf() ),
	      "Deep copy from a link to a directory", msg.getString().buf() );

    return true;
}


static bool testPerms( const char* fnm, const File::Permissions& expperms )
{
    const File::Permissions perms = File::getPermissions( fnm );
    if ( perms != expperms )
	return false;

#ifdef __win__
    return true;
#else
    struct stat filestat;
    lstat( fnm, &filestat );
    const int res = File::Permissions::get_st_mode( fnm );
    return res == filestat.st_mode;
#endif
}


static bool testFilePermissions()
{
    const auto unixfileperms = File::Permissions::getDefault( true, false );
    const auto unixdirperms = File::Permissions::getDefault( false, false );
    const auto winfileperms = File::Permissions::getDefault( true, true );
    const auto windirperms = File::Permissions::getDefault( false, true );
    mRunStandardTest( unixfileperms.asInt() == 26214 ||
		      unixfileperms.asInt() == 26212 ,
		      "default Unix file permissions" );
    mRunStandardTest( unixdirperms.asInt() == 30583 ||
		      unixdirperms.asInt() == 30581 ,
		      "default Unix directory permissions" );
    mRunStandardTest( winfileperms.asInt() == 32,
		      "default Windows file permissions" );
    mRunStandardTest( windirperms.asInt() == 16,
		      "default Windows directory permissions" );

    const BufferString filename =
		FilePath::getTempFullPath( "test_file", "txt" );
    const char* fnm = filename.str();
    FileDisposer disposer( fnm );
    od_ostream strm( fnm );
    strm.add( "some text" );
    mRunStandardTestWithError( strm.isOK(), "test file is created",
			       strm.errMsg().getString() );
    strm.close();

#ifdef __win__
    const File::Permissions perms = File::Permissions::getDefault( true );
    if ( !(perms.asInt() & FILE_ATTRIBUTE_ARCHIVE) )
	return false;

    mRunStandardTest( testPerms(fnm,perms), "Standard file permissions" );

    File::Permissions permstest( perms );
    permstest.setHidden( true );
    mRunStandardTest( File::setHiddenFileAttrib(fnm,true) &&
		      File::isHidden(fnm) &&
		      permstest.isHidden() &&
		      testPerms(fnm,permstest),
		      "Hidden file permissions (set)" );

    permstest.setHidden( false );
    mRunStandardTest( File::setHiddenFileAttrib(fnm,false) &&
		      !File::isHidden(fnm) &&
		      !permstest.isHidden() &&
		      testPerms(fnm,permstest),
		      "Hidden file permissions (unset)" );

    permstest.setSystem( true );
    mRunStandardTest( File::setSystemFileAttrib(fnm,true) &&
		      File::isSystem(fnm) &&
		      permstest.isSystem() &&
		      testPerms(fnm,permstest),
		      "System file permissions (set)" );

    permstest.setSystem( false );
    mRunStandardTest( File::setSystemFileAttrib(fnm,false) &&
		      !File::isSystem(fnm) &&
		      !permstest.isSystem() &&
		      testPerms(fnm,permstest),
		      "System file permissions (unset)" );

    return true;
#else
    struct stat filestat;
    lstat( fnm, &filestat );
    const File::Permissions stdperms =
		File::Permissions::getFrom( filestat.st_mode, filestat.st_uid );
    const File::Permissions perms = File::getPermissions( fnm );
    mRunStandardTest( perms == stdperms, "Retrieve default file permissions" );
    const bool hasgroupmask = perms.testFlag( File::Permission::WriteGroup );
    const bool hasothermask = perms.testFlag( File::Permission::WriteOther );

    File::Permissions permstest = File::Permissions::getDefault( true );
    permstest.setFlag( File::Permission::WriteOwner, false )
	     .setFlag( File::Permission::WriteUser, false )
	     .setFlag( File::Permission::WriteGroup, false )
	     .setFlag( File::Permission::WriteOther, false );
    mRunStandardTest( File::setReadOnly(fnm) && testPerms(fnm,permstest),
		      "Read-only file permissions" );
    permstest.setFlag( File::Permission::WriteOwner, true )
	     .setFlag( File::Permission::WriteUser, true );
    if ( hasgroupmask )
	permstest.setFlag( File::Permission::WriteGroup, true );
    if ( hasothermask )
	permstest.setFlag( File::Permission::WriteOther, true );

    mRunStandardTest( File::setWritable(fnm,true) && testPerms(fnm,permstest),
		      "Writable file permissions" );

    permstest.setFlag( File::Permission::ExeOwner, true )
	     .setFlag( File::Permission::ExeUser, true )
	     .setFlag( File::Permission::ExeGroup, true )
	     .setFlag( File::Permission::ExeOther, true );

    mRunStandardTest( File::setExecutable(fnm,true) &&
		      File::isExecutable(fnm) &&
		      testPerms(fnm,permstest),
		      "Executable file permissions (set)" );

    permstest.setFlag( File::Permission::ExeOwner, false )
	     .setFlag( File::Permission::ExeUser, false )
	     .setFlag( File::Permission::ExeGroup, false )
	     .setFlag( File::Permission::ExeOther, false );

    mRunStandardTest( File::setExecutable(fnm,false) &&
		      !File::isExecutable(fnm) &&
		      testPerms(fnm,permstest),
		      "Executable file permissions (unset)" );
    return true;
#endif
}


static bool testDirPermissions()
{
#ifdef __win__
    const BufferString dirname = FilePath::getTempFullPath( "test_dir",nullptr);
    mRunStandardTest( File::createDir(dirname.buf()),
		      "Create new directory" );
    const char* dirnm = dirname.str();
    FileDisposer disposer( dirnm );

    const File::Permissions perms = File::Permissions::getDefault( false );
    if ( !(perms.asInt() & FILE_ATTRIBUTE_DIRECTORY) )
	return false;

    mRunStandardTest( testPerms(dirnm,perms), "Standard directory permissions");

    File::Permissions permstest( perms );
    permstest.setHidden( true );
    mRunStandardTest( File::setHiddenFileAttrib(dirnm,true) &&
		      File::isHidden(dirnm) &&
		      permstest.isHidden() &&
		      testPerms(dirnm,permstest),
		      "Hidden directory permissions (set)" );

    permstest.setHidden( false );
    mRunStandardTest( File::setHiddenFileAttrib(dirnm,false) &&
		      !File::isHidden(dirnm) &&
		      !permstest.isHidden() &&
		      testPerms(dirnm,permstest),
		      "Hidden directory permissions (unset)" );

    permstest.setSystem( true );
    mRunStandardTest( File::setSystemFileAttrib(dirnm,true) &&
		      File::isSystem(dirnm) &&
		      permstest.isSystem() &&
		      testPerms(dirnm,permstest),
		      "System directory permissions (set)" );

    permstest.setSystem( false );
    mRunStandardTest( File::setSystemFileAttrib(dirnm,false) &&
		      !File::isSystem(dirnm) &&
		      !permstest.isSystem() &&
		      testPerms(dirnm,permstest),
		      "System directory permissions (unset)" );
#endif
    return true;

}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    FilePath fp( __FILE__ );
    fp.setExtension( "txt" );
    if ( !fp.exists() )
    {
	fp.set( GetSoftwareDir(false) ).add( __FILE__ ).setExtension( "txt" );
	if ( !fp.exists() )
	{
	    errStream() << "Input file not found\n";
	    return 1;
	}
    }

    const BufferString parfile( fp.fullPath() );
    const BufferString pardir( fp.pathOnly() );
    if ( !testReadContent() ||
	 !testIStream(parfile.buf()) ||
	 !testFilePathParsing() ||
	 !testCleanPath() ||
	 !testFileReadWrite() ||
	 !testFileTime(parfile.buf()) ||
	 !testFileTime(pardir.buf()) ||
	 !testFilePermissions() ||
	 !testDirPermissions() )
	return 1;

    return 0;
}
