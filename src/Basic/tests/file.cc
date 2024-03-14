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


bool testEmptyReadContent( const BufferString& tempfile )
{
    od_ostream stream( tempfile );
    stream.close();
    BufferString buf;
    mRunStandardTest(File::getContent(tempfile,buf),"Empty file read")
    mRunStandardTest(buf.isEmpty(),"empty file: No data to be read");
    return true;
}


bool testNonEmptyReadContent( const BufferString& tempfile )
{
    BufferString buf;
    od_ostream stream( tempfile );
    stream << "test text";
    stream.close();
    mRunStandardTest(File::getContent(tempfile,buf),"Non-empty file read")
    mRunStandardTest(buf.size(),"Valid data read");
    return true;
}


bool testReadContent()
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
		   const char* filename,
		   const char* domain,
		   const char* extension,
		   const char* postfix,
		   int nrlevels,
		   bool absolute )
{
    const FilePath path( inputpath );

    mRunStandardTest( path.isAbsolute()==absolute,
	    BufferString( inputpath, " detects absolute status" ) );

    mRunStandardTest( path.fileName()==filename,
	    BufferString( inputpath, " detects filename" ) );

    mRunStandardTest( StringView(path.domain())==domain,
	    BufferString( inputpath, " detects domain" ) );

    mRunStandardTest( StringView(path.extension())==extension,
	    BufferString( inputpath, " detects extension" ) );

    mRunStandardTest( StringView(path.postfix())==postfix,
	    BufferString( inputpath, " detects postfix" ) );

    mRunStandardTest( path.nrLevels()==nrlevels,
	    BufferString( inputpath, " detects nrLevels" ) );

    return true;
}


bool testFilePathParsing()
{
    if ( !testFilePath( "C:\\path\\to\\me.txt",
			"me.txt",	//filename
			"",		//domain
			"txt",		//extension
			"",		//postfix
			3,		//nrlevels
			true ))	//absolute
    {
	return false;
    }

    if ( !testFilePath( "/data/apps/OpendTect 5.0.0/file.txt",
			"file.txt",	//filename
			"",		//domain
			"txt",		//extension
			"",		//postfix
			4,		//nrlevels
			true ))	//absolute
    {
	return false;
    }

    if ( !testFilePath( "C:\\Program Files/OpendTect\\5.0.0/file.txt",
			"file.txt",	//filename
			"",		//domain
			"txt",		//extension
			"",		//postfix
			4,		//nrlevels
			true ))	//absolute
    {
	return false;
    }

    if ( !testFilePath( "https://dgbes.com/surveys/aap/noot?x=y&&a=b",
			"noot",		//filename
			"dgbes.com",	//domain
			"",		//extension
			"x=y&&a=b",	//postfix
			3,		//nrlevels
			true ))	//absolute
    {
	return false;
    }

    if ( !testFilePath(
       "https://dgbes.amazon.com/surveys/F3 Demo/Seismics/median_filtered.cbvs",
			"median_filtered.cbvs",	//filename
			"dgbes.amazon.com",	//domain
			"cbvs",		//extension
			"",		//postfix
			4,		//nrlevels
			true ))		//absolute
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
