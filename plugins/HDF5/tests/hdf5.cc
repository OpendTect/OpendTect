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
static const int nrdims_ = 2;
static const int dim1_ = 10;
static const int dim2_ = 20;
static const int chunksz_ = 6;
static const char* sPropNm = "Block and comp idxs";

template <class T>
static void fillArr2D( Array2D<T>& arr2d, int shft )
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
    for ( int iblk=0; iblk<nrblocks_; iblk++ )
    {
	dsky.setDataSetName( BufferString( "Block [", iblk, "]" ) );

	fillArr2D( arr2d, 1000*iblk );
	dsky.setGroupName( "Component 1" );
	uirv = arrtool.put( *wrr, dsky );
	if ( !uirv.isOK() )
	    break;
	iop.set( sPropNm, iblk, 1 );
	uirv = wrr->putInfo( dsky, iop );
	if ( !uirv.isOK() )
	    break;

	fillArr2D( arr2d, 10000*iblk );
	dsky.setGroupName( "Component 2" );
	uirv = arrtool.put( *wrr, dsky );
	if ( !uirv.isOK() )
	    break;
	iop.set( sPropNm, iblk, 2 );
	uirv = wrr->putInfo( dsky, iop );
	if ( !uirv.isOK() )
	    break;
    }
    mAddTestResult( "Write blocks" );

    Array2DImpl<int> iarr2d( dim1_, dim2_ );
    Array2DImpl<int> iarr2dx2( dim1_, (short)(2*dim2_) );
    HDF5::ArrayNDTool<int> iarrtoolx2( iarr2dx2 );
    HDF5::ArrayNDTool<int> iarrtool( iarr2d );
    dsky.setGroupName( "Slabby" );
    dsky.setDataSetName( "Slabby Data" );
    uirv = iarrtoolx2.createDataSet( *wrr, dsky );
    mAddTestResult( "Create Slabby DataSet" );
    HDF5::SlabSpec slabspec; HDF5::SlabDimSpec dimspec;
    dimspec.start_ = 0; dimspec.step_ = 1; dimspec.count_ = dim1_;
    slabspec += dimspec;
    dimspec.start_ = 0; dimspec.step_ = 1; dimspec.count_ = dim2_;
    slabspec += dimspec;
    fillArr2D( iarr2d, 100 );
    uirv = iarrtool.putSlab( *wrr, slabspec );
    mAddTestResult( "Write Slabby First Slab" );
    slabspec[1].start_ = dim2_;
    fillArr2D( iarr2d, 100 + dim2_ );
    uirv = iarrtool.putSlab( *wrr, slabspec );
    mAddTestResult( "Write Slabby Second Slab" );

    dsky.setGroupName( "Component 1" );
    dsky.setDataSetName( "Apenoot" );
    mRunStandardTest( !wrr->setScope(dsky), "Set to non-existing scope" );
    dsky.setDataSetName( "Block [1]" );
    mRunStandardTest( wrr->setScope(dsky), "Set to existing scope" );
    iop.setEmpty();
    iop.set( "Appel", "peer" );
    uirv = wrr->putInfo( dsky, iop );
    mAddTestResult( "Write Comp1/Block1 attrib" );

    return true;
}


static bool testReadInfo( HDF5::Reader& rdr )
{
    BufferStringSet grps;
    rdr.getGroups( grps );
    mRunStandardTestWithError( grps.size()==3, "Groups in file",
			       BufferString("nrgrps=",grps.size()) );

    BufferStringSet dsnms;
    rdr.getDataSets( grps.get(0), dsnms );
    mRunStandardTestWithError( dsnms.size()==nrblocks_, "Datasets in group",
			       BufferString("nrblocks=",grps.size()) );

    HDF5::DataSetKey filedsky;
    bool scoperes = rdr.setScope( filedsky );
    mRunStandardTest( scoperes, "Set scope (file)" )

    IOPar iop;
    uiRetVal uirv = rdr.getInfo( iop );
    mAddTestResult( "Get file info" );

    const BufferString iopval = iop.find( "File attr" );
    mRunStandardTestWithError( iopval=="file attr value", "File info contents",
				BufferString("found: '",iopval,"'") );

    HDF5::DataSetKey dsky33( "Component 3", "Block [3]" );
    scoperes = rdr.setScope( dsky33 );
    mRunStandardTest( !scoperes, "Set scope (non-existing)" )

    dsky33 = HDF5::DataSetKey( "Component 2", "Block [3]" );
    scoperes = rdr.setScope( dsky33 );
    mRunStandardTest( scoperes, "Set scope (Comp2,Block3)" )

    PtrMan<ArrayNDInfo> arrinf = rdr.getDataSizes();
    mRunStandardTest( arrinf, "Get dimensions" )
    const int nrdims = arrinf->nrDims();
    mRunStandardTestWithError( nrdims==nrdims_, "Correct nr dimensions",
				BufferString("nrdims=",nrdims) )
    const int dim1 = arrinf->getSize( 0 );
    mRunStandardTestWithError( dim1==dim1_, "Correct size of 1st dim",
				BufferString("dim1=",dim1) )
    const int dim2 = arrinf->getSize( 1 );
    mRunStandardTestWithError( dim2==dim2_, "Correct size of 2nd dim",
				BufferString("dim2=",dim2) )

    iop.setEmpty();
    uirv = rdr.getInfo( iop );
    mAddTestResult( "Get dataset info" );
    int iblk=0, icomp=0;
    iop.get( sPropNm, iblk, icomp );
    mRunStandardTestWithError( iblk==3 && icomp==2, "Dataset info contents",
		BufferString("iblk=",iblk).add(" icomp=").add(icomp) );

    iop.setEmpty();
    HDF5::DataSetKey dsky11( "Component 1", "Block [1]" );
    scoperes = rdr.setScope( dsky11 );
    mAddTestResult( "Set scope to Comp1/Block1" );
    uirv = rdr.getInfo( iop );
    const BufferString iopres( iop.find( "Appel" ) );
    iop.set( "Appel", "peer" );
    mRunStandardTestWithError( iopres=="peer", "Attr value in Comp1/Block1",
				BufferString("iopres=",iopres) );

    scoperes = rdr.setScope( dsky33 );
    mRunStandardTest( scoperes, "Set scope back to (Comp2,Block3)" )

    return true;
}


