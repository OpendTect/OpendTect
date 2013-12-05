/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2013
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "file.h"
#include "oddirs.h"
#include "commandlineparser.h"
#include "filepath.h"
#include "keystrs.h"
#include "od_istream.h"

#include "od_iostream.h"

#define mTest( testname, test ) \
if ( !(test) ) \
{ \
    od_ostream::logStream() << "Test " << testname << " FAILED\n"; \
    return false; \
} \
else if ( !quiet ) \
{ \
    od_ostream::logStream() << "Test " << testname << " - SUCCESS\n"; \
}

#define mRunTest( test ) \
mTest( #test, test )

bool testReadContent( bool quiet )
{
    BufferString basedir = GetSoftwareDir( 0 );

    BufferString buf;

    //Read non existent file - should fail.
    buf.setEmpty();
    FilePath nofile( basedir.buf(), "src", "Basic", "tests","NonExistingFile");
    mRunTest(!File::getContent(nofile.fullPath(),buf) && buf.isEmpty());

    //Read empty file - should work fine.
    buf.setEmpty();
    FilePath emptyfile( basedir.buf(), "src", "Basic", "tests","emptyfile.txt");
    mRunTest(File::getContent(emptyfile.fullPath(),buf) && buf.isEmpty());

    //Read non empty file - should work fine.
    buf.setEmpty();
    FilePath nonempty( basedir.buf(), "src", "Basic", "tests","file.cc");
    mRunTest(File::getContent(nonempty.fullPath(),buf) && buf.size());

    return true;
}


bool testIStream( const char* file, bool quiet )
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


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );
    CommandLineParser parser;

    const bool quiet = parser.hasKey( sKey::Quiet() );

    BufferStringSet normalargs;
    CommandLineParser().getNormalArguments(normalargs);

    if ( !normalargs.size() )
    {
        od_ostream::logStream() << "No input file specified";
        ExitProgram( 1 );
    }


    if ( !testReadContent(quiet) )
        ExitProgram( 1 );

    if ( !testIStream( normalargs.get(0).buf(), quiet ) )
	ExitProgram( 1 );


    ExitProgram(0);
}

