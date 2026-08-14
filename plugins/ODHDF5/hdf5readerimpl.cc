/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5readerimpl.h"

#include "arrayndinfo.h"
#include "file.h"
#include "iopar.h"
#include "od_ostream.h"
#include "odjson.h"
#include "uistrings.h"


#define mCatchErrDuringRead() \
    mCatchAdd2uiRv( uiStrings::phrErrDuringRead(fileName()) )


HDF5::ReaderImpl::ReaderImpl()
    : AccessImpl(*this)
{
    myfile_ = false;
}


HDF5::ReaderImpl::ReaderImpl( const FileID& h5file )
    : AccessImpl(*this)

{
    myfile_ = false;
    fileid_ =  h5file;
}


HDF5::ReaderImpl::~ReaderImpl()
{
    closeFile();
}


void HDF5::ReaderImpl::openFile( const char* fnm, uiRetVal& uirv, bool )
{
    Threads::Locker locker( lock_ );
    if ( !File::exists(fnm) )
    {
	uirv.add( uiStrings::phrCannotOpenForRead(fnm) );
	return;
    }

    closeFile();
    ::hid_t fid = -1;

    {
	fid = H5Fopen( fnm, H5F_ACC_RDONLY,
		       H5P_DEFAULT );
    }

    if ( fid < 0 )
    {
	uirv.add( uiStrings::phrCannotOpenForRead(fnm) );
	od_cout() << "The HDF5 plugin is not applied" << od_endl;
	return;
    }

    myfile_ = true;
    fileid_ = FileID::get( mCast(hid_t, fid) );

#ifdef __debug__
    if ( DBG::isOn(DGB_HDF5) )
    {
	od_cout() << "Open: " << fileid_.asInt() << " "
		  << fnm << od_endl;
    }
#endif
}


void HDF5::ReaderImpl::listObjs( const GroupID& dir,
				      BufferStringSet& nms,
				      bool wantgroups ) const
{
    if ( !dir.isValid() )
	return;

    const ::hid_t dirid = dir.asInt();
    const bool islevel0 = nms.isEmpty();
    H5G_info_t ginfo;
    if ( H5Gget_info(dirid, &ginfo) < 0 )
	return;

    const hsize_t nrobjs = ginfo.nlinks;
    for ( hsize_t iobj = 0; iobj < nrobjs; iobj++ )
    {
	ssize_t name_len = H5Lget_name_by_idx( dirid, ".",
			       H5_INDEX_NAME, H5_ITER_NATIVE,
			       iobj, nullptr, 0,
			       H5P_DEFAULT );
	if ( name_len < 0 )
	    continue;

	std::vector<char> name_buf( name_len + 1 );
	if ( H5Lget_name_by_idx(dirid, ".", H5_INDEX_NAME,
			       H5_ITER_NATIVE, iobj,
			       name_buf.data(), name_len + 1,
			       H5P_DEFAULT) < 0 )
	    continue;

	const BufferString nm( name_buf.data() );
	H5O_info_t oinfo;
	if ( H5Oget_info_by_name(dirid, name_buf.data(), &oinfo,
	     H5O_INFO_BASIC, H5P_DEFAULT) < 0 )
	    continue;

	const H5O_type_t h5objtyp = oinfo.type;
	if ( (wantgroups && h5objtyp != H5O_TYPE_GROUP) ||
	    (!wantgroups && h5objtyp != H5O_TYPE_DATASET) )
	    continue;

	nms.add( wantgroups && islevel0 ? BufferString("/", nm) : nm );

	if ( wantgroups )
	{
	    const ::hid_t grp_hid = H5Gopen2( dirid, name_buf.data(),
					      H5P_DEFAULT );
	    if ( grp_hid < 0 )
		continue;

	    const GroupID grp = GroupID::get( mCast(hid_t, grp_hid) );
	    BufferStringSet subnms;
	    listObjs( grp, subnms, true );

	    for ( int idx = 0; idx < subnms.size(); idx++ )
	    {
		BufferString fullname = "/";
		fullname += nm;
		fullname += "/";
		fullname += subnms.get( idx );
		while ( fullname.contains("//") )
		    fullname.replace( "//", "/" );
		nms.add( fullname );
	    }
	    H5Gclose( grp_hid );
	}
    }
}

