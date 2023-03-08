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

static uiRetVal testCreateLargeDS( const HDF5::DataSetKey& grpdsky,
				   HDF5::Writer& wrr )
{
    uiRetVal uirv;
    const int nrbatch = 5000;
    const int nrattribs = 2;
    const int nrinl = 8;
    const int nrcrl = 16;
    const int nrz = 32;
    TypeSet<int> arrdims;
    arrdims += nrbatch;
    arrdims += nrattribs;
    arrdims += nrinl;
    arrdims += nrcrl;
    arrdims += nrz;
    PtrMan<ArrayNDInfo> allinfo =
	   ArrayNDInfoImpl::create( arrdims.arr(), arrdims.size() );
    Array3DImpl<float> arr( nrinl, nrcrl, nrz );
    if ( !arr.isOK() )
    {
	uirv.add( uiStrings::phrCannotAllocateMemory(
		  arr.totalSize()*sizeof(float)) );
	return uirv;
    }

    HDF5::DataSetKey dsky( grpdsky.fullDataSetName(), "BigData" );
    dsky.setMaximumSize( 0, 5 );
    uirv = wrr.createDataSet( dsky, *allinfo, OD::F32 );
    if ( !uirv.isOK() )
	return uirv;

    HDF5::SlabSpec slabspec;
    HDF5::SlabDimSpec batchspec; batchspec.count_ = 1;
    HDF5::SlabDimSpec attribspec; attribspec.count_ = 1;
    slabspec += batchspec;
    slabspec += attribspec;
    slabspec += HDF5::SlabDimSpec();
    slabspec += HDF5::SlabDimSpec();
    slabspec += HDF5::SlabDimSpec();

    const float* arrptr = arr.getData();
    //Filling only the first 300 rows, all rows > 15 will get cropped anyway
    for ( int ibatch=0; ibatch<300; ibatch++ )
    {
	slabspec[0].start_ = ibatch;
	for ( int iattr=0; iattr<nrattribs; iattr++ )
	{
	    slabspec[1].start_ = iattr;
	    const float fact = iattr == 0 ? 1.f : -1.f;
	    for ( int idx=0; idx<nrinl; idx++ )
		for ( int idy=0; idy<nrcrl; idy++ )
		    for ( int idz=0; idz<nrz; idz++ )
		    {
			arr.set( idx, idy, idz,
				 fact*arr.info().getOffset(idx,idy,idz) );
		    }
	    uirv = wrr.putSlab( dsky, slabspec, arrptr );
	    if ( !uirv.isOK() )
		return uirv;
	}
    }

    const int croppedsz = 15;
    arrdims[0] = croppedsz;
    allinfo = ArrayNDInfoImpl::create( arrdims.arr(), arrdims.size() );
    uirv = wrr.resizeDataSet( dsky, *allinfo );
    //Will crop the dataset, but keeps the remaining data

    //Now let's append new data
    const int nraddedrows = 10;
    arrdims[0] += nraddedrows;
    allinfo = ArrayNDInfoImpl::create( arrdims.arr(), arrdims.size() );
    uirv = wrr.resizeDataSet( dsky, *allinfo );
    /*Filling all new rows, except the last.
      If it is still uninitialized, crop+append succeeded */
    for ( int ibatch=croppedsz; ibatch<arrdims[0]-1; ibatch++ )
    {
	slabspec[0].start_ = ibatch;
	for ( int iattr=0; iattr<nrattribs; iattr++ )
	{
	    slabspec[1].start_ = iattr;
	    const float fact = iattr == 0 ? 1.f : -1.f;
	    for ( int idx=0; idx<nrinl; idx++ )
		for ( int idy=0; idy<nrcrl; idy++ )
		    for ( int idz=0; idz<nrz; idz++ )
		    {
			arr.set( idx, idy, idz,
				 fact*arr.info().getOffset(idx,idy,idz) );
		    }
	    uirv = wrr.putSlab( dsky, slabspec, arrptr );
	    if ( !uirv.isOK() )
		return uirv;
	}
    }

    return uirv;
}


