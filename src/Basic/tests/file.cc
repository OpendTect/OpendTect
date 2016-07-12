/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2013
 * FUNCTION :
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
    od_cout() << "Test " << testname << " FAILED\n"; \
    return false; \
} \
else if ( !quiet ) \
{ \
    od_cout() << "Test " << testname << " - SUCCESS\n"; \
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


bool testFilePathParsing()
{
    FilePath winstyle( "C:\\Program Files\\OpendTect 5.0.0\\file.txt" );
    if ( winstyle.fileName() != "file.txt" )
    {
	od_cout() << "Failed to parse Windows style file path" << od_endl;
	od_cout() << "Actual result: " << winstyle.fileName() << od_endl;
	od_cout() << "Expected result: file.txt" << od_endl;
	return false;
    }

    FilePath unixstyle( "/data/apps/OpendTect 5.0.0/file.txt" );
    if ( unixstyle.fileName() != "file.txt" )
    {
	od_cout() << "Failed to parse Unix style file path" << od_endl;
	od_cout() << "Actual result: " << unixstyle.fileName() << od_endl;
	od_cout() << "Expected result: file.txt" << od_endl;
	return false;
    }

    FilePath mixedstyle( "C:\\Program Files/OpendTect\\5.0.0/file.txt" );
    if ( mixedstyle.fileName() != "file.txt" )
    {
	od_cout() << "Failed to parse Windows-Unix mixed file path" << od_endl;
	od_cout() << "Actual result: " << mixedstyle.fileName() << od_endl;
	od_cout() << "Expected result: file.txt" << od_endl;
	return false;
    }

    return true;
}


int testMain( int argc, char** argv )
{
    mInitTestProg();

    BufferStringSet normalargs;
    clparser.getNormalArguments(normalargs);

    if ( normalargs.isEmpty() )
	{ od_cout() << "No input file specified"; return 1; }

    if ( !testReadContent()
      || !testIStream( normalargs.get(0).buf() ) )
	return 1;

    if ( !testFilePathParsing() )
	return 1;

    return 0;
}