void HDF5::ReaderImpl::getGroups( BufferStringSet& nms ) const
{
    Threads::Locker locker( lock_ );
    nms.setEmpty();
    if ( !fileid_.isValid() )
	return;

    listObjs( GroupID::get( mCast(hid_t, fileid_.asInt())), nms,
	      true);
}


void HDF5::ReaderImpl::getSubGroups( const char* grpnm,
				     BufferStringSet& nms ) const
{
    Threads::Locker locker( lock_ );
    const GroupID group = selectGroup( grpnm );
    if ( !group.isValid() )
	return;

    nms.setEmpty();
    listObjs( group, nms, true );
}

void HDF5::ReaderImpl::getDataSets( const char* grpnm,
				    BufferStringSet& nms ) const
{
    Threads::Locker locker( lock_ );
    const GroupID group = selectGroup( grpnm );
    if ( !group.isValid() )
	return;

    nms.setEmpty();
    listObjs( group, nms, false );
}


void HDF5::ReaderImpl::gtComment( const LocationID& h5loc, const char* name,
				  BufferString& txt, uiRetVal& uirv ) const
{
    Threads::Locker locker( lock_ );
    if ( !h5loc.isValid() || !name || !*name )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return;
    }

    const size_t bufsz = 2048;
    BufferString buf;
    buf.setBufSize( bufsz );
    BufferString nm;
    if ( *name == '/' )
    {
	nm.set( name );
    }

    else
    {
	nm.set( "/" );
	nm.add( name );
    }

    const ssize_t outsz = H5Oget_comment_by_name( h5loc.asInt(),
						  nm.buf(),
						  buf.getCStr(),
						  bufsz,
						  H5P_DEFAULT );
    if ( outsz < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return;
    }

    txt = buf;
}


unsigned HDF5::ReaderImpl::gtVersion( const ObjectID& h5obj,
				      uiRetVal& uirv ) const
{
    Threads::Locker locker( lock_ );
    if ( !h5obj.isValid() )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return 0;
    }

    const ::hid_t oid = h5obj.asInt();
    if ( !H5Iis_valid(oid) )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return 0;
    }

    unsigned version = 0;
#if H5_VERSION_GE(1,12,0)
    H5O_native_info_t ninfo;
    if ( H5Oget_native_info(oid, &ninfo,
			    H5O_INFO_HDR) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return 0;
    }

    version =ninfo.hdr.version;
#else
    H5O_info_t info;
    if ( H5Oget_info1(oid, &info) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return 0;
    }

    version =info.hdr.version;
#endif
    return version;
}

HDF5::DatatypeID HDF5::ReaderImpl::h5DataType(
    const DatasetID& h5ds ) const
{
    Threads::Locker locker( lock_ );
    if ( !h5ds.isValid() )
	return DatatypeID::get( H5I_INVALID_HID );

    const ::hid_t dt = H5Dget_type( h5ds.asInt() );
    if ( dt < 0 )
	return DatatypeID::get( H5I_INVALID_HID );

    const DatatypeID ret = DatatypeID::get( mCast(hid_t, dt) );
    H5Tclose( dt );
    return ret;
}

