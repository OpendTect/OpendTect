/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5writerimpl.h"

#include "envvars.h"
#include "hdf5readerimpl.h"
#include "iopar.h"
#include "odjson.h"
#include "od_ostream.h"
#include "uistrings.h"


/*static unsigned gzip_pixels_per_block = 16;
		    // can be an even number [2,32]*/
static int gzip_encoding_status = -1;

#define mCatchErrDuringWrite() \
    mCatchAdd2uiRv( uiStrings::phrErrDuringWrite(fileName()) )


HDF5::WriterImpl::WriterImpl()
    : AccessImpl(*this)
{
    Threads::Locker locker( lock_ );
    fileid_.setUdf();
    myfile_ = false;
    if ( gzip_encoding_status < 0 )
    {
       gzip_encoding_status = H5Zfilter_avail( H5Z_FILTER_DEFLATE ) ? 1 : 0;
	if ( gzip_encoding_status == 1 )
	{
	    unsigned int filter_info;
	    H5Zget_filter_info( H5Z_FILTER_DEFLATE,
				&filter_info );
	    if ( !(filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED) ||
		 !(filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED) )
		gzip_encoding_status = 0;
	}
    }
}


HDF5::WriterImpl::~WriterImpl()
{
    Threads::Locker locker( lock_ );
    closeFile();
}


HDF5::Reader* HDF5::WriterImpl::createCoupledReader() const
{
    if ( !fileid_.isValid() )
    {
	return nullptr;
    }

    return new HDF5::ReaderImpl( fileid_ );
}


void HDF5::WriterImpl::openFile( const char* fnm, uiRetVal& uirv, bool edit )
{
    Threads::Locker locker( lock_ );
    closeFile();
    myfile_ = true;

    ::hid_t fid = edit
    ? H5Fopen( fnm, H5F_ACC_RDWR, H5P_DEFAULT )
    : H5Fcreate( fnm, H5F_ACC_TRUNC, H5P_DEFAULT,
		     H5P_DEFAULT );

    if ( fid < 0 )
    {
	uirv.add( tr("Cannot open or create file") );
	return;
    }

    const ::hid_t gid = H5Gopen2( fid, "/", H5P_DEFAULT );
    if ( gid < 0 )
    {
	H5Fclose( fid );
	uirv.add( tr("Cannot open root group") );
	return;
    }

    fileid_ = FileID::get( mCast(hid_t,fid) );
    group_ = GroupID::get( mCast(hid_t,gid) );
    dataset_ = DatasetID();
}


HDF5::GroupID HDF5::WriterImpl::ensureGroup( const char* grpnm,
					     uiRetVal& uirv )
{
    if ( !fileid_.isValid() )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return GroupID::get( H5I_INVALID_HID );
    }

    if ( !grpnm || !*grpnm || StringView(grpnm)=="/" )
    {
	hid_t root = H5Gopen2( fileid_.asInt(), "/", H5P_DEFAULT );
	if ( root < 0 )
	{
	    uirv.add( tr("Cannot open root group") );
	    return GroupID::get( H5I_INVALID_HID );
	}

	if ( group_.isValid() && group_.asInt() > 0 &&
	     group_.asInt() != fileid_.asInt() )
	    H5Gclose( group_.asInt() );

	group_ = GroupID::get( mCast(hid_t,root) );
	return group_;
    }

    hid_t grpid = H5Gopen2( fileid_.asInt(), grpnm, H5P_DEFAULT );
    if ( grpid >= 0 )
    {
	if ( group_.isValid() && group_.asInt() > 0 &&
	     group_.asInt() != fileid_.asInt() &&
	     group_.asInt() != grpid )
	{
	    H5Gclose( group_.asInt() );
	}

	group_ = GroupID::get( mCast(hid_t,grpid) );
	return group_;
    }

    BufferString path;
    if ( grpnm[0] == '/' )
	path = grpnm;

    else
    {
	path = "/";
	path += grpnm;
    }

    hid_t lcpl = H5Pcreate( H5P_LINK_CREATE );
    H5Pset_create_intermediate_group( lcpl, 1 );
    hid_t newgrp = H5Gcreate2( fileid_.asInt(), path.buf(),
			       lcpl, H5P_DEFAULT, H5P_DEFAULT );
    H5Pclose( lcpl );
    if ( newgrp < 0 )
    {
	uirv.add( tr("Cannot create Group '%1'").arg(grpnm) );
	return GroupID::get( H5I_INVALID_HID );
    }

    if ( group_.isValid() && group_.asInt() > 0 &&
	 group_.asInt() != fileid_.asInt() )
    {
	H5Gclose( group_.asInt() );
    }

    group_ = GroupID::get( mCast(hid_t,newgrp) );
    return group_;
}