static bool testReadInfo( HDF5::Reader& rdr )
{
    BufferStringSet grps;
    rdr.getGroups( grps );
    mRunStandardTestWithError( grps.size()==6, "Groups in file",
			       BufferString("nrgrps=",grps.size()) );

    BufferStringSet dsnms;
    rdr.getDataSets( grps.get(0), dsnms );
    mRunStandardTestWithError( dsnms.size()==nrblocks_, "Datasets in group",
			       BufferString("nrblocks=",grps.size()) );

    mRunStandardTest( rdr.hasGroup(nullptr), "Root as current group" )

    IOPar iop;
    uiRetVal uirv = rdr.get( iop );
    mAddTestResult( "Get file info from root attributes" );

    const BufferString iopval = iop.find( "File attr" );
    mRunStandardTestWithError( iopval=="file attr value", "File info contents",
				BufferString("found: '",iopval,"'") );

    BufferString attrstr;
    mRunStandardTest( !rdr.hasAttribute("not an attribute"),
		      "Does not have non-existing top level attribute" )
    mRunStandardTest( rdr.hasAttribute("File attribute"),
		      "Has top level attribute" )
    mRunStandardTest( rdr.getAttribute("File attribute",attrstr) &&
		      attrstr == "New attribute value",
		      "Can read updated top level attribute value" )
    mRunStandardTest( rdr.hasAttribute("Empty attribute 1") &&
		      rdr.hasAttribute("Empty attribute 2") &&
		      rdr.hasAttribute("Empty attribute 3"),
		      "Has top level empty attributes" )
    attrstr.setEmpty();
    mRunStandardTest( rdr.getAttribute("Empty attribute 1",attrstr) &&
		      attrstr.isEmpty(), "Can read empty attribute string" )
    attrstr.setEmpty();
    mRunStandardTest( rdr.getAttribute("Empty attribute 2",attrstr) &&
		      attrstr.isEmpty(), "Can read empty attribute string" )
    attrstr.setEmpty();
    mRunStandardTest( rdr.getAttribute("Empty attribute 3",attrstr) &&
		      attrstr.isEmpty(), "Can read empty attribute string" )
    mRunStandardTest( !rdr.hasAttribute( "Attrib to be deleted" ),
		    "Does not have a removed attribute" )
    mRunStandardTest( rdr.getNrAttributes() == 5,
	    "Correct number of attributes found at top level: 5" )
    BufferStringSet attribnms;
    uirv = rdr.getAttributeNames( attribnms );
    mAddTestResult( "Retrieve list of top level attribute names" )
    mRunStandardTest( attribnms.size() == 5 &&
		attribnms.get(4) == "File attribute",
		"Attribute names content test" )

    const HDF5::DataSetKey infods( "", "++info++" );
    mRunStandardTest( rdr.hasDataSet(infods), "Has ++info++ text dataset" )
    mRunStandardTest( rdr.getNrAttributes(&infods) == 0,
		    "No attributes found when none present" )

    const HDF5::DataSetKey dskyslb( "Slabby" );
    mRunStandardTest( !rdr.hasAttribute("not an attribute",&dskyslb),
		      "Does not have non-existing group attribute" )
    mRunStandardTest( rdr.hasAttribute("slabby key",&dskyslb),
		      "Has group attribute" )
    attrstr.setEmpty();
    mRunStandardTest( rdr.getAttribute("slabby key",attrstr,&dskyslb) &&
		      attrstr == "New slabby value",
		      "Can read updated group attribute value" )

    const HDF5::DataSetKey dsky23( "Component 2", "Block [3]" );
    iop.setEmpty();
    uirv = rdr.get( iop, &dsky23 );
    mAddTestResult( "Get dataset info" );
    int iblk=0, icomp=0;
    iop.get( sPropNm, iblk, icomp );
    mRunStandardTestWithError( iblk==3 && icomp==2, "Dataset info contents",
		BufferString("iblk=",iblk).add(" icomp=").add(icomp) );

    mRunStandardTest( !rdr.hasAttribute("not an attribute",&dsky23),
		      "Does not have non-existing dataset attribute" )
    mRunStandardTest( rdr.hasAttribute(sPropNm,&dsky23),
		      "Has dataset attribute" )
    attrstr.setEmpty();
    mRunStandardTest( rdr.getAttribute(sPropNm,attrstr,&dsky23) &&
		      attrstr == "3`2",
		      "Can read dataset attribute value" )

    iop.setEmpty();
    const HDF5::DataSetKey dsky11( "Component 1", "Block [1]" );
    uirv = rdr.get( iop, &dsky11 );
    const BufferString iopres( iop.find( "Appel" ) );
    iop.set( "Appel", "peer" );
    mRunStandardTestWithError( iopres=="peer", "Attr value in Comp1/Block1",
				BufferString("iopres=",iopres) );

    return true;
}