HDF5::ODDataType HDF5::ReaderImpl::gtDataType( const DatasetID& h5ds ) const
{
    Threads::Locker locker( lock_ );
    ODDataType ret = OD::F32;
    if ( !h5ds.isValid() )
	return ret;

    const ::hid_t dt = H5Dget_type( h5ds.asInt() );
    if ( dt < 0 )
	return ret;

    const H5T_class_t cls = H5Tget_class( dt );
    const size_t sz = H5Tget_size( dt );
    bool issigned = true, isfp = true;
    if ( cls == H5T_INTEGER )
    {
	isfp = false;
	issigned = H5Tget_sign( dt ) != H5T_SGN_NONE;
    }

    else if ( cls == H5T_FLOAT )
	isfp = true;

    else
    {
	H5Tclose( dt );
	return ret;
    }

    ret = OD::GetDataRepType( isfp, issigned, sz );
    H5Tclose( dt );
    return ret;
}


ArrayNDInfo* HDF5::ReaderImpl::gtDataSizes( const DatasetID& h5ds ) const
{
    Threads::Locker locker( lock_ );
    if ( !h5ds.isValid() )
	return nullptr;

    const ::hid_t space = H5Dget_space( h5ds.asInt() );
    if ( space < 0 )
	return nullptr;

    const int nrdims = H5Sget_simple_extent_ndims( space );
    if ( nrdims < 1 )
    {
	H5Sclose( space );
	return nullptr;
    }

    TypeSet<hsize_t> dims( nrdims, 0 );
    if ( H5Sget_simple_extent_dims( space, dims.arr(),
	 nullptr ) < 0 )
    {
	H5Sclose( space );
	return nullptr;
    }


    H5Sclose( space );
    ArrayNDInfo* ret = ArrayNDInfoImpl::create( nrdims );
    for ( int idim=0; idim<nrdims; idim++ )
	ret->setSize( idim, (int)dims[idim] );
    return ret;
}

void HDF5::ReaderImpl::gtSlab( const DatasetID& h5ds,
			       const SlabSpec& spec,
			       void* data,
			       uiRetVal& uirv ) const
{
    Threads::Locker locker( lock_ );
    if ( !h5ds.isValid() || !data )
    {
	uirv.add( uiStrings::phrCannotOpenForRead(fileName()) );
	return;
    }

    const ::hid_t dsetid = h5ds.asInt();
    if ( dsetid < 0 || mIsUdf(dsetid) || H5Iis_valid(dsetid) <= 0 )
    {
	uirv.add( uiStrings::phrCannotOpenForRead(fileName()) );
	return;
    }

    const ::hid_t filespace = H5Dget_space( dsetid );
    if ( filespace < 0 )
    {
	uirv.add( uiStrings::phrCannotOpenForRead(fileName()) );
	return;
    }

    H5Sselect_all( filespace );
    TypeSet<hsize_t> counts;
    selectSlab( DataspaceID::get(mCast(hid_t,filespace)),
		spec, &counts );
    const ::hid_t memspace = H5Screate_simple( counts.size(),
					       counts.arr(),
					       nullptr );
    if ( memspace < 0 )
    {
	H5Sclose( filespace );
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return;
    }

    const ::hid_t dtype = H5Dget_type( dsetid );
    if ( dtype < 0 )
    {
	H5Sclose( memspace );
	H5Sclose( filespace );
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return;
    }

    if ( H5Dread(dsetid, dtype, memspace,
		 filespace, H5P_DEFAULT, data) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
    }

    H5Tclose( dtype );
    H5Sclose( memspace );
    H5Sclose( filespace );
}


void HDF5::ReaderImpl::gtAll( const DatasetID& h5ds, void* data,
			      uiRetVal& uirv ) const
{
    Threads::Locker locker( lock_ );
    if ( !h5ds.isValid() || !data )
    {
	uirv.add( uiStrings::phrCannotOpenForRead(fileName()) );
	return;
    }

    const ::hid_t dsetid = h5ds.asInt();
    if ( dsetid < 0 || H5Iis_valid(dsetid) <= 0 )
    {
	uirv.add( tr("Invalid dataset ID") );
	return;
    }

    const ::hid_t filespace = H5Dget_space( dsetid );
    const ::hid_t dtype = H5Dget_type( dsetid );
    if ( filespace < 0 || dtype < 0 )
    {
	if ( dtype >= 0 ) H5Tclose( dtype );
	if ( filespace >= 0 ) H5Sclose( filespace );
	uirv.add( tr("Failed to get dataspace or type") );
	return;
    }

    H5Sselect_all( filespace );
    if ( H5Dread(dsetid, dtype, filespace,
	 filespace, H5P_DEFAULT, data) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return;
    }

    H5Tclose( dtype );
    H5Sclose( filespace );
}

