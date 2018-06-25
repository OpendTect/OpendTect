/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : March 2018
-*/


#include "hdf5arraynd.h"
#include "testprog.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "arrayndimpl.h"
#include "iopar.h"
#include "moddepmgr.h"
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
	    arr2d.set( idx0, idx1, mCast(T,100.*idx0 + shft + idx1) );
    }
}


#define mAddTestResult(desc) \
    mRunStandardTestWithError( uirv.isOK(), desc, toString(uirv) )


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
    float* valarr = new float [nrdim1*nrdim2];
    HDF5::SlabSpec slabspec; HDF5::SlabDimSpec dimspec;
    dimspec.start_ = 1; dimspec.step_ = 2; dimspec.count_ = nrdim1;
    slabspec += dimspec;
    DBG::setCrashOnProgError( false );
    uirv = rdr.getSlab( slabspec, valarr );
    DBG::setCrashOnProgError( true );
    mRunStandardTest( !uirv.isOK(), "Not accept incorrect SlabSpec" )

    dimspec.start_ = 5; dimspec.step_ = 1; dimspec.count_ = nrdim2;
    slabspec += dimspec;
    uirv = rdr.getSlab( slabspec, valarr );
    mAddTestResult( "Get slab values" );

    mRunStandardTestWithError( valarr[0]==30105.f,
			"Correct slab value [0,0]",
			BufferString("valarr[0]=",valarr[0]) )
    mRunStandardTestWithError( valarr[3]==30108.f,
			"Correct slab value [0,3]",
			BufferString("valarr[3]=",valarr[3]) )
    mRunStandardTestWithError( valarr[8]==30505.f,
			"Correct slab value [2,0]",
			BufferString("valarr[8]=",valarr[8]) )
    mRunStandardTestWithError( valarr[11]==30508.f,
			"Correct slab value [2,3]",
			BufferString("valarr[11]=",valarr[11]) )

    dimspec.start_ = 2; dimspec.step_ = 3; dimspec.count_ = 100;
    slabspec += dimspec;
    const char* slabspecmsg = "Should have pErrMsg but no error";
    try {
	uirv = rdr.getSlab( slabspec, valarr );
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

    HDF5::DataSetKey dsky2( "", "ShortArr" );
    scoperes = rdr.setScope( dsky2 );
    mRunStandardTest( scoperes, "Set scope (ShortArr)" )
    TypeSet<short> shortvals;
    uirv = rdr.get( shortvals );
    mAddTestResult( "Get ShortArr values" );
    mRunStandardTestWithError( shortvals.size()==3, "Correct ShortArr size",
				BufferString("sz=",shortvals.size()) )
    mRunStandardTestWithError( shortvals[0]==10, "Correct ShortArr value [0]",
				BufferString("v[0]=",shortvals[0]) )

    dsky2.setDataSetName( "Strings" );
    BufferStringSet bss;
    scoperes = rdr.setScope( dsky2 );
    mRunStandardTest( scoperes, "Set scope (Strings)" )
    uirv = rdr.get( bss );
    mAddTestResult( "Get Strings values" );
    mRunStandardTestWithError( bss.get(0)=="Overwritten 1",
				"Correct Strings value [0]",
				BufferString("s[0]=",bss.get(0)) )
    mRunStandardTestWithError( bss.get(3).isEmpty(),
				"Correct Strings value [3]",
				BufferString("s[3]=",bss.get(3)) )

    return true;
}


static bool testReadFrom( HDF5::Reader& rdr )
{
    return testReadInfo(rdr) && testReadData(rdr);
}


static bool testRead()
{
    PtrMan<HDF5::Reader> rdr = HDF5::mkReader();
    mRunStandardTest( rdr, "Get Reader" );
    uiRetVal uirv = rdr->open( filename_ );
    mAddTestResult( "Open file for read" );
    return testReadFrom( *rdr );
}


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

    //Editable
    wrr->setEditableCreation( true );

    TypeSet<short> ts;
    ts += 1; ts += 2; ts += 3; ts += 4;
    dsky.setGroupName( "" );
    dsky.setDataSetName( "ShortArr" );
    uirv = wrr->put( dsky, ts );
    mAddTestResult( "Write TypeSet" );

    BufferStringSet bss;
    bss.add( "Str 1" );
    bss.add( "Str 2" );
    bss.add( "String 3" );
    bss.add( "" );
    bss.add( "Str5/Str 4 is empty" );
    dsky.setDataSetName( "Strings" );
    uirv = wrr->put( dsky, bss );
    mAddTestResult( "Write BufferStringSet" );

    return true;
}