static bool testReadData( HDF5::Reader& rdr )
{
    HDF5::DataSetKey dsky( "Component 3", "Block [3]" );
    bool scoperes = rdr.hasDataSet( dsky );
    mRunStandardTest( !scoperes, "Does not have non-existing group" )

    dsky = HDF5::DataSetKey( "Component 2", "Block [10]" );
    scoperes = rdr.hasDataSet( dsky );
    mRunStandardTest( !scoperes, "Does not have non-existing dataset" )

    dsky = HDF5::DataSetKey( "Component 2", "Block [3]" );
    scoperes = rdr.hasDataSet( dsky );
    mRunStandardTest( scoperes, "Have existing dataset (Comp2,Block3)" )

    uiRetVal uirv;
    PtrMan<ArrayNDInfo> arrinf = rdr.getDataSizes( dsky, uirv );
    mRunStandardTest( arrinf, "Get dimensions" )
    const int nrdims = arrinf->getNDim();
    mRunStandardTestWithError( nrdims==nrdims_, "Correct nr dimensions",
				BufferString("nrdims=",nrdims) )
    const int dim1 = arrinf->getSize( 0 );
    mRunStandardTestWithError( dim1==dim1_, "Correct size of 1st dim",
				BufferString("dim1=",dim1) )
    const int dim2 = arrinf->getSize( 1 );
    mRunStandardTestWithError( dim2==dim2_, "Correct size of 2nd dim",
				BufferString("dim2=",dim2) )
    Array2DImpl<float> arr2d( dim1_, dim2_ );
    uirv = rdr.getAll( dsky, arr2d.getData() );
    mAddTestResult( "Get entire block data" );
    const float arrval = arr2d.get( 6, 15 );
    mRunStandardTestWithError( arrval==30615.f, "Correct value [comp2,6,15]",
				BufferString("arrval=",arrval) )

    TypeSet<HDF5::Reader::idx_type> poss;
    poss += 7; poss += 16;
    float val = 0.f;
    uirv = rdr.getValue( dsky, poss.arr(), &val );
    mAddTestResult( "Get single value" );
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
    uirv = rdr.getValues( dsky, positions, ptvals.arr() );
    mAddTestResult( "Get multiple values" );
    mRunStandardTestWithError( ptvals[1]==30106.f, "Correct value [comp2,1,6]",
				BufferString("ptvals[1]=",ptvals[1]) )

    const int nrdim1 = 3; const int nrdim2 = 4;
    float* valarr = new float [nrdim1*nrdim2];
    HDF5::SlabSpec slabspec; HDF5::SlabDimSpec dimspec;
    dimspec.start_ = 1; dimspec.step_ = 2; dimspec.count_ = nrdim1;
    slabspec += dimspec;
    DBG::setCrashOnProgError( false );
    uirv = rdr.getSlab( dsky, slabspec, valarr );
    DBG::setCrashOnProgError( true );
    mRunStandardTest( !uirv.isOK(), "Not accept incorrect SlabSpec" )

    dimspec.start_ = 5; dimspec.step_ = 1; dimspec.count_ = nrdim2;
    slabspec += dimspec;
    uirv = rdr.getSlab( dsky, slabspec, valarr );
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
    const bool onerror = DBG::setCrashOnProgError( false );
    uirv = rdr.getSlab( dsky, slabspec, valarr );
    mRunStandardTest( true, slabspecmsg )
    DBG::setCrashOnProgError( onerror );
    mAddTestResult( "Get slab values again" );

    dsky = HDF5::DataSetKey( "Slabby", "Slabby Data" );
    mRunStandardTest( rdr.hasDataSet(dsky), "Has dataset (Slabby)" )
    Array2DImpl<int> iarr2dx2( dim1_, (short)2*dim2_ );
    uirv = rdr.getAll( dsky, const_cast<int*>(iarr2dx2.getData()) );
    mAddTestResult( "Get Slabby values" );
    const int v3_11 = iarr2dx2.get( 3, 11 );
    const int v3_31 = iarr2dx2.get( 3, 31 );
    mRunStandardTestWithError( v3_11==411, "Correct Slabby value [3,11]",
				BufferString("v3_11=",v3_11) )
    mRunStandardTestWithError( v3_31==431,"Correct Slabby value [3,31]",
				BufferString("v3_31=",v3_31) )

    HDF5::DataSetKey dsky2( "", "ShortArr" );
    TypeSet<short> shortvals;
    uirv = rdr.get( dsky2, shortvals );
    mAddTestResult( BufferString( "Get ShortArr values",
				  dsky2.fullDataSetName()) );
    mRunStandardTestWithError( shortvals.size()==3, "Correct ShortArr size",
				BufferString("sz=",shortvals.size()) )
    mRunStandardTestWithError( shortvals[0]==10, "Correct ShortArr value [0]",
				BufferString("v[0]=",shortvals[0]) )

    dsky2.setDataSetName( "Strings" );
    BufferStringSet bss;
    uirv = rdr.get( dsky2, bss );
    mAddTestResult( BufferString( "Get Strings values for ",
				  dsky2.fullDataSetName()) );
    mRunStandardTestWithError( bss.get(0)=="Overwritten 1",
				"Correct Strings value [0]",
				BufferString("s[0]=",bss.get(0)) )
    mRunStandardTestWithError( bss.get(3).isEmpty(),
				"Correct Strings value [3]",
				BufferString("s[3]=",bss.get(3)) )

    const auto maindsky = HDF5::DataSetKey( "MainGroup" );
    mRunStandardTest( rdr.hasGroup( maindsky.groupName() ), "Has main group" );
    dsky = HDF5::DataSetKey::groupKey( maindsky, "GroupA" );
    mRunStandardTest( rdr.hasGroup( dsky.groupName() ), "Has sub-group A" );
    dsky = HDF5::DataSetKey::groupKey( maindsky.fullDataSetName(), "GroupB" );
    mRunStandardTest( rdr.hasGroup( dsky.groupName() ), "Has sub-group B" );

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

    Array2DImpl<float> arr2d( dim1_, dim2_ );
    IOPar iop;
    iop.set( "File attr", "file attr value" );
    uirv = wrr->set( iop );
    mAddTestResult( "Write file attribute" );
    wrr->setAttribute( "File attribute", "file attribute value" );
    wrr->setAttribute( "Empty attribute 1", nullptr );
    wrr->setAttribute( "Empty attribute 2", "" );
    wrr->setAttribute( "Empty attribute 3", "Will be edited" );
    wrr->setAttribute( "Attrib to be deleted", "" );

    HDF5::DataSetKey dsky;
    TypeSet<int> chunkszs( 3, chunksz_ );
    dsky.setChunkSize( chunkszs.arr(), chunkszs.size() );
    BufferStringSet compnms;
    compnms.add( "Component 1" ).add( "Component 2" );

    iop.setEmpty();
    HDF5::ArrayNDTool<float> arrtool( arr2d );
    iop.set( "Apenoot", "pere boom" );
    for ( int iblk=0; iblk<nrblocks_; iblk++ )
    {
	dsky.setDataSetName( BufferString( "Block [", iblk, "]" ) );

	fillArr2D( arr2d, 1000*iblk );
	dsky.setGroupName( compnms.first()->str() );
	uirv = arrtool.put( *wrr, dsky );
	if ( !uirv.isOK() )
	    break;
	iop.set( sPropNm, iblk, 1 );
	uirv = wrr->set( iop, &dsky );
	if ( !uirv.isOK() )
	    break;

	fillArr2D( arr2d, 10000*iblk );
	dsky.setGroupName( compnms.last()->str() );
	uirv = arrtool.put( *wrr, dsky );
	if ( !uirv.isOK() )
	    break;
	iop.set( sPropNm, iblk, 2 );
	uirv = wrr->set( iop, &dsky );
	if ( !uirv.isOK() )
	    break;
    }
    mAddTestResult( "Write blocks" );

    for ( const auto compnm : compnms )
    {
	dsky = HDF5::DataSetKey( compnm->str() );
	wrr->setAttribute( "key string", "value string", &dsky );
	wrr->setAttribute( "a float", 30.25f, &dsky );
	wrr->setAttribute( "a double", 30.25, &dsky );
	wrr->setAttribute( "an integer", -30, &dsky );
	wrr->setAttribute( "an unsigned short", 30, &dsky );
    }

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
    uirv = iarrtool.putSlab( *wrr, dsky, slabspec );
    mAddTestResult( "Write Slabby First Slab" );
    slabspec[1].start_ = dim2_;
    fillArr2D( iarr2d, 100 + dim2_ );
    uirv = iarrtool.putSlab( *wrr, dsky, slabspec );
    mAddTestResult( "Write Slabby Second Slab" );
    dsky.setDataSetName( "" );
    wrr->setAttribute( "slabby key", "slabby value", &dsky );

    mRunStandardTest( !wrr->hasGroup("Yo"), "Does not have an invalid group" );
    dsky.setGroupName( "Component 1" );
    dsky.setDataSetName( "Apenoot" );
    mRunStandardTest( !wrr->hasDataSet(dsky),
		      "Does not have an invalid dataset" );

    dsky.setDataSetName( "Block [1]" );
    iop.setEmpty();
    iop.set( "Appel", "peer" );
    uirv = wrr->set( iop, &dsky );
    mAddTestResult( "Write Comp1/Block1 attrib using an IOPar" );

    const auto maindsky = HDF5::DataSetKey( "MainGroup" );
    wrr->ensureGroup( maindsky.groupName(), uirv );
    mAddTestResult( "Create MainGroup group" );
    dsky = HDF5::DataSetKey::groupKey( maindsky, "GroupA" );
    wrr->ensureGroup( dsky.groupName(), uirv );
    mAddTestResult( "Create sub-group A" );
    uirv = testCreateLargeDS( dsky, *wrr );
    mAddTestResult( "Create Large Dataset in sub-group A" )
    dsky = HDF5::DataSetKey::groupKey( maindsky.fullDataSetName(), "GroupB" );
    wrr->ensureGroup( dsky.groupName(), uirv );
    mAddTestResult( "Create sub-group B" );

    //Editable
    dsky.setEditable( true );
    dsky.setChunkSize( nullptr );

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

    dsky.setGroupName( "" ).setDataSetName( "++info++" );
    uirv = wrr->createTextDataSet( dsky );
    mAddTestResult( "Created a text dataset" );
    wrr->setAttribute( "Attrib 1 to be removed", "", &dsky );
    wrr->setAttribute( "Attrib 2 to be removed", "", &dsky );
    wrr->setAttribute( "Attrib 3 to be removed", "", &dsky );

    return true;
}