void HDF5::ReaderImpl::gtStrings( const DatasetID& h5ds,
				  BufferStringSet& bss, uiRetVal& uirv ) const
{
    Threads::Locker locker( lock_ );
    if ( !h5ds.isValid() )
    {
	uirv.add( uiStrings::phrCannotOpenForRead(fileName()) );
	return;
    }

    const ::hid_t dsetid = h5ds.asInt();
    if ( !H5Iis_valid(dsetid) )
    {
	uirv.add( uiStrings::phrCannotOpenForRead(fileName()) );
	return;
    }

    const ::hid_t space = H5Dget_space( dsetid );
    const ::hid_t dtype = H5Dget_type( dsetid );
    if ( space < 0 || dtype < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	if ( dtype >= 0 ) H5Tclose( dtype );
	if ( space >= 0 ) H5Sclose( space );
	return;
    }

    if ( H5Tget_class(dtype) != H5T_STRING )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	H5Tclose( dtype );
	H5Sclose( space );
	return;
    }

    hsize_t dims[1];
    if ( H5Sget_simple_extent_dims( space, dims,
				    nullptr ) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	H5Tclose( dtype );
	H5Sclose( space );
	return;
    }

    const hsize_t nrstrs = dims[0];
    const bool isvariable = ( H5Tis_variable_str(dtype) > 0 );
    if ( isvariable )
    {
	const ::hid_t memtype = H5Tcopy( H5T_C_S1 );
	H5Tset_size( memtype, H5T_VARIABLE );

	char** strs = new char* [nrstrs];
	if ( H5Dread( dsetid, memtype,
		      H5S_ALL, H5S_ALL,
		      H5P_DEFAULT, strs ) < 0 )
	{
	    uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	    delete [] strs;
	    H5Tclose( memtype );
	    H5Tclose( dtype );
	    H5Sclose( space );
	    return;
	}

	for ( hsize_t i=0; i<nrstrs; i++ )
	{
	    BufferString str( strs[i] );
	    str.trimBlanks();
	    bss.add( str );
	    H5free_memory( strs[i] );
	}

	delete [] strs;
	H5Tclose( memtype );
    }

    else
    {
	const size_t strsize = H5Tget_size( dtype );
	mAllocLargeVarLenArr( char, buf, (nrstrs * strsize) + 1 );
	if ( H5Dread( dsetid, dtype, H5S_ALL,
		      H5S_ALL, H5P_DEFAULT,
		      buf.ptr() ) < 0 )
	{
	    uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	    H5Tclose( dtype );
	    H5Sclose( space );
	    return;
	}

	mAllocLargeVarLenArr( char, single_str_buf, strsize + 1 );
	for ( hsize_t i=0; i<nrstrs; i++ )
	{
	    const char* src_ptr = buf.ptr() + (i * strsize);
	    OD::sysMemCopy( single_str_buf.ptr(), src_ptr, strsize );
	    single_str_buf.ptr()[strsize] = '\0';
	    BufferString str( single_str_buf.ptr() );
	    str.trimBlanks();
	    bss.add( str );
	}
    }

    H5Tclose( dtype );
    H5Sclose( space );
}

