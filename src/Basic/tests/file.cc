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

#include <iostream>

#define mRunTest( test ) \
if ( !(test) ) \
{ \
    std::cerr << "Test " << #test << " FAILED\n"; \
    return false; \
} \
else if ( !quiet ) \
{ \
    std::cerr << "Test " << #test << " - SUCCESS\n"; \
}

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


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );
    CommandLineParser parser;

    const bool quiet = parser.hasKey( sKey::Quiet() );

    if ( !testReadContent(quiet) )
        ExitProgram( 1 );


    ExitProgram(0);
}

