/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   :	Salil Agarwal
 * DATE     :	Nov 2013
-*/



#include "ziputils.h"

#include "testprog.h"
#include "file.h"
#include "filepath.h"


#define mRunTest(testname,command) \
{ \
if ( !command ) \
{ \
    od_cout() << testname << " failed!\n" << toString(err); \
    File::remove( zipfilename.fullPath() ); \
    File::removeDir( outputdir.fullPath() ); \
    return 1; \
} \
else if ( !quiet_ ) \
    od_cout() << testname << " Succeeded!\n"; \
}


#define mCheckDataIntegrity(relpart1,relpart2) \
{ \
    File::Path src( tozip.fullPath() ); \
    src.add( relpart1 ); \
    src.add( relpart2 ); \
    File::Path dest( zipfilename.pathOnly() ); \
    dest.add( "F3_Test_Survey" ); \
    dest.add( relpart1 ); \
    dest.add( relpart2 ); \
    if ( File::getFileSize(dest.fullPath()) != \
					  File::getFileSize(src.fullPath()) ) \
    { \
	od_cout() << "Data integrety check failed!\n" \
			       << dest.fullPath(); \
	File::remove( zipfilename.fullPath() ); \
	File::removeDir( outputdir.fullPath() ); \
	return 1; \
    } \
    else if ( !quiet_ ) \
	od_cout() << "Data integrety check succeeded!\n"; \
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    const BufferString basedir = clParser().keyedString( "datadir" );
    File::Path tozip( basedir );
    tozip.add( "F3_Test_Survey" );
    uiString err;
    File::Path zipfilename( File::Path::getTempFullPath("zip_test","zip") );
    File::Path outputdir( zipfilename.pathOnly() );
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

    return 0;
}