void HDF5::ReaderImpl::gtValues( const DatasetID& h5ds,
				 const NDPosBufSet& posbufs, void* data,
				 uiRetVal& uirv ) const
{
    Threads::Locker locker( lock_ );
    if ( !h5ds.isValid() || !data || posbufs.isEmpty() || nrdims_ < 1 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return;
    }

    const ::hid_t dsetid = h5ds.asInt();
    const ::hid_t filespace = H5Dget_space( dsetid );

     if ( filespace < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return;
    }

    const hsize_t nrpts = (hsize_t)posbufs.size();
    mAllocVarLenArr( hsize_t, hdfcoordarr, nrdims_ * nrpts );
    if ( !mIsVarLenArrOK(hdfcoordarr) )
    {
	uirv.add( uiStrings::phrCannotAllocateMemory() );
	H5Sclose( filespace );
	return;
    }

    for ( NDPosBufSet::idx_type ipt=0; ipt<nrpts; ipt++ )
    {
	const NDPosBuf& posbuf = posbufs[ipt];
	const int arroffs = ipt * nrdims_;
	for ( dim_idx_type idim=0; idim<nrdims_; idim++ )
		hdfcoordarr[arroffs + idim] = posbuf[idim];
    }

    if ( H5Sselect_elements( filespace, H5S_SELECT_SET,
	 nrpts, mVarLenArr(hdfcoordarr) ) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	H5Sclose( filespace );
	return;
    }

    const ::hid_t memspace = H5Screate_simple( 1, &nrpts,
					       nullptr );
    const ::hid_t dtype = H5Dget_type( dsetid );
    if ( H5Dread(dsetid, dtype, memspace,
	 filespace, H5P_DEFAULT, data) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	H5Tclose( dtype );
	H5Sclose( memspace );
	H5Sclose( filespace );
    }

}


bool HDF5::ReaderImpl::hasAttribute( const char* attrnm,
				     const DataSetKey* dsky ) const
{
    Threads::Locker locker( lock_ );
    if ( !attrnm || !*attrnm )
	return false;

    const ObjectID h5scope = getScope( dsky );
    if ( !h5scope.isValid() )
	return false;

    const ::hid_t scope = h5scope.asInt();
    if ( scope < 0 || mIsUdf(scope) || H5Iis_valid(scope) <= 0 )
	return false;

    return H5Aexists( scope, attrnm ) > 0;
}

int HDF5::ReaderImpl::getNrAttributes( const DataSetKey* dsky ) const
{
    Threads::Locker locker( lock_ );
    const ObjectID h5scope = getScope( dsky );
    if ( !h5scope.isValid() )
	return 0;

    H5O_info2_t info;
    if ( H5Oget_info3(h5scope.asInt(), &info,
	 H5O_INFO_NUM_ATTRS) < 0 )
	return 0;

    return (int)info.num_attrs;
}

static herr_t addAttrName( hid_t loc_id, const char* name,
			   const H5A_info_t*, void* opdata )
{
    BufferStringSet* nms = reinterpret_cast<BufferStringSet*>( opdata );
    nms->add( name );
    return 0;
}

void HDF5::ReaderImpl::gtAttribNames( const ObjectID& h5obj,
				      BufferStringSet& nms ) const
{
    Threads::Locker locker( lock_ );
    if ( !h5obj.isValid() )
	return;


    hid_t hid = h5obj.asInt();
    if (!H5Iis_valid(hid))
	return;

    hsize_t idx = 0;
    herr_t ret = H5Aiterate2(hid, H5_INDEX_NAME,
			     H5_ITER_INC, &idx, addAttrName,
			    &nms );
    ( void )ret;
}