static bool testEdit()
{
    PtrMan<HDF5::Writer> wrr = HDF5::mkWriter();
    mRunStandardTest( wrr, "Get Writer for edit" );
    uiRetVal uirv = wrr->open4Edit( filename_ );
    mAddTestResult( "Open file for edit" );

    HDF5::DataSetKey dsky( "", "ShortArr" );
    TypeSet<short> ts;
    ts += 10; ts += 20; ts += 30;
    uirv = wrr->resizeDataSet( dsky, Array1DInfoImpl(3) );
    mAddTestResult( "Shrink ShortArr's DataSet" );
    uirv = wrr->put( dsky, ts );
    mAddTestResult( "Write shrunk ShortArr" );


    dsky.setDataSetName( "Strings" );
    BufferStringSet bss;
    bss.add( "Overwritten 1" ).add( "Overwritten 2" ).add( "Overwritten 3" );
    bss.add( "" ).add( "Prev is empty" ).add( "Total 6 strings" );
    uirv = wrr->put( dsky, bss );
    mAddTestResult( "Overwrite BufferStringSet with larger" );

    return true;
}


static bool sampsOK( const short* data, const short* expected, int nrsamps )
{
    bool allok = true;
    for ( int isamp=0; isamp<nrsamps; isamp++ )
    {
	if ( data[isamp] != expected[isamp] )
	{
	    tstStream() << isamp << "-> " << data[isamp]
		<< " should be " << expected[isamp] << od_endl;
	    allok = false;
	}
    }
    return allok;
}


static bool testSmallCube()
{
    const char* fnm = "ORG_420-430_500-600_500-1500_HDF.hdf5";
    File::Path fp( GetDataDir(), "Seismics", fnm );
    const BufferString fullfnm( fp.fullPath() );
    if ( !File::exists(fullfnm) )
	return true;

    PtrMan<HDF5::Reader> rdr = HDF5::mkReader();
    mRunStandardTest( rdr, "Get Reader for small cube" );

    uiRetVal uirv = rdr->open( fullfnm );
    mAddTestResult( "Open small cube for read" );

    const HDF5::DataSetKey dsky( "Component 1", "5.3" );
    mRunStandardTest( rdr->setScope(dsky), "Set scope (Small Cube)" )

    HDF5::SlabSpec slabspec( 3 );
    slabspec[0].count_ = slabspec[1].count_ = 1;
    slabspec[2].count_ = 251;
    short* data = new short [slabspec[2].count_];
    for ( int isamp=0; isamp<25; isamp++ )
	data[isamp] = -999;

    uirv = rdr->getSlab( slabspec, data );
    mAddTestResult( "Read entire first trace in block" );

    const short expected_0_0[25] = {
	3786, -3128, -2840, 3555, -2726, -8163, 3820, 9823, -1327, -5408,
	686, -142, -3595, -1721, 21, 1817, 4446, 2600, 59, 437,
	-2379, -5331, -775, 3593, 1407 };
    bool sampsok1 = sampsOK( data, expected_0_0, 25 );

    slabspec[0].start_ = 2;
    slabspec[1].start_ = 1;
    slabspec[2].start_ = 10;
    slabspec[2].count_ = 10;
    uirv = rdr->getSlab( slabspec, data );
    mAddTestResult( "Read another trace (+2 inls, +1 crl, +10 samps)" );
    const short expected_2_1[10] = {
	2559, -1551, -4620, -2825, -571, 2442, 4799, 3742, 2602, 892 };
    bool sampsok2 = sampsOK( data, expected_2_1, 10 );

    mRunStandardTest( sampsok1, "Sample values @ 0/0/0" )
    mRunStandardTest( sampsok2, "Sample values @ 2/1/10" )

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded("General");
    PIM().loadAuto( false );

    if ( !HDF5::isAvailable() )
    {
	tstStream( true ) << "HDF5 not available" << od_endl;
	return 0;
    }

    filename_.set( File::Path(File::getTempPath(),"test.hd5").fullPath() );
    if ( !testSmallCube() )
	return 1;

    if ( !testWrite()
      || !testEdit()
      || !testRead() )
	return 1;

    return 0;
}
