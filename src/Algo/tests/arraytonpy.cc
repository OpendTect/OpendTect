/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "arrayndalgo.h"

#include <csignal>

static const float floatarr[] =
{
    1.5, 2.5, 3.5,
    4.5, 5.5, mUdf(float),
    7.5, 8.5, 9.5
};


static const double doublearr[] =
{
    -3.141592653589793, 0.0, mUdf(double),
    -1.0e-6, 42.5, 1.0e3  
};


static const int intarr[] =
{
    -42, 0, 1, mUdf(int), 1024, -32768
};


static const od_int64 int64arr[] = 
{
    0, 1, -1,  42, -42, 1000,  -1000, 7, -7,
    123456789, -123456789, 2,  32767, -32768, 65536,
    -65536, 99, -99,  INT64_MAX, -1, 1, INT64_MIN, 0, -2,  
    9999999999LL, -9999999999LL, 256
};

static BufferString workdir_;
static BufferString floatfpstr_;
static BufferString doublefpstr_;
static BufferString intfpstr_;
static BufferString int64fpstr_;

void cleanup()
{
    if ( File::exists(workdir_.buf()) )
	File::removeDir( workdir_.buf() );

    if ( File::exists(floatfpstr_.buf()) )
	File::removeDir( floatfpstr_.buf() );

    if ( File::exists(doublefpstr_.buf()) )
	File::removeDir( doublefpstr_.buf() );

    if ( File::exists(intfpstr_.buf()) )
	File::removeDir( intfpstr_.buf() );

    if ( File::exists(int64fpstr_.buf()) )
	File::removeDir( int64fpstr_.buf() );
}


void signalHandler( int signum )
{
    errStream() << "Interrupt signal (" << signum << ") received." << od_endl;
    exit( signum );
}


bool testExportFloatAsNpy()
{
    Array2DImpl<float> farr( 3, 3 );
    farr.setData( floatarr );
    FilePath ffp( workdir_, "farr" );
    ffp.setExtension( "npy" );
    floatfpstr_ = ffp.fullPath();
    uiRetVal uirv;
    saveAsNpy( farr, floatfpstr_, uirv );
    mRunStandardTestWithError( uirv.isOK(), "Exporting float array to .npy",
			       uirv.getText() );

    const BufferString existdescstr( floatfpstr_, " file exists." );
    const BufferString existerrstr( floatfpstr_, " file does not exist." );
    mRunStandardTestWithError( File::exists(floatfpstr_.buf()),
			       existdescstr, existerrstr );

    const BufferString emptydescstr( floatfpstr_, " file exists." );
    const BufferString emptyerrstr( floatfpstr_, " file does not exist." );
    mRunStandardTestWithError( !File::isEmpty(floatfpstr_.buf()),
			       emptydescstr, emptyerrstr );

    return true;
}


bool testExportDoubleAsNpy()
{
    Array2DImpl<double> darr( 2, 3 );
    darr.setData( doublearr );
    FilePath dfp( workdir_, "darr" );
    dfp.setExtension( "npy" );
    doublefpstr_ = dfp.fullPath();
    uiRetVal uirv;
    saveAsNpy( darr, doublefpstr_.buf(), uirv );
    mRunStandardTestWithError( uirv.isOK(), "Exporting double array to .npy",
			       uirv.getText() );

    const BufferString existdescstr( doublefpstr_, " file exists." );
    const BufferString existerrstr( doublefpstr_, " file does not exist." );
    mRunStandardTestWithError( File::exists(doublefpstr_.buf()),
			       existdescstr, existerrstr );

    const BufferString emptydescstr( doublefpstr_, " file exists." );
    const BufferString emptyerrstr( doublefpstr_, " file does not exist." );
    mRunStandardTestWithError( !File::isEmpty(doublefpstr_.buf()),
			       emptydescstr, emptyerrstr );

    return true;
}


bool testExportIntAsNpy()
{
    Array1DImpl<int> iarr( 6 );
    iarr.setData( intarr );
    FilePath ifp( workdir_, "iarr" );
    ifp.setExtension( "npy" );
    intfpstr_ = ifp.fullPath();
    uiRetVal uirv;
    saveAsNpy( iarr, intfpstr_.buf(), uirv );
    mRunStandardTestWithError( uirv.isOK(), "Exporting int array to .npy",
			       uirv.getText() );

    const BufferString existdescstr( intfpstr_, " file exists." );
    const BufferString existerrstr( intfpstr_, "file does not exist." );
    mRunStandardTestWithError( File::exists(intfpstr_.buf()),
			       existdescstr, existerrstr );

    const BufferString emptydescstr( intfpstr_, " file exists." );
    const BufferString emptyerrstr( intfpstr_, " file does not exist." );
    mRunStandardTestWithError( !File::isEmpty(intfpstr_.buf()),
			       emptydescstr, emptyerrstr );

    return true;
}


