/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   :	Salil Agarwal
 * DATE     :	Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "ziputils.h"

#include "commandlineparser.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"

#include "od_iostream.h"

#define mRunTest(testname,command) \
{ \
if ( !command ) \
{ \
    od_ostream::logStream() << testname << " failed!\n" << err.buf(); \
    File::remove( zipfilename.fullPath() ); \
    File::removeDir( outputdir.fullPath() ); \
    ExitProgram(1); \
} \
else if ( !quiet ) \
    od_ostream::logStream() << testname << " Succeeded!\n"; \
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
	od_ostream::logStream() << "Data integrety check failed!\n" \
			       << dest.fullPath(); \
	File::remove( zipfilename.fullPath() ); \
	File::removeDir( outputdir.fullPath() ); \
	ExitProgram(1); \
    } \
    else if ( !quiet ) \
	od_ostream::logStream() << "Data integrety check succeeded!\n"; \
}


int main(int argc, char** argv)
{
    od_init_test_program( argc, argv );
    CommandLineParser clparser;
    const bool quiet = clparser.hasKey( sKey::Quiet() );

    BufferString basedir;
    clparser.getVal("datadir", basedir );
    FilePath tozip( basedir );
    tozip.add( "F3_Test_Survey" );
    BufferString err;
    FilePath zipfilename( FilePath::getTempName("zip") );
    FilePath outputdir( zipfilename.pathOnly() );
    outputdir.add( "F3_Test_Survey" );

    mRunTest("Zipping", ZipUtils::makeZip(zipfilename.fullPath(),
					  tozip.fullPath(),err) );
    mRunTest("Unzipping", ZipUtils::unZipArchive(zipfilename.fullPath(),
						 outputdir.pathOnly(),err) );

    mCheckDataIntegrity( "Seismics","Seismic.cbvs" );
    mCheckDataIntegrity( "Misc",".omf" );
    mCheckDataIntegrity( "Surfaces","horizonrelations.txt" );
    mCheckDataIntegrity( "WellInfo","F03-4^5.wll" );

    File::remove( zipfilename.fullPath() );
    File::removeDir( outputdir.fullPath() );

    ExitProgram(0);
}