static bool testEdit()
{
    PtrMan<HDF5::Writer> wrr = HDF5::mkWriter();
    mRunStandardTest( wrr, "Get Writer for edit" );
    uiRetVal uirv = wrr->open4Edit( filename_ );
    mAddTestResult( "Open file for edit" );

    PtrMan<HDF5::Reader> rdr = wrr->createCoupledReader();
    mRunStandardTest( rdr.ptr(), "Can get coupled reader" )
    mRunStandardTest( rdr->hasAttribute("Empty attribute 3"),
		      "Has top level attribute to set empty" )
    rdr = nullptr;
    wrr->setAttribute( "Empty attribute 3", BufferString::empty() );
    wrr->removeAttribute( "Attrib to be deleted" );

    HDF5::DataSetKey dsky( "", "ShortArr" );
    mRunStandardTest( wrr->hasDataSet(dsky), "Has dataset ShortArr" )
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

    wrr->setAttribute( "File attribute", "New attribute value" );
    dsky.setGroupName( "Slabby" ).setDataSetName( "" );
    wrr->setAttribute( "slabby key", "New slabby value", &dsky );

    dsky.setGroupName( "" ).setDataSetName( "++info++" );
    uirv = wrr->removeAllAttributes( &dsky );
    mAddTestResult( "Removed all attributes of dataset ++info++" );

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
    const char* fnm = "ORG_420-430_500-600_500-1500_HDF.h5";
    FilePath fp( GetDataDir(), "Seismics", fnm );
    const BufferString fullfnm( fp.fullPath() );
    if ( !File::exists(fullfnm) )
	return true;

    PtrMan<HDF5::Reader> rdr = HDF5::mkReader();
    mRunStandardTest( rdr, "Get Reader for small cube" );

    uiRetVal uirv = rdr->open( fullfnm );
    mAddTestResult( "Open small cube for read" );

    const HDF5::DataSetKey dsky( "Component 1", "5.3" );
    mRunStandardTest( rdr->hasDataSet(dsky), "Has dataset (Small Cube)" )

    HDF5::SlabSpec slabspec( 3 );
    slabspec[0].count_ = slabspec[1].count_ = 1;
    slabspec[2].count_ = 251;
    short* data = new short [slabspec[2].count_];
    for ( int isamp=0; isamp<25; isamp++ )
	data[isamp] = -999;

    uirv = rdr->getSlab( dsky, slabspec, data );
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
    uirv = rdr->getSlab( dsky, slabspec, data );
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
    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "General" );
    PIM().loadAuto( true );

    if ( !HDF5::isAvailable() )
    {
	tstStream( true ) << "HDF5 not available" << od_endl;
	return 1;
    }

    filename_.set( FilePath(File::getTempPath(),"test.h5").fullPath() );
    if ( File::exists(filename_) && !File::remove(filename_) )
	return 1;

    if ( !testSmallCube() )
	return 1;

    if ( !testWrite()
      || !testEdit()
      || !testRead() )
	return 1;

    if ( !clParser().hasKey("keep") )
        File::remove( filename_.buf() );

    return 0;
}