HDF5::DatasetID HDF5::WriterImpl::crDS( const DataSetKey& dsky,
					const ArrayNDInfo& info,
					ODDataType dt,
					uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    const GroupID grp = ensureGroup( dsky.groupName(), uirv );
    const ::hid_t grpid = grp.asInt();
    if ( !grp.isValid() || grpid < 0 || H5Iis_valid(grpid) <= 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return DatasetID::get( H5I_INVALID_HID );
    }

    nrdims_ = info.nrDims();
    if ( nrdims_ <= 0 )
	return DatasetID::get( H5I_INVALID_HID );

    TypeSet<hsize_t> dims, maxszs, chunkdims;
    dims.setSize( nrdims_ );
    maxszs.setSize( nrdims_ );
    chunkdims.setSize( nrdims_ );
    for ( int idim=0; idim<nrdims_; ++idim )
    {
	const int sz = info.getSize( idim );
	dims[idim] = sz > 0 ? sz : 1;
	const int maxchunkzs = dsky.maxDimSz( idim );
	if ( !mIsUdf(maxchunkzs) )
	{
	    chunkdims[idim] = maxchunkzs;
	    maxszs[idim] = H5S_UNLIMITED;
	}
	else
	{
	    chunkdims[idim] = sz > 0 ? sz : 1;
	    maxszs[idim] = H5S_UNLIMITED;
	}
    }

    const DatatypeID h5dt = h5DataTypeFor( dt );
    if ( !h5dt.isValid() || h5dt.asInt() < 0 )
	return DatasetID::get( H5I_INVALID_HID );

    const ::hid_t space = H5Screate_simple( nrdims_, dims.arr(),
					    maxszs.arr() );
    if ( space < 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return DatasetID::get( H5I_INVALID_HID );
    }

    const ::hid_t dcpl = H5Pcreate( H5P_DATASET_CREATE );
    if ( dcpl < 0 )
    {
	H5Sclose( space );
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return DatasetID::get( H5I_INVALID_HID );
    }

    H5Pset_chunk( dcpl, nrdims_, chunkdims.arr() );
    if ( compressionlvl_ > 0 )
    {
	H5Pset_deflate( dcpl, compressionlvl_ );
	H5Pset_shuffle( dcpl );
    }

    if ( H5Lexists(grpid, dsky.dataSetName(),
		   H5P_DEFAULT) > 0 )
    {
	H5Ldelete( grpid, dsky.dataSetName(),
		   H5P_DEFAULT );
    }

    const ::hid_t dsid = H5Dcreate2( grpid,
				     dsky.dataSetName(),
				     h5dt.asInt(), space,
				     H5P_DEFAULT, dcpl,
				     H5P_DEFAULT );
    H5Pclose( dcpl );
    H5Sclose( space );
    if ( dsid < 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return DatasetID::get( H5I_INVALID_HID );
    }

    const ::hid_t oldds = dataset_.asInt();
    if ( oldds > 0 && H5Iis_valid(oldds) > 0 )
	H5Dclose( oldds );

    dataset_ = DatasetID::get( mCast(hid_t,dsid) );
    return dataset_;
}


