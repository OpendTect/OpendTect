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
    const FilePath fp( FilePath::getTempFullPath(mTestDirNm,0) );
    File::createDir( fp.fullPath() );
    const FilePath filefp( fp.fullPath(),
			    FilePath::getTempFileName("python","txt") );
    od_ostream strm( filefp.fullPath() );
    strm << "Testing deletion via python command line";
    strm.close();

    return fp.fullPath();
}

bool testRemoveDir( const BufferString& path, bool expectedres )
{
    bool ret = OD::pythonRemoveDir( path, true ).isOK();
    if ( ret )
    {
	const DirList dl( FilePath::getTempDir(), DirList::DirsOnly );

	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    FilePath fp = dl.fullPath(idx);

	    const BufferString dirnm = fp.dir();

	    if ( dirnm.isEqual(mTestDirNm) )
		ret = false;
	}
    }

    const BufferString desc = expectedres ? "Writable folder deleted"
					  : "Read-only folder not deleted";
    mRunStandardTest( ret == expectedres, desc )

    return ret;
}

int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !OD::PythA().isUsable().isOK() )
    {
	if ( !quiet )
	    od_cout() << "Python link is not usable" << od_endl;
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
