/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ziputils.h"

#include "testprog.h"
#include "file.h"
#include "filepath.h"

#define mCleanup() \
    delete taskrun; \
    File::remove( zipfilename.fullPath() ); \
    File::remove( zipfilename2.fullPath() ); \
    File::removeDir( outputdir.fullPath() ); \


#define mRunTest(testname,command) \
{ \
if ( !command ) \
{ \
    errStream() << testname << " failed!\n" << err.getFullString(); \
    mCleanup() \
    return 1; \
} \
else \
    logStream() << testname << " Succeeded!\n"; \
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
	errStream() << "Data integrety check failed!\n" \
			       << dest.fullPath(); \
	mCleanup() \
	return 1; \
    } \
    else \
	logStream() << "Data integrety check succeeded!\n"; \
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    BufferString basedir;
    clParser().getVal("datadir", basedir );
    FilePath tozip( basedir );
    tozip.add( "F3_Test_Survey" );
    uiString err;
    const FilePath zipfilename(
		FilePath::getTempFullPath("zip_test","zip") );
    const FilePath zipfilename2(
		FilePath::getTempFullPath("zip_test2","zip") );

    FilePath outputdir( zipfilename.pathOnly() );
    outputdir.add( "F3_Test_Survey" );

    TaskRunner* taskrun = nullptr;
    if ( !quiet_ )
	taskrun = new TextTaskRunner( logStream() );

    mRunTest("Zipping", ZipUtils::makeZip(zipfilename.fullPath(),
					  tozip.fullPath(),err,taskrun) );
    mRunTest("Zipping (copy)", ZipUtils::makeZip(zipfilename2.fullPath(),
					  tozip.fullPath(),err,taskrun) );
    mRunTest("Unzipping", ZipUtils::unZipArchive(zipfilename.fullPath(),
					 outputdir.pathOnly(),err,taskrun) );
    mRunTest("Unzipping (copy)", ZipUtils::unZipArchive(zipfilename2.fullPath(),
					 outputdir.pathOnly(),err,taskrun) );

    mCheckDataIntegrity( "Seismics","Seismic.cbvs" );
    mCheckDataIntegrity( "Misc",".omf" );
    mCheckDataIntegrity( "Surfaces","horizonrelations.txt" );
    mCheckDataIntegrity( "WellInfo","F03-4^5.wll" );

    mCleanup();

    return 0;
}