static bool testReadData( HDF5::Reader& rdr )
{
    Array2DImpl<float> arr2d( dim1_, dim2_ );
    uiRetVal uirv = rdr.getAll( arr2d.getData() );
    mAddTestResult( "Get entire block data" );
    const float arrval = arr2d.get( 6, 15 );
    mRunStandardTestWithError( arrval==30615.f, "Correct value [comp2,6,15]",
				BufferString("arrval=",arrval) )

    TypeSet<HDF5::Reader::IdxType> poss;
    poss += 7; poss += 16;
    float val = 0.f;
    uirv = rdr.getPoint( poss.arr(), &val );
    mAddTestResult( "Get single point value" );
    mRunStandardTestWithError( val==30716.f, "Correct value [comp2,7,16]",
				BufferString("val=",val) )

    const int nrpts = 3;
    HDF5::Reader::NDPosBufSet positions;
    for ( int ipt=0; ipt<nrpts; ipt++ )
    {
	mDefNDPosBuf( pos, nrdims_ );
	for ( int idim=0; idim<nrdims_; idim++ )
	    pos[idim] = idim*5 + ipt;
	positions += pos;
    }

    TypeSet<float> ptvals( nrpts, 0.f );
    uirv = rdr.getPoints( positions, ptvals.arr() );
    mAddTestResult( "Get multi point values" );
    mRunStandardTestWithError( ptvals[1]==30106.f, "Correct value [comp2,1,6]",
				BufferString("ptvals[1]=",ptvals[1]) )

    const int nrdim1 = 3; const int nrdim2 = 4;
    TypeSet<float> slabvals( nrdim1*nrdim2, 0.f );
    HDF5::SlabSpec slabspec; HDF5::SlabDimSpec dimspec;
    dimspec.start_ = 1; dimspec.step_ = 2; dimspec.count_ = nrdim1;
    slabspec += dimspec;
    DBG::setCrashOnProgError( false );
    uirv = rdr.getSlab( slabspec, slabvals.arr() );
    DBG::setCrashOnProgError( true );
    mRunStandardTest( !uirv.isOK(), "Not accept incorrect SlabSpec" )

    dimspec.start_ = 5; dimspec.step_ = 1; dimspec.count_ = nrdim2;
    slabspec += dimspec;
    uirv = rdr.getSlab( slabspec, slabvals.arr() );
    mAddTestResult( "Get slab values" );

    mRunStandardTestWithError( slabvals[0]==30105.f, "Correct slab value [0,0]",
				BufferString("slabvals[1]=",slabvals[0]) )
    mRunStandardTestWithError( slabvals[3]==30108.f, "Correct slab value [0,3]",
				BufferString("slabvals[2]=",slabvals[2]) )
    mRunStandardTestWithError( slabvals[8]==30505.f, "Correct slab value [2,0]",
				BufferString("slabvals[9]=",slabvals[9]) )
    mRunStandardTestWithError( slabvals[11]==30508.f,"Correct slab value [2,3]",
				BufferString("slabvals[11]=",slabvals[11]) )

    dimspec.start_ = 2; dimspec.step_ = 3; dimspec.count_ = 100;
    slabspec += dimspec;
    const char* slabspecmsg = "Should have pErrMsg but no error";
    try {
	uirv = rdr.getSlab( slabspec, slabvals.arr() );
	mRunStandardTest( false, slabspecmsg )
    } catch ( ... )
    {
	mRunStandardTest( true, slabspecmsg )
    }
    mAddTestResult( "Get slab values again" );

    const HDF5::DataSetKey dsky( "Slabby", "Slabby Data" );
    bool scoperes = rdr.setScope( dsky );
    mRunStandardTest( scoperes, "Set scope (Slabby)" )
    Array2DImpl<int> iarr2dx2( dim1_, (short)2*dim2_ );
    uirv = rdr.getAll( const_cast<int*>(iarr2dx2.getData()) );
    mAddTestResult( "Get Slabby values" );
    const int v3_11 = iarr2dx2.get( 3, 11 );
    const int v3_31 = iarr2dx2.get( 3, 31 );
    mRunStandardTestWithError( v3_11==411, "Correct Slabby value [3,11]",
				BufferString("v3_11=",v3_11) )
    mRunStandardTestWithError( v3_31==431,"Correct Slabby value [3,31]",
				BufferString("v3_31=",v3_31) )

    return true;
}


static bool testRead()
{
    PtrMan<HDF5::Reader> rdr = HDF5::mkReader();
    mRunStandardTest( rdr, "Get Reader" );

    uiRetVal uirv = rdr->open( filename_ );
    mAddTestResult( "Open file for read" );

    return testReadInfo(*rdr) && testReadData(*rdr);
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
