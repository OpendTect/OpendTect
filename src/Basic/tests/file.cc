/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "file.h"
#include "testprog.h"
#include "oddirs.h"
#include "filepath.h"
#include "od_istream.h"
#include "od_ostream.h"

#define mTest( testname, test ) \
if ( !(test) ) \
{ \
    handleTestResult( false, testname ); \
    return false; \
} \
else\
{ \
    handleTestResult( true, testname ); \
}

#define mRunTest( test ) \
mTest( #test, test )

bool testReadContent()
{
    BufferString basedir = GetSoftwareDir( 0 );

    BufferString buf;

    //Read non existent file - should fail.
    buf.setEmpty();
    FilePath nofile( basedir.buf(), "src", "Basic", "tests","NonExistingFile");
    mRunTest(!File::getContent(nofile.fullPath(),buf) && buf.isEmpty());

    //Create empty file

    //Read empty file - should work fine.
    buf.setEmpty();
    FilePath emptyfile( basedir.buf(), "emptyfile.txt");
    od_ostream stream(emptyfile.fullPath());
    stream.close();
    mRunTest(File::getContent(emptyfile.fullPath(),buf) && buf.isEmpty());

    File::remove( emptyfile.fullPath() );

    //Read non empty file - should work fine.
    buf.setEmpty();
    FilePath nonempty( basedir.buf(), "CMakeCache.txt" );
    mRunTest(File::getContent(nonempty.fullPath(),buf) && buf.size());

    return true;
}


bool testIStream( const char* file )
{
    od_istream invalidstream( "IUOIUOUOF");
    mTest( "isOK on open non-existing file", !invalidstream.isOK() );

    od_istream stream( file );
    mTest( "isOK on open existing file", stream.isOK() );


    int i;
    stream.get(i);
    mTest( "Reading positive integer to int",
            i==1 && stream.isOK() );

    stream.get(i);
    mTest( "Reading negative integer to int",
           i==-1 && stream.isOK() );

    stream.get(i);
    mTest( "Reading float into integer",
           stream.isOK() );

    return true;
}


bool testFilePath( const char* inputpath,
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

    return true;
}


bool testFilePathParsing()
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
    if ( !testReadContent() || !testIStream( parfile.buf() ) )
	return 1;

    if ( !testFilePathParsing() )
	return 1;

    return 0;
}
