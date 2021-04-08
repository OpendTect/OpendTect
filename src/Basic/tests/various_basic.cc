/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : Jun 2020
 * FUNCTION :
-*/


#include "testprog.h"

#include "file.h"
#include "filepath.h"
#include "multiid.h"
#include "perthreadrepos.h"

#ifdef __win__
# include "winutils.h"
#endif


bool testPointerCast()
{
    float val;
    float* ptr = &val;

    void* voidptr = ptr;

    float* newptr = reinterpret_cast<float*>(voidptr);

    mRunStandardTest( newptr==ptr, "Pointer cast" );

    return true;
}


bool testPointerAlignment()
{
    char buffer[] = { 0, 0, 0, 0, 1, 1, 1, 1 };

    char* ptr0 = buffer;
    char* ptr1 = buffer+1;

    od_uint32* iptr0 = reinterpret_cast<od_uint32*>( ptr0 );
    od_uint32* iptr1 = reinterpret_cast<od_uint32*>( ptr1 );

    od_uint32 ival0 = *iptr0;
    od_uint32 ival1 = *iptr1;

    char* ptr0_mod = (char*) iptr0;
    char* ptr1_mod = (char*) iptr1;

    union IVal1Reference
    {
    public:
			IVal1Reference()
		{
                            charval_[0] = 0;
                            charval_[1] = 0;
                            charval_[2] = 0;
                            charval_[3] = 1;
                        }
        char		charval_[4];
        int		intval_;
    }  val1ref;

    mRunStandardTest( ptr0_mod==ptr0 && ptr1_mod==ptr1 &&
		      ival0==0 && ival1 == val1ref.intval_,
		       "Pointer alignment" );

    return true;
}


bool test64BitDetection()
{
#ifdef __win__
    mRunStandardTest( is64BitWindows(), "Detecting 64 bit Windows OS" );
#endif
    return true;
}



bool testOSVersion()
{
    mRunStandardTest( GetOSIdentifier(), "GetOSIdentifier not returning null" );

    return true;
}

#define mFuncInMacro( dummy ) BufferString( __func__ )


bool testFuncName()
{
    BufferString func = mFuncInMacro( "Hello" );
    mRunStandardTest( func=="testFuncName", "Function name macro" );

    return true;
}


static BufferString getTestFileName()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
	ret = FilePath::getTempFullPath( "test with space", "txt" );
    return ret;
}


bool testFilePermissions( const char* fnm)
{
    mRunStandardTest( !File::exists(fnm), "Temporary file does not exist" );
    od_ostream strm( fnm );
    strm << "some content";
#ifdef __win__
    mRunStandardTest( File::isInUse(fnm), "Temporary file is being used" );
#endif
    strm.close();

#ifdef __win__
    mRunStandardTest( !File::isInUse(fnm), "Temporary file is not used" );
#endif

    mRunStandardTest( File::exists(fnm), "Temporary file is created" );
    mRunStandardTest( File::isReadable(fnm), "Temporary file is readable" );
    mRunStandardTest( File::makeReadOnly(fnm,true),
	"Temporary file set read-only" );
    mRunStandardTest( !File::isWritable(fnm),
	"Temporary file is read-only" );
    mRunStandardTest( File::makeWritable(fnm,true,false),
	"Temporary file set writable" );
    mRunStandardTest( File::isWritable(fnm),
	"Temporary file is writable" );
#ifdef __unix__
    mRunStandardTest( File::makeExecutable(fnm,true),
	"Temporary file set executable" );
    mRunStandardTest( File::isExecutable(fnm),
	"Temporary file is executable" );
    mRunStandardTest( File::makeExecutable(fnm,false),
	"Temporary file set not executable" );
    mRunStandardTest( !File::isExecutable(fnm),
	"Temporary file is not executable" );
#else
    File::hide( fnm, true );
    mRunStandardTest( File::isHidden(fnm),
	"Temporary file is hidden" );
    File::hide( fnm, false );
    mRunStandardTest( !File::isHidden(fnm),
	"Temporary file is not hidden" );

    bool res = File::isWritable("C:\\temp");
    res = File::isWritable("C:\\Program Files");
    res = File::isWritable("C:\\Program Files\\OpendTect");
    res = File::isWritable("C:\\Program Files\\OpendTect\\6.6.0");
    res = File::isWritable("C:\\Program Files\\OpendTect\\6.6.0\\bla");
    res = File::isWritable(
	"C:\\Program Files\\OpendTect\\6.6.0\\relinfo\\ver.base_win64.txt");
    res = File::isWritable("D:\\ODData");
    res = File::isWritable("E:\\surveys");
    res = File::isWritable("F:");
#endif

    return true;
}


bool testRemoveFile(const char* fnm)
{
    if ( !File::exists(fnm) )
	return true;

#ifdef __win__
    //Will fail if run with elevated privileges (should not be tried anyway)
    mRunStandardTest(WinUtils::belongsToStdUser(fnm),
		    "WinUtils test ownership" );
    mRunStandardTest( File::makeReadOnly(fnm,false) && !File::remove(fnm),
	"Temporary file is read-only and cannot be deleted" );
#else
    // Linux allows removing read-only files, if the parent folder is writable
#endif
    mRunStandardTest( File::makeWritable(fnm,true,false) && File::remove(fnm),
	"Temporary file is writable and can be deleted" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    // Main idea is to test things that are so basic they don't
    // really fit anywhere else.

    if ( !testPointerCast()
	|| !testOSVersion()
        || !testPointerAlignment()
	|| !testFuncName()
	|| !test64BitDetection() )
	return 1;

    const BufferString fnm = getTestFileName();
    const bool success = testFilePermissions( fnm );
    if ( !testRemoveFile(fnm) )
	return 1;

    return success ? 0 : 1;
}