bool HDF5::ReaderImpl::getAttribute( const char* attrnm,
				     BufferString& res,
				     const DataSetKey* dsky ) const
{
    Threads::Locker locker( lock_ );
    if ( !attrnm || !*attrnm )
	return false;

    const ObjectID scopeid = getScope( dsky );
    if ( !scopeid.isValid() )
	return false;

    const ::hid_t scope = scopeid.asInt();

    if ( scope < 0 || mIsUdf(scope) || H5Iis_valid(scope) <= 0 )
	return false;

    if (  H5Aexists(scope, attrnm) <= 0 )
	return false;

    const ::hid_t attr = H5Aopen( scope, attrnm,
				  H5P_DEFAULT );
    if ( attr < 0 )
	return false;

    const ::hid_t atype = H5Aget_type( attr );
    const ::hid_t ntype = H5Tget_native_type( atype,
					      H5T_DIR_ASCEND );
    if ( H5Tget_class(ntype) != H5T_STRING )
    {
	H5Tclose( ntype );
	H5Tclose( atype );
	H5Aclose( attr );
	return false;
    }

    if ( H5Tis_variable_str(ntype) )
    {
	const ::hid_t memtype = H5Tcopy( H5T_C_S1 );
	if ( memtype < 0 ||
	     H5Tset_size(memtype, H5T_VARIABLE) < 0 )
	{
	    if ( memtype >= 0 )
		H5Tclose( memtype );
	    H5Tclose( ntype );
	    H5Tclose( atype );
	    H5Aclose( attr );
	    return false;
	}

	char* buf = nullptr;
	if ( H5Aread(attr, memtype, &buf) < 0 )
	{
	    H5Tclose( memtype );
	    H5Tclose( ntype );
	    H5Tclose( atype );
	    H5Aclose( attr );
	    return false;
	}

	res.set( buf ? buf : "" );
	H5free_memory( buf );
	H5Tclose( memtype );
    }
    else
    {
	const size_t sz = H5Tget_size( ntype );
	BufferString buf;
	buf.setBufSize( (int)sz + 1 );
	if ( H5Aread(attr, ntype, buf.getCStr()) < 0 )
	{
	    H5Tclose( ntype );
	    H5Tclose( atype );
	    H5Aclose( attr );
	    return false;
	}

	buf.getCStr()[sz] = '\0';
	res.set( buf.str() );
    }

    H5Tclose( ntype );
    H5Tclose( atype );
    H5Aclose( attr );
    return true;
}


