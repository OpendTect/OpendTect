/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : March 2018
-*/


#include "hdf5arraynd.h"
#include "testprog.h"
#include "file.h"
#include "filepath.h"
#include "arrayndimpl.h"
#include "iopar.h"
#include "plugins.h"

static BufferString filename_;
static const int nrblocks_ = 5;
static const int dim1_ = 10;
static const int dim2_ = 20;
static const int chunksz_ = 6;
static const char* sPropNm = "Block and comp idxs";

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


static bool testWrite()
{
    PtrMan<HDF5::Writer> wrr = HDF5::mkWriter();
    mRunStandardTest( wrr, "Get Writer" );
    uiRetVal uirv = wrr->open( filename_ );
    mAddTestResult( "Open file for write" );

    mRunStandardTestWithError( filename_==wrr->fileName(), "File name retained",
			       BufferString(wrr->fileName(),"!=",filename_) )
    wrr->setChunkSize( chunksz_ );

    Array2DImpl<float> arr2d( dim1_, dim2_ );
    HDF5::DataSetKey dsky;
    IOPar iop;
    iop.set( "File attr", "file attr value" );
    uirv = wrr->putInfo( dsky, iop );
    mAddTestResult( "Write file attribute" );

    iop.setEmpty();
    HDF5::ArrayNDTool<float> arrtool( arr2d );
    iop.set( "Apenoot", "pere boom" );
    for ( int idx=0; idx<nrblocks_; idx++ )
    {
	dsky.setDataSetName( BufferString( "Block [", idx, "]" ) );

	fillArr2D( arr2d, 1000*idx );
	dsky.setGroupName( "Component 1" );
	uirv = arrtool.putData( *wrr, dsky );
	if ( !uirv.isOK() )
	    break;
	iop.set( sPropNm, idx, 1 );
	uirv = wrr->putInfo( dsky, iop );
	if ( !uirv.isOK() )
	    break;

	fillArr2D( arr2d, 10000*idx );
	dsky.setGroupName( "Component 2" );
	uirv = arrtool.putData( *wrr, dsky );
	if ( !uirv.isOK() )
	    break;
	iop.set( sPropNm, idx, 2 );
	uirv = wrr->putInfo( dsky, iop );
	if ( !uirv.isOK() )
	    break;
    }
    mAddTestResult( "Write blocks" );

    return true;
}


static bool testRead()
{
    PtrMan<HDF5::Reader> rdr = HDF5::mkReader();
    mRunStandardTest( rdr, "Get Reader" );

    uiRetVal uirv = rdr->open( filename_ );
    mAddTestResult( "Open file for read" );

    BufferStringSet grps;
    rdr->getGroups( grps );
    mRunStandardTestWithError( grps.size()==2, "Groups in file",
			       BufferString("nrgrps=",grps.size()) );

    BufferStringSet dsnms;
    rdr->getDataSets( grps.get(0), dsnms );
    mRunStandardTestWithError( dsnms.size()==nrblocks_, "Datasets in group",
			       BufferString("nrblocks=",grps.size()) );

    HDF5::DataSetKey dsky( "Component 3", "Block [3]" );
    bool scoperes = rdr->setScope( dsky );
    mRunStandardTest( !scoperes, "Set scope (non-existing)" )
    dsky = HDF5::DataSetKey( "Component 2", "Block [3]" );
    scoperes = rdr->setScope( dsky );
    mRunStandardTest( scoperes, "Set scope (existing)" )

    PtrMan<ArrayNDInfo> arrinf = rdr->getDataSizes();
    mRunStandardTest( arrinf, "Get dimensions" )
    const int nrdims = arrinf->nrDims();
    mRunStandardTestWithError( nrdims==2, "Correct nr dimensions",
				BufferString("nrdims=",nrdims) )
    const int dim1 = arrinf->getSize( 0 );
    mRunStandardTestWithError( dim1==dim1_, "Correct size of 1st dim",
				BufferString("dim1=",dim1) )
    const int dim2 = arrinf->getSize( 1 );
    mRunStandardTestWithError( dim2==dim2_, "Correct size of 2nd dim",
				BufferString("dim2=",dim2) )

    IOPar iop;
    uirv = rdr->getInfo( iop );
    mAddTestResult( "Get dataset info" );
    int iblk=0, icomp=0;
    iop.get( sPropNm, iblk, icomp );
    mRunStandardTestWithError( iblk==3 && icomp==2, "dataset info contents",
		BufferString("iblk=",iblk).add(" icomp=").add(icomp) );

    Array2DImpl<float> arr2d( arrinf->getSize(0), arrinf->getSize(1) );
    uirv = rdr->getAll( arr2d.getData() );
    mAddTestResult( "Get block data" );
    const float arrval = arr2d.get( 6, 15 );
    mRunStandardTestWithError( arrval==30615.f, "Correct value [comp2,6,16]",
				BufferString("arrval=",arrval) )

    HDF5::DataSetKey filedsky;
    scoperes = rdr->setScope( filedsky );
    mRunStandardTest( scoperes, "Set scope (file)" )
    uirv = rdr->getInfo( iop );
    mAddTestResult( "Get file info" );
    const BufferString iopval = iop.find( "File attr" );
    mRunStandardTestWithError( iopval=="file attr value", "file info contents",
				BufferString("found: '",iopval,"'") );

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