HDF5::DatasetID HDF5::WriterImpl::crTxtDS( const DataSetKey& dsky,
					   uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    const GroupID grp = ensureGroup( dsky.groupName(), uirv );
    if ( !grp.isValid() )
	return DatasetID::get( H5I_INVALID_HID );

    hsize_t dims[1] = { 1 };
    hsize_t maxdims[1] = { H5S_UNLIMITED };
    hsize_t chunks[1] = { 1 };
    const ::hid_t strtype = H5Tcopy( H5T_C_S1 );
    H5Tset_size( strtype, H5T_VARIABLE );
    const ::hid_t space = H5Screate_simple( 1, dims, maxdims );
    if ( space < 0 )
    {
	H5Tclose( strtype );
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return DatasetID::get( H5I_INVALID_HID );
    }

    const ::hid_t dcpl = H5Pcreate( H5P_DATASET_CREATE );
    if ( dcpl < 0 )
    {
	H5Sclose( space );
	H5Tclose( strtype );
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return DatasetID::get( H5I_INVALID_HID );
    }

    H5Pset_chunk( dcpl, 1, chunks );
    const ::hid_t dsid = H5Dcreate2( grp.asInt(),
				     dsky.dataSetName(),
				     strtype, space,
				     H5P_DEFAULT, dcpl,
				     H5P_DEFAULT );
    H5Pclose( dcpl );
    H5Sclose( space );
    if ( dsid < 0 )
    {
	H5Tclose( strtype );
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return DatasetID::get( H5I_INVALID_HID );
    }

    const char* empty = "";
    if ( H5Dwrite(dsid, strtype, H5S_ALL,
	 H5S_ALL, H5P_DEFAULT, &empty) < 0 )
    {
	H5Dclose( dsid );
	H5Tclose( strtype );
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return DatasetID::get( H5I_INVALID_HID );
    }

    H5Tclose( strtype );
    const ::hid_t oldds = dataset_.asInt();
    int retval = 0;
    if ( (retval = H5Iis_valid(oldds)) > 0 )
	H5Dclose( oldds );

    dataset_ = DatasetID::get( mCast(hid_t,dsid) );
    return dataset_;
}


