/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "testprog.h"

#include "dirlist.h"
#include "genc.h"
#include "file.h"
#include "filepath.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "oscommand.h"
#include "pythonaccess.h"

#include <iostream>
#include "settingsaccess.h"

#define mTestDirNm "Test Dir"

BufferString createTestDir()
{
    const File::Path fp( File::Path::getTempFullPath(mTestDirNm,0) );
    File::createDir( fp.fullPath() );
    const File::Path filefp( fp.fullPath(),
			    File::Path::getTempFileName("python","txt") );
    od_ostream strm( filefp.fullPath() );
    strm << "Testing deletion via python command line";
    strm.close();

    return fp.fullPath();
}


bool testRemoveDir( const char* path, bool expectedres )
{
    const uiRetVal uirv = OD::pythonRemoveDir( path, true );
    BufferString err = toString(uirv);

    const File::Path fp( path );
    const BufferString dirnm = fp.dir();
    const DirList dl( File::Path::getTempDir(), File::DirsInDir );
    if ( expectedres )
    {
	bool missing = true;
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    if ( dl.get(idx) == dirnm )
	    {
		missing = false;
		break;
	    }
	}
	if ( !missing )
	    err += "Folder reported deleted but still present";
    }
    else
    {
	bool present = false;
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    if ( dl.get(idx) == dirnm )
	    {
		present = true;
		break;
	    }
	}
	if ( present )
	    err += "Folder to be tested is missing";
    }

    const bool ret = uirv.isOK() && err.isEmpty();
    const BufferString desc = expectedres ? "Delete writable folder"
					  : "Don't delete read-only folder";
    mRunStandardTestWithError( ret == expectedres, desc, err );

    return ret;
}

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    const uiRetVal uirv = OD::PythA().isUsable();
    if ( !uirv.isOK() )
    {
	logStream() << "Python link is not usable" << od_newline;
	logStream() << toString(uirv) << od_endl;
	return 1;
    }

    const BufferString path = createTestDir();
    File::makeReadOnly( path, true );
    if ( testRemoveDir(path,false) )
	return 1;

    File::makeWritable( path, true, true );
    if ( !testRemoveDir(path,true) )
	return 1;

    return 0;
}