namespace {

static bool readNativeIntAttr( ::hid_t scope, const char* attrnm,
			       od_int64& res )
{
    if ( !attrnm || !*attrnm )
	return false;

    if ( scope < 0 || mIsUdf(scope) || H5Iis_valid(scope) <= 0 )
	return false;

    if ( H5Aexists(scope, attrnm) <= 0 )
	return false;

    const ::hid_t attr = H5Aopen( scope, attrnm,
				  H5P_DEFAULT );
    if ( attr < 0 )
	return false;

    const ::hid_t atype = H5Aget_type( attr );
    const ::hid_t ntype = H5Tget_native_type( atype,
					      H5T_DIR_ASCEND );
    if ( H5Tget_class(ntype) != H5T_INTEGER )
    {
	H5Tclose( ntype );
	H5Tclose( atype );
	H5Aclose( attr );
	return false;
    }

    od_int64 ival = 0;
    if ( H5Aread(attr, ntype, &ival) < 0 )
    {
	H5Tclose( ntype );
	H5Tclose( atype );
	H5Aclose( attr );
	return false;
    }

    res = ival;
    H5Tclose( ntype );
    H5Tclose( atype );
    H5Aclose( attr );
    return true;
}

static bool readAttrValueAsString( ::hid_t scope, const char* attrnm,
				   BufferString& res )
{
    if ( scope < 0 || mIsUdf(scope) || !attrnm || !*attrnm )
	return false;

    if ( H5Iis_valid(scope) <= 0 || H5Aexists(scope,
						  attrnm) <= 0 )
	return false;

    const ::hid_t attr = H5Aopen( scope, attrnm,
				  H5P_DEFAULT );
    if ( attr < 0 )
	return false;

    const ::hid_t atype = H5Aget_type( attr );
    if ( atype < 0 )
    {
	H5Aclose( attr );
	return false;
    }

    const ::hid_t space = H5Aget_space( attr );
    if ( space < 0 || H5Sget_simple_extent_type(space) != H5S_SCALAR )
    {
	if ( space >= 0 )
	    H5Sclose( space );
	H5Tclose( atype );
	H5Aclose( attr );
	return false;
    }

    const ::hid_t ntype = H5Tget_native_type( atype,
					      H5T_DIR_ASCEND );
    if ( ntype < 0 )
    {
	H5Sclose( space );
	H5Tclose( atype );
	H5Aclose( attr );
	return false;
    }

    const H5T_class_t cls = H5Tget_class( ntype );
    bool ok = false;
    if ( cls == H5T_STRING )
    {
	if ( H5Tis_variable_str(ntype) )
	{
	    const ::hid_t memtype = H5Tcopy( H5T_C_S1 );
	    if ( memtype >= 0 &&
		 H5Tset_size(memtype, H5T_VARIABLE) >= 0 )
	    {
		char* buf = nullptr;
		if ( H5Aread(attr, memtype, &buf) >= 0 )
		{
		    res.set( buf ? buf : "" );
		    H5free_memory( buf );
		    ok = true;
		}
	    }

	    if ( memtype >= 0 )
		H5Tclose( memtype );
	}
	else
	{
	    const size_t sz = H5Tget_size( ntype );
	    BufferString buf;
	    buf.setBufSize( (int)sz + 1 );
	    if ( H5Aread(attr, ntype,
			 buf.getCStr()) >= 0 )
	    {
		buf.getCStr()[sz] = '\0';
		res.set( buf.str() );
		ok = true;
	    }
	}
    }
    else if ( cls == H5T_INTEGER )
    {
	od_int64 ival = 0;
	if ( H5Aread(attr, ntype, &ival) >= 0 )
	{
	    res.set( ival );
	    ok = true;
	}
    }
    else if ( cls == H5T_FLOAT )
    {
	const size_t typesz = H5Tget_size( ntype );
	if ( typesz <= sizeof(float) )
	{
	    float fval = 0;
	    if ( H5Aread(attr, ntype, &fval) >= 0 )
	    {
		res.set( fval );
		ok = true;
	    }
	}
	else
	{
	    double dval = 0;
	    if ( H5Aread(attr, ntype, &dval) >= 0 )
	    {
		res.set( dval );
		ok = true;
	    }
	}
    }

    H5Tclose( ntype );
    H5Sclose( space );
    H5Tclose( atype );
    H5Aclose( attr );
    return ok;
}


static herr_t addAttrToIOPar( hid_t loc_id, const char* name,
			      const H5A_info_t* ainfo, void* opdata )
{
    IOPar* iop = static_cast<IOPar*>( opdata );
    BufferString res;
    if ( readAttrValueAsString( loc_id, name, res ) )
    {
	if ( res.isEmpty() || !res.buf() )
	{
	    iop->set( name, "" );
	}
	else
	{
	    iop->set( name, res.buf() );
	}
    }

    return 0;
}

static bool readNativeFloatAttr( ::hid_t scope, const char* attrnm,
				 double& res )
{
    if ( !attrnm || !*attrnm )
	return false;

    if ( scope < 0 || mIsUdf(scope) || H5Iis_valid(scope) <= 0 )
	return false;

    if ( H5Aexists(scope, attrnm) <= 0 )
	return false;

    const ::hid_t attr = H5Aopen( scope, attrnm,
				  H5P_DEFAULT );
    if ( attr < 0 )
	return false;

    const ::hid_t atype = H5Aget_type( attr );
    const ::hid_t ntype = H5Tget_native_type( atype,
					      H5T_DIR_ASCEND );
    if ( H5Tget_class(ntype) != H5T_FLOAT )
    {
	H5Tclose( ntype );
	H5Tclose( atype );
	H5Aclose( attr );
	return false;
    }

    double dval = 0;
    const size_t typesz = H5Tget_size( ntype );
    if ( typesz <= sizeof(float) )
    {
	float fval = 0;
	if ( H5Aread(attr, ntype, &fval) < 0 )
	{
	    H5Tclose( ntype );
	    H5Tclose( atype );
	    H5Aclose( attr );
	    return false;
	}
	dval = fval;
    }
    else if ( H5Aread(attr, ntype, &dval) < 0 )
    {
	H5Tclose( ntype );
	H5Tclose( atype );
	H5Aclose( attr );
	return false;
    }

    res = dval;
    H5Tclose( ntype );
    H5Tclose( atype );
    H5Aclose( attr );
    return true;
}



} // namespace