bool testExportInt64AsNpy()
{
    Array3DImpl<od_int64> i64arr( 3, 3, 3 );
    i64arr.setData( int64arr );
    FilePath i64fp( workdir_, "i64arr" );
    i64fp.setExtension( "npy" );
    int64fpstr_ = i64fp.fullPath();
    uiRetVal uirv;
    saveAsNpy( i64arr, int64fpstr_.buf(), uirv );
    mRunStandardTestWithError( uirv.isOK(), "Exporting od_int64 array to .npy",
			       uirv.getText() );

    const BufferString existdescstr( int64fpstr_, " file exists." );
    const BufferString existerrstr( int64fpstr_, " file does not exist." );
    mRunStandardTestWithError( File::exists(int64fpstr_.buf()),
			       existdescstr, existerrstr );

    const BufferString emptydescstr( int64fpstr_, " file exists." );
    const BufferString emptyerrstr( int64fpstr_, " file does not exist." );
    mRunStandardTestWithError( !File::isEmpty(int64fpstr_.buf()),
			       emptydescstr, emptyerrstr );

    return true;
}


bool testGetFloatFromNpy()
{
    const BufferString doesnotexiststr( floatfpstr_, " does not exist." );
    const BufferString existstr( floatfpstr_, ": file exists" );
    mRunStandardTestWithError( File::exists(floatfpstr_), existstr,
			       doesnotexiststr );
    uiRetVal uirv;
    PtrMan<ArrayND<float>> data 
		= getFromNpy<float>( floatfpstr_.buf(), uirv );
    mRunStandardTestWithError( uirv.isOK(), 
			       "Reading from .npy file", uirv.getText() );
    mRunStandardTestWithError( data, "Got a valid array pointer", 
			       "Return value: nullptr" );

    const int exparrsz = 9;
    const od_int64 datasz = data->totalSize();
    BufferString errmsg( "Expected size: ", exparrsz, 
			 "Size of float array read: " );
    errmsg.add( datasz );
    mRunStandardTestWithError( datasz==exparrsz, "Read size correct", errmsg );

    Array2DImpl<float> farr( 3, 3 );
    farr.setData( floatarr );
    mRunStandardTestWithError( farr.info()==data->info(), 
			       "Expected ArrayNDInfo", 
			       "Unexpected array dimensions" );
   
    bool valtest = true;
    float expectedval = mUdf(float);
    float readval = mUdf(float);
    for ( int idx=0; idx<exparrsz; idx++ )
    {
	expectedval = floatarr[idx];
	readval = data->getData()[idx];
	if ( expectedval != readval )
	{
	    if ( std::isnan(expectedval) && mIsUdf(readval) )
		continue;

	    valtest = false;
	    break;
	}
    }

    BufferString comperrmsg( "Expected float value: ", expectedval, 
			     "; Read value: ");
    comperrmsg.add( readval );
    mRunStandardTestWithError( valtest, "Values compared.", comperrmsg );

    return true;
}


bool testGetDoubleFromNpy()
{
    const BufferString doesnotexiststr( doublefpstr_, " does not exist." );
    const BufferString existstr( doublefpstr_, ": file exists" );
    mRunStandardTestWithError( File::exists(doublefpstr_), existstr,
			       doesnotexiststr );
    uiRetVal uirv;
    PtrMan<ArrayND<double>> data 
	    = getFromNpy<double>( doublefpstr_.buf(), uirv );
    mRunStandardTestWithError( uirv.isOK(), 
			       "Reading from .npy file", uirv.getText() );
    mRunStandardTestWithError( data, "Got a valid array pointer", 
			       "Return value: nullptr" );

    const int exparrsz = 6;
    const od_int64 datasz = data->totalSize();
    BufferString errmsg( "Expected size: ", exparrsz, 
			 "Size of double array read: " );
    errmsg.add( datasz );
    mRunStandardTestWithError( datasz==exparrsz, "Read size correct", errmsg );

    Array2DImpl<double> darr( 2, 3 );
    darr.setData( doublearr );
    mRunStandardTestWithError( darr.info()==data->info(), 
			       "Expected ArrayNDInfo", 
			       "Unexpected array dimensions" );
    bool valtest = true;
    float expectedval = mUdf(double);
    float readval = mUdf(double);
    for ( int idx=0; idx<exparrsz; idx++ )
    {
	expectedval = doublearr[idx];
	readval = data->getData()[idx];
	if ( expectedval != readval )
	{
	    if ( std::isnan(expectedval) && mIsUdf(readval) )
		continue;

	    valtest = false;
	    break;
	}
    }

    BufferString comperrmsg( "Expected double value: ", expectedval, 
			     "; Read value: ");
    comperrmsg.add( readval );
    mRunStandardTestWithError( valtest, "Values compared.", comperrmsg );

    return true;
}


