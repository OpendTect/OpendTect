/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2014
 * FUNCTION :
-*/


#include "hdf5writer.h"
#include "hdf5reader.h"
#include "testprog.h"
#include "file.h"
#include "filepath.h"
#include "arrayndimpl.h"
#include "plugins.h"

static BufferString filename_;
static const int dim1_ = 10;
static const int dim2_ = 20;
static const int chunksz_ = 6;

static void fillArr2D( Array2D<float>& arr2d, int shft )
{
    const int dim0 = arr2d.info().getSize( 0 );
    const int dim1 = arr2d.info().getSize( 1 );
    for ( int idx0=0; idx0<dim0; idx0++ )
    {
	for ( int idx1=0; idx1<dim1; idx1++ )
	    arr2d.set( idx0, idx1, shft + idx0*100 + idx1 );
    }
}


#define mAddTestResult(desc) \
    mRunStandardTestWithError( uirv.isOK(), desc, toString(uirv) )


bool testWrite()
{
    PtrMan<HDF5::Writer> wrr = HDF5::mkWriter();
    mRunStandardTest( wrr, "Get Writer" );
    uiRetVal uirv = wrr->open( filename_ );
    mAddTestResult( "Open file for write" );

    mRunStandardTestWithError( filename_==wrr->fileName(), "Retain file name",
			       BufferString(wrr->fileName(),"!=",filename_) )

    wrr->setDataType( OD::F32 );
    wrr->setChunkSize( chunksz_ );

    Array2DImpl<float> arr2d( dim1_, dim2_ );
    uirv.setEmpty();
    for ( int idx=0; idx<5; idx++ )
    {
	HDF5::GroupPath path;
	path.add( BufferString("Block [",idx,"]") ).add( "Component 1" );
	fillArr2D( arr2d, 1000*idx );
	uirv = wrr->putData( path, arr2d );
	if ( !uirv.isOK() )
	    break;
	path.set( 1, new BufferString("Component 2") );
	fillArr2D( arr2d, 10000*idx );
	uirv = wrr->putData( path, arr2d );
	if ( !uirv.isOK() )
	    break;
    }
    mAddTestResult( "Write blocks" );

    return true;
}


bool testRead()
{
    PtrMan<HDF5::Reader> rdr = HDF5::mkReader();
    mRunStandardTest( rdr, "Get Reader" );
    //TODO
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    PIM().loadAuto( false );

    if ( !HDF5::isAvailable() )
    {
	tstStream( true ) << "HDF5 not available" << od_endl;
	return 0;
    }

    filename_.set( File::Path(File::getTempPath(),"test.hd5").fullPath() );
    return testWrite() && testRead() ? 0 : 1;
}
