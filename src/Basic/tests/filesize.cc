/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"
#include "file.h"
#include "oddirs.h"

static const char* sOneKBStr		    = "1.00 kB";
static const char* sNonEmptyFSizeStr	    = "9.00 bytes";

static const od_int64 sSizeZero		    = 0;

bool testSizeStrEmptyAndCleanup( const BufferString& tempfile )
{
    mRunStandardTest(File::exists(tempfile), "Empty file exists");
    const od_int64 emptyfsz = File::getFileSize( tempfile );
    mRunStandardTest((emptyfsz==sSizeZero),"File size of empty file");
    BufferString szstr;
    szstr = File::getFileSizeString( tempfile.buf() );
    mRunStandardTest(!szstr.isEmpty(),"Size string created")
    mRunStandardTest((szstr=="empty"),"empty file size string")
    return true;
}


bool testSizeStrNonEmptyAndCleanup( const BufferString& tempfile )
{
    od_ostream strm( tempfile );
    strm << "test text";
    strm.close();
    mRunStandardTest(File::exists(tempfile),"Non-empty file exists");
    const od_int64 fsz = File::getFileSize( tempfile );
    mRunStandardTest((fsz==9),"Valid file size");
    BufferString szstr;
    szstr = File::getFileSizeString( tempfile.buf() );
    mRunStandardTest(!szstr.isEmpty(),"Size string created")
    mRunStandardTest((szstr==sNonEmptyFSizeStr),"Valid file size string")
    return true;
}


bool testFileSize()
{
    const BufferString tempfile = FilePath::getTempFullPath( "test", "txt" );
    mRunStandardTest(!tempfile.isEmpty(),"Temp filepath created")

    BufferString szstr;
    //get file size of non existent file.
    mRunStandardTest(!File::exists(tempfile),"File does not exist");
    const od_int64 nofsz = File::getFileSize( tempfile );
    mRunStandardTest((nofsz==0),"File size of non-existent file")
    szstr = File::getFileSizeString( tempfile.buf() );
    mRunStandardTest(!szstr.isEmpty(),"Size string created")
    mRunStandardTest((szstr=="File not found"),"non-existent file size string")

    //get file size of empty file - should work fine.
    od_ostream stream( tempfile );
    stream.close();
    if ( !testSizeStrEmptyAndCleanup(tempfile) )
    {
	File::remove( tempfile );
	return false;
    }

    //get file size of non empty file - should work fine.
    if ( !testSizeStrNonEmptyAndCleanup(tempfile) )
    {
	File::remove( tempfile );
	return false;
    }

    mRunStandardTest(File::remove(tempfile),"Remove temporary file")
    mRunStandardTest(!File::exists(tempfile),"Temp file removed");

    return true;
}


bool testFileSizeString()
{
    BufferString szstr;
    szstr = File::getFileSizeString( -1 );
    mRunStandardTest(!szstr.isEmpty(),"Size string created")
    mRunStandardTest((szstr=="File not found"),"File not found string")

    szstr.setEmpty();
    szstr = File::getFileSizeString( sSizeZero );
    mRunStandardTest(!szstr.isEmpty(),"Size string created")
    mRunStandardTest((szstr=="empty"),"empty string")

    szstr.setEmpty();
    szstr = File::getFileSizeString( 1024 );
    mRunStandardTest(!szstr.isEmpty(),"Size string created")
    mRunStandardTest((szstr==sOneKBStr),"Actual size string")

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testFileSize() || !testFileSizeString() )
	return 1;

    return 0;
}