bool testGetIntFromNpy()
{
    const BufferString doesnotexiststr( intfpstr_, " does not exist." );
    const BufferString existstr( intfpstr_, ": file exists" );
    mRunStandardTestWithError( File::exists(intfpstr_), existstr,
			       doesnotexiststr );
    uiRetVal uirv;
    PtrMan<ArrayND<int>> data 
		    = getFromNpy<int>( intfpstr_.buf(), uirv );
    mRunStandardTestWithError( uirv.isOK(), 
			       "Reading from .npy file", uirv.getText() );
    mRunStandardTestWithError( data, "Got a valid array pointer", 
			       "Return value: nullptr" );

    const int exparrsz = 6;
    const od_int64 datasz = data->totalSize();
    BufferString errmsg( "Expected size: ", exparrsz, 
			 "Size of int array read: " );
    errmsg.add( datasz );
    mRunStandardTestWithError( datasz==exparrsz, "Read size correct", errmsg );

    Array1DImpl<int> iarr( exparrsz );
    iarr.setData( intarr );
    mRunStandardTestWithError( iarr.info()==data->info(), 
			       "Expected ArrayNDInfo", 
			       "Unexpected array dimensions" );
    bool valtest = true;
    float expectedval = mUdf(int);
    float readval = mUdf(int);

    TypeSet<int> dataset;
    for ( int idx=0; idx<exparrsz; idx++ )
	dataset += data->getData()[idx];

    for ( int idx=0; idx<exparrsz; idx++ )
    {
	if ( intarr[idx] != data->getData()[idx] )
	{
	    valtest = false;
	    expectedval = intarr[idx];
	    readval = data->getData()[idx];
	    break;
	}
    }

    BufferString comperrmsg( "Expected int value: ", expectedval, 
			     "; Read value: ");
    comperrmsg.add( readval );
    mRunStandardTestWithError( valtest, "Values compared.", comperrmsg );

    return true;
}


bool testGetInt64FromNpy()
{
    const BufferString doesnotexiststr( int64fpstr_, " does not exist." );
    const BufferString existstr( int64fpstr_, ": file exists" );
    mRunStandardTestWithError( File::exists(int64fpstr_), existstr,
			       doesnotexiststr );
    uiRetVal uirv;
    PtrMan<ArrayND<od_int64>> data 
	    = getFromNpy<od_int64>( int64fpstr_.buf(), uirv );
    mRunStandardTestWithError( uirv.isOK(), 
			       "Reading from .npy file", uirv.getText() );
    mRunStandardTestWithError( data, "Got a valid array pointer", 
			       "Return value: nullptr" );

    const int exparrsz = 27;
    const od_int64 datasz = data->totalSize();
    BufferString errmsg( "Expected size: ", exparrsz, 
			 "Size of od_int64 array read: " );
    errmsg.add( datasz );
    mRunStandardTestWithError( datasz==exparrsz, "Read size correct", errmsg );

    Array3DImpl<od_int64> i64arr( 3, 3, 3 );
    i64arr.setData( int64arr );
    mRunStandardTestWithError( i64arr.info()==data->info(), 
			       "Expected ArrayNDInfo", 
			       "Unexpected array dimensions" );
    bool valtest = true;
    float expectedval = mUdf(od_int64);
    float readval = mUdf(od_int64);
    for ( int idx=0; idx<exparrsz; idx++ )
    {
	if ( int64arr[idx] != data->getData()[idx] )
	{
	    valtest = false;
	    expectedval = int64arr[idx];
	    readval = data->getData()[idx];
	    break;
	}
    }

    BufferString comperrmsg( "Expected od_int64 value: ", expectedval, 
			     "; Read value: ");
    comperrmsg.add( readval );
    mRunStandardTestWithError( valtest, "Values compared.", comperrmsg );
    
    return true;
}


bool createWorkingDir()
{
    const BufferString tempdirfp = FilePath::getTempDir();;
    FilePath fp( tempdirfp.str(), "od_test_npy_read_write" );
    workdir_ = fp.fullPath();
    if ( File::exists(workdir_.str()) )
	mRunStandardTest( File::removeDir(workdir_.str()),
			  "Remove existing working directory" );

    mRunStandardTest( File::createDir(workdir_.str()),
		      BufferString("Creating working directory: ",workdir_) );
    mRunStandardTestWithError( File::exists(workdir_.buf()),
		      "Working directory created.", 
		      "Could not create working directory" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    signal( SIGINT, signalHandler );
    signal( SIGTERM, signalHandler );
    atexit( cleanup );

    if ( !createWorkingDir() )
	return 1;

    if ( !testExportFloatAsNpy() || 
	 !testGetFloatFromNpy() ||
	 !testExportDoubleAsNpy() || 
	 !testGetDoubleFromNpy() ||
	 !testExportIntAsNpy() ||
	 !testGetIntFromNpy() ||
	 !testExportInt64AsNpy()||
	 !testGetInt64FromNpy() )
	return 1;

    return 0;
}