#define mGetIntAttr(type) \
bool HDF5::ReaderImpl::getAttribute( const char* attrnm, type& res, \
				     const DataSetKey* dsky ) const \
{ \
    Threads::Locker locker( lock_ ); \
    const ObjectID scopeid = getScope( dsky ); \
    if ( !scopeid.isValid() ) \
    { \
	return false; \
    } \
    od_int64 ival = 0; \
    if ( !readNativeIntAttr(scopeid.asInt(), attrnm, ival) ) \
    { \
	return false; \
    } \
    res = mCast(type, ival); \
    return true; \
}
mGetIntAttr(od_int16)
mGetIntAttr(od_uint16)
mGetIntAttr(od_int32)
mGetIntAttr(od_uint32)
mGetIntAttr(od_int64)
mGetIntAttr(od_uint64)
#undef mGetIntAttr


bool HDF5::ReaderImpl::getAttribute( const char* attrnm, float& res,
				     const DataSetKey* dsky ) const
{
    Threads::Locker locker( lock_ );
    const ObjectID scopeid = getScope( dsky );
    if ( !scopeid.isValid() )
	return false;

    double dval = 0;
    if ( !readNativeFloatAttr(scopeid.asInt(), attrnm, dval) )
	return false;

    res = mCast(float, dval);
    return true;
}


bool HDF5::ReaderImpl::getAttribute( const char* attrnm, double& res,
				     const DataSetKey* dsky ) const
{
    Threads::Locker locker( lock_ );
    const ObjectID scopeid = getScope( dsky );
    if ( !scopeid.isValid() )
	return false;

    return readNativeFloatAttr( scopeid.asInt(), attrnm, res );
}


void HDF5::ReaderImpl::gtInfo( const ObjectID& h5obj, IOPar& iop,
	uiRetVal& uirv ) const
{
    Threads::Locker locker( lock_ );
    if ( !h5obj.isValid() )
	return;

    const ::hid_t objid = h5obj.asInt();
    if ( H5Iis_valid(objid) <= 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
	return;
    }

    iop.setEmpty();
    hsize_t idx = 0;
    if ( H5Aiterate2(objid, H5_INDEX_NAME,
		     H5_ITER_INC, &idx, addAttrToIOPar,
		     &iop) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringRead(fileName()) );
    }
}


uiRetVal HDF5::ReaderImpl::readJSonAttribute( const char* attrnm,
					      OD::JSON::ValueSet& vs,
					      const DataSetKey* dsky ) const
{
    uiRetVal uirv;
    if ( !attrnm || !*attrnm )
    {
	uirv.set( tr("Valid attribute name required") );
	return uirv;
    }

    if ( !hasAttribute(attrnm,dsky) )
    {
	uirv.set( tr("No attribute named: %1").arg(attrnm) );
	return uirv;
    }

    BufferString valstr;
    if ( !getAttribute(attrnm,valstr,dsky) )
    {
	uirv.set( tr("Cannot read attribute: %1").arg(attrnm) );
	return uirv;
    }

    return vs.parseJSon( valstr.getCStr(), valstr.size() );
}