void HDF5::WriterImpl::reSzDS( const ArrayNDInfo& info,
			       const DatasetID& h5ds, uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    const ::hid_t dsid = h5ds.asInt();
    if ( dsid <= 0 || H5Iis_valid(dsid) <= 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    const int nrdims = info.nrDims();
    TypeSet<hsize_t> newdims( nrdims, 0 );
    for ( int idim=0; idim<nrdims; ++idim )
	newdims[idim] = info.getSize( idim );

    if ( H5Dset_extent(dsid, newdims.arr()) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
    }
}


void HDF5::WriterImpl::ptSlab( const SlabSpec& spec, const void* data,
			       const DatasetID& h5ds, uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );

    const ::hid_t dsetid = h5ds.asInt();
    int retval = 0;
    if ( (retval = H5Iis_valid(dsetid)) <= 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    const ::hid_t filespace = H5Dget_space( dsetid );
    if ( filespace < 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    H5Sselect_all( filespace );
    TypeSet<hsize_t> counts;
    selectSlab( DataspaceID::get(mCast(hid_t,filespace)), spec, &counts );
    const ::hid_t memspace = H5Screate_simple( counts.size(),
					       counts.arr(),
					       nullptr );
    const ::hid_t dtype = H5Dget_type( dsetid );
    if ( memspace < 0 || dtype < 0 ||
	H5Dwrite(dsetid, dtype, memspace,
	     filespace, H5P_DEFAULT, data) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
    }

    if ( dtype >= 0 )
	H5Tclose( dtype );

    if ( memspace >= 0 )
	H5Sclose( memspace );

    H5Sclose( filespace );
}


void HDF5::WriterImpl::ptAll( const void* data, const DatasetID& h5ds,
			      uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    if ( !data )
    {
    uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
    return;
    }

    const ::hid_t dsetid = h5ds.asInt();
    if ( dsetid <= 0 || H5Iis_valid(dsetid) <= 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    const ::hid_t filespace = H5Dget_space( dsetid );
    const ::hid_t dtype = H5Dget_type( dsetid );
    if ( filespace < 0 || dtype < 0 )
    {
	if ( dtype >= 0 ) H5Tclose( dtype );

	if ( filespace >= 0 ) H5Sclose( filespace );
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    H5Sselect_all( filespace );
    if ( H5Dwrite(dsetid, dtype, filespace,
	 filespace, H5P_DEFAULT, data) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    H5Tclose( dtype );
    H5Sclose( filespace );
}

void HDF5::WriterImpl::ptStrings( const BufferStringSet& bss,
				  const GroupID& grpobj,
				  const DatasetID& h5obj,
				  const char* dsnm, uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    if ( !dsnm || !*dsnm || !grpobj.isValid() )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    DatasetID ds = h5obj;
    if ( ds.asInt() <= 0 || H5Iis_valid(ds.asInt()) <= 0 )
    {
	const int sz = mMAX( (int)bss.size(), 1 );
	const Array1DInfoImpl inf( sz );
	const HDF5::DataSetKey dsky( "", dsnm );
	hsize_t dims[1] = { (hsize_t)inf.getSize(0) };
	hsize_t maxdims[1] = { H5S_UNLIMITED };
	hsize_t chunks[1] = { 1 };

	::hid_t strtype = H5Tcopy( H5T_C_S1 );
	H5Tset_size( strtype, H5T_VARIABLE );

	::hid_t space = H5Screate_simple( 1, dims, maxdims );
	::hid_t dcpl = H5Pcreate( H5P_DATASET_CREATE );
	H5Pset_chunk( dcpl, 1, chunks );

	::hid_t dsid = H5Dcreate2( grpobj.asInt(), dsnm,
				   strtype, space,
				   H5P_DEFAULT, dcpl,
				   H5P_DEFAULT );
	H5Pclose( dcpl );
	H5Sclose( space );
	H5Tclose( strtype );

	if ( dsid < 0 )
	{
	    uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	    return;
	}

	ds = DatasetID::get( mCast(hid_t,dsid) );
	dataset_ = ds;
    }

    else if ( !bss.isEmpty() )
    {
	reSzDS( Array1DInfoImpl(bss.size()), ds, uirv );
	if ( !uirv.isOK() )
	    return;
    }

    const hsize_t nrstrs = bss.size();
    char** strs = new char* [nrstrs];
    for ( hsize_t i=0; i<nrstrs; i++ )
    {

	strs[i] = ( char* )bss.get( (int)i ).buf();
    }

    ::hid_t dtype = H5Dget_type( ds.asInt() );
    ::hid_t space = H5Dget_space( ds.asInt() );
    H5Sselect_all( space );
    if ( H5Dwrite(ds.asInt(), dtype, space,
		  space, H5P_DEFAULT, strs) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
    }

    delete [] strs;
    H5Tclose( dtype );
    H5Sclose( space );
}

bool HDF5::WriterImpl::rmObj( const DataSetKey& dsky )
{
    Threads::Locker locker( lock_ );
    if ( !fileid_.isValid() )
	return false;

    if ( dataset_.isValid() )
    {
	const hid_t h5ds = dataset_.asInt();
	dataset_.setUdf();
	if ( h5ds > 0 && H5Iis_valid(h5ds) > 0 )
	    H5Dclose( h5ds );
    }

    if ( group_.isValid() )
    {
	const hid_t h5grp = group_.asInt();
	group_.setUdf();
	if ( h5grp > 0 && H5Iis_valid(h5grp) > 0 && h5grp != fileid_.asInt() )
	    H5Gclose( h5grp );
    }

    uiRetVal uirv;
    const GroupID grp = ensureGroup( dsky.groupName(), uirv );
    const hid_t grpid = grp.asInt();

    if ( !grp.isValid() || grpid < 0 || H5Iis_valid(grpid) <= 0 )
	return false;

    const char* dsname = dsky.dataSetName();
    if ( !dsname || !*dsname )
	return false;

    if ( H5Lexists(grpid, dsname, H5P_DEFAULT) <= 0 )
	return false;

    if ( H5Ldelete(grpid, dsname, H5P_DEFAULT) < 0 )
	return false;

    if ( dataset_.isValid() )
    {
	const hid_t h5ds = dataset_.asInt();
	dataset_.setUdf();
	if ( h5ds > 0 && H5Iis_valid(h5ds) > 0 )
	    H5Dclose( h5ds );
    }

    if ( group_.isValid() )
    {
	const hid_t h5grp = group_.asInt();
	group_.setUdf();
	if ( h5grp > 0 && H5Iis_valid(h5grp) > 0 && h5grp != fileid_.asInt() )
	    H5Gclose( h5grp );
    }

    H5Fflush( fileid_.asInt(), H5F_SCOPE_GLOBAL );
    nrdims_ = -1;
    return true;
}

void HDF5::WriterImpl::renObj( const LocationID&,
			       const char* from, const char* to,
			       uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    if ( !fileid_.isValid() )
    {
	uirv.add( uiStrings::phrErrDuringWrite( fileName() ) );
	return;
    }

    if ( !from || !*from || !to || !*to )
    {
	uirv.add( uiStrings::phrErrDuringWrite( fileName() ) );
	return;
    }

    const hid_t fileid = fileid_.asInt();
    if ( H5Lmove( fileid, from, fileid, to,
		  H5P_DEFAULT, H5P_DEFAULT ) < 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite( fileName() ) );
	return;
    }

    uirv.setOK();
}





void HDF5::WriterImpl::rmAttrib( const char* nm, const ObjectID& h5obj )
{
    Threads::Locker locker( lock_ );
    if ( !nm || !*nm )
	return;

    const ::hid_t oid = h5obj.asInt();
    if ( oid <= 0 || H5Iis_valid(oid) <= 0 )
	return;

    if ( H5Aexists(oid, nm) <= 0 )
	return;

    H5Adelete( oid, nm );
}


namespace {
static herr_t collectAttrName( ::hid_t, const char* name,
			       const H5A_info_t*, void* opdata )
{
    ( (BufferStringSet*)opdata )->add( name );
    return 0;
}
}

void HDF5::WriterImpl::rmAllAttribs( const ObjectID& h5obj )
{
    Threads::Locker locker( lock_ );
    const ::hid_t oid = h5obj.asInt();
    if ( oid <= 0 || H5Iis_valid(oid) <= 0 )
	return;

    BufferStringSet nms;
    H5Aiterate2( oid, H5_INDEX_NAME, H5_ITER_INC,
		 nullptr,
		 collectAttrName, &nms );

    for ( const auto* nm : nms )
    H5Adelete( oid, nm->buf() );
}


void HDF5::WriterImpl::setAttribute( const char* ky, const char* val,
				     const DataSetKey* dsky )
{
    Threads::Locker locker( lock_ );
    const ObjectID scope = AccessImpl::stScope( dsky );
    if ( !scope.isValid() )
	return;

    setAttribute( ky, val, scope );
}


void HDF5::WriterImpl::stComment( const LocationID& h5loc, const char* name,
				  const char* comment, uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    if ( !h5loc.isValid() || !name || !*name )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    BufferString nm;
    if ( *name == '/' )
	nm.set( name );

    else
    {
	nm.set( "/" );
	nm.add( name );
    }

    const ::hid_t loc = h5loc.asInt();
    if ( !comment || !*comment )
    {
	if ( H5Oset_comment_by_name(loc, nm.buf(), NULL,
				    H5P_DEFAULT) < 0 )
	    uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
    }

    else if ( H5Oset_comment_by_name(loc, nm.buf(), comment,
				     H5P_DEFAULT) < 0 )
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
}

void HDF5::WriterImpl::setAttribute( const char* ky, const char* val,
				     const ObjectID& h5scope )
{
    if ( !ky || !*ky )
	return;

    const ::hid_t scope = h5scope.asInt();
    if ( scope <= 0 || H5Iis_valid(scope) <= 0 )
	return;

    const ::hid_t strtype = H5Tcopy( H5T_C_S1 );
    H5Tset_size( strtype, H5T_VARIABLE );
    const ::hid_t space = H5Screate( H5S_SCALAR );
    if ( H5Aexists(scope, ky) > 0 )
	H5Adelete( scope, ky );

    const ::hid_t attr = H5Acreate2( scope, ky,
				     strtype, space,
				     H5P_DEFAULT,
				     H5P_DEFAULT );
    if ( attr >= 0 )
    {
	const char* s = (val && *val) ? val : "";
	H5Awrite( attr, strtype, &s );
	H5Aclose( attr );
    }

    H5Sclose( space );
    H5Tclose( strtype );
}


#define mSetNumAttr(type) \
void HDF5::WriterImpl::setAttribute( const char* attrnm, type val, \
				     const DataSetKey* dsky ) \
{ \
    setAttribute( attrnm, toString(val), dsky ); \
}

mSetNumAttr(od_int16)
mSetNumAttr(od_uint16)
mSetNumAttr(od_int32)
mSetNumAttr(od_uint32)
mSetNumAttr(od_int64)
mSetNumAttr(od_uint64)
mSetNumAttr(float)
mSetNumAttr(double)
#undef mSetNumAttr


void HDF5::WriterImpl::ptInfo( const IOPar& iop, const ObjectID& h5obj,
			       uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    IOParIterator iter( iop );
    BufferString key, val;
    while ( iter.next(key,val) )
	setAttribute( key.str(), val.str(), h5obj );
}


uiRetVal HDF5::WriterImpl::writeJSonAttribute( const char* attrnm,
					       const OD::JSON::ValueSet&
					       jsonobj, const DataSetKey* dsky )
{
    uiRetVal uirv;
    if ( !attrnm || !*attrnm )
    {
	uirv.set( tr("Valid attribute name required") );
	return uirv;
    }

    BufferString jsonstr;
    jsonobj.dumpJSon( jsonstr );
    setAttribute( attrnm, jsonstr.buf(), dsky );
    return uirv;
}
