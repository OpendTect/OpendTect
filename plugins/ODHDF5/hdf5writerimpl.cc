/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5writerimpl.h"

#include "datachar.h"
#include "debug.h"
#include "envvars.h"
#include "hdf5common.h"
#include "hdf5readerimpl.h"
#include "iopar.h"
#include "math2.h"
#include "odjson.h"
#include "od_ostream.h"
#include "uistrings.h"


static unsigned gzip_pixels_per_block = 16;
		    // can be an even number [2,32]
static int gzip_encoding_status = -1;

#define mAddErrDuringWrite() \
    mAddHDFErr2uiRv( uiStrings::phrErrDuringWrite(fileName()) )

#define mCatchErrDuringWrite( id ) \
    mCatchHDF( id, mAddErrDuringWrite(); return )


HDF5::WriterImpl::WriterImpl()
    : AccessImpl(*this)
{
    Threads::Locker locker( lock_ );
    if ( gzip_encoding_status < 0 )
    {
	gzip_encoding_status = H5Zfilter_avail( H5Z_FILTER_DEFLATE ) ? 1 : 0;
	if ( gzip_encoding_status == 1 )
	{
	    unsigned int filter_info;
	    H5Zget_filter_info( H5Z_FILTER_DEFLATE, &filter_info );
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
    return fileid_.isValid() ? new HDF5::ReaderImpl( fileid_ ) : nullptr;
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
    mCatchHDFAdd2uiRv( fid, uiStrings::phrErrDuringWrite(fnm) );

    const ::hid_t gid = H5Gopen2( fid, "/", H5P_DEFAULT );
    mCatchHDF( gid,
	H5Fclose( fid );
	mAddHDFErr2uiRv( uiStrings::phrErrDuringWrite(fnm) );
	return );

    fileid_ = FileID::get( mCast(hid_t,fid) );
    group_ = GroupID::get( mCast(hid_t,gid) );
    dataset_ = DatasetID();
}


HDF5::GroupID HDF5::WriterImpl::ensureGroup( const char* grpnm,
					     uiRetVal& uirv )
{
    const GroupID existing = selectGroup( grpnm );
    if ( existing.isValid() )
	return existing;

    if ( !grpnm || !*grpnm || StringView(grpnm) == "/" )
    {
	uirv.add( tr("Cannot open root group") );
	return GroupID::udf();
    }

    BufferString path;
    if ( grpnm[0] == '/' )
	path = grpnm;
    else
    {
	path = "/";
	path += grpnm;
    }

    const hid_t lcpl = H5Pcreate( H5P_LINK_CREATE );
    mCatchHDF( lcpl,
	mAddErrDuringWrite();
	return GroupID::udf() );

    H5Pset_create_intermediate_group( lcpl, 1 );
    const hid_t newgrp = H5Gcreate2( fileid_.asInt(), path.buf(),
				     lcpl, H5P_DEFAULT,
				     H5P_DEFAULT );
    H5Pclose( lcpl );
    mCatchHDF( newgrp,
	mAddErrDuringWrite();
	return GroupID::udf() );

    group_ = GroupID::get( mCast(hid_t,newgrp) );
    previousgroupids_.add( newgrp );
    return group_;
}


HDF5::DatasetID HDF5::WriterImpl::crDS( const DataSetKey& dsky,
					const ArrayNDInfo& info,
					ODDataType dt,
					uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    const char* dsnm = dsky.dataSetName();
    if ( !dsnm || !*dsnm )
	return DatasetID::udf();

    const GroupID grp = ensureGroup( dsky.groupName(), uirv );
    if ( !grp.isValid() )
	return DatasetID::udf();

    nrdims_ = info.nrDims();
    if ( nrdims_ <= 0 )
	return DatasetID::udf();

    TypeSet<hsize_t> dims, chunkdims;
    hsize_t maxdim = 0;
    hsize_t maxchunkdim = 0;
    const bool editable = dsky.isEditable();
    bool mustchunk = editable;
    for ( ArrayNDInfo::dim_idx_type idim=0; idim<nrdims_; idim++ )
    {
	const auto dimsz = info.getSize( idim );
	if ( dimsz > maxdim )
	    maxdim = dimsz;
	dims += dimsz > 0 ? dimsz : 1;
    }

    TypeSet<hsize_t> maxszs;
    bool hasmaxsz = false;
    od_uint64 totsz = 1;
    for ( ArrayNDInfo::dim_idx_type idim=0; idim<nrdims_; idim++ )
    {
	const hsize_t dimsz = dims[idim];
	const int chunksz = dsky.chunkSz( idim );
	const int maxchunkzs = dsky.maxDimSz( idim );
	hsize_t chunkdim = dimsz < (hsize_t)chunksz ? dimsz
						    : (hsize_t)chunksz;
	const bool hasmaxresize = !mIsUdf(maxchunkzs);
	if ( mustchunk && hasmaxresize )
	{
	    chunkdim = maxchunkzs;
	    hasmaxsz = true;
	}
	if ( maxchunkdim < chunkdim )
	    maxchunkdim = chunkdim;
	chunkdims += chunkdim;
	maxszs += hasmaxresize ? H5S_UNLIMITED : dimsz;
	totsz *= chunkdim;
    }

    const DataCharacteristics dc( dt );
    totsz *= dc.nrBytes();
    static od_uint64 maxhdf5chunksz = mDef4GB;
    if ( totsz >= maxhdf5chunksz && !chunkdims.isEmpty() )
    {
	int maxdimidx = 0;
	for ( int idim=1; idim<nrdims_; idim++ )
	{
	    if ( chunkdims[idim] > chunkdims[maxdimidx] )
		maxdimidx = idim;
	}

	const double chunkratio = double(totsz) / (maxhdf5chunksz-1);
	const double new1dsz = double(chunkdims[maxdimidx]) / chunkratio;
	if ( new1dsz > 1. )
	{
	    maxchunkdim = (hsize_t)Math::Floor( new1dsz );
	    if ( maxchunkdim > 0 )
	    {
		maxszs[maxdimidx] = H5S_UNLIMITED;
		chunkdims[maxdimidx] = maxchunkdim;
		hasmaxsz = true;
		mustchunk = true;
		const_cast<DataSetKey&>( dsky ).setMaximumSize( maxdimidx,
								maxchunkdim );
	    }
	}
    }

    if ( mustchunk && !hasmaxsz )
    {
	for ( auto& maxsz : maxszs )
	    maxsz = H5S_UNLIMITED;
    }

    mDefineStaticLocalObject(bool, allowzip,
		= GetEnvVarYN("OD_HDF5_ALLOWZIP",false) );
    mDefineStaticLocalObject(bool, allowshuffle,
		= GetEnvVarYN("OD_HDF5_ALLOWSHUFFLE",true) );

    const bool wantchunk = maxdim > maxchunkdim;
    const bool canzip = allowzip && (mustchunk || wantchunk) &&
		     gzip_encoding_status>0
		     && maxdim >= gzip_pixels_per_block;

    const DatatypeID h5dt = h5DataTypeFor( dt );
    if ( !h5dt.isValid() )
	return DatasetID::udf();

    const ::hid_t space = H5Screate_simple( nrdims_, dims.arr(),
					    maxszs.arr() );
    mCatchHDF( space,
	mAddErrDuringWrite();
	return DatasetID::udf() );

    const ::hid_t dcpl = H5Pcreate( H5P_DATASET_CREATE );
    mCatchHDF( dcpl,
	H5Sclose( space );
	mAddErrDuringWrite();
	return DatasetID::udf() );

    if ( mustchunk || wantchunk )
	H5Pset_chunk( dcpl, nrdims_, chunkdims.arr() );

    if ( canzip )
    {
	H5Pset_deflate( dcpl, compressionlvl_ );
	if ( allowshuffle && compressionlvl_ > 0 )
	    H5Pset_shuffle( dcpl );
    }

#ifdef __debug__
    if ( DBG::isOn(DGB_HDF5) )
    {
	od_cout() << "Create DataSet: "
		  << dsky.fullDataSetName() << od_endl;
    }
#endif

    const ::hid_t grpid = grp.asInt();
    if ( H5Lexists(grpid ,dsnm, H5P_DEFAULT) > 0 )
    {
	if ( atDataSet(dsnm) )
	{
	    const ::hid_t oldid = dataset_.asInt();
	    dataset_.setUdf();
	    if ( oldid >= 0 && H5Iis_valid(oldid) > 0
	      && H5Iget_type(oldid) == H5I_DATASET )
		H5Dclose( oldid );
	}

	H5Ldelete( grpid, dsnm, H5P_DEFAULT );
    }

    const ::hid_t dsid = H5Dcreate2( grpid, dsnm,
				     h5dt.asInt(), space,
				     H5P_DEFAULT, dcpl,
				     H5P_DEFAULT );
    H5Pclose( dcpl );
    H5Sclose( space );
    mCatchHDF( dsid,
	mAddErrDuringWrite();
	return DatasetID::udf() );

    const ::hid_t oldid = dataset_.asInt();
    dataset_.setUdf();
    if ( oldid >= 0 && H5Iis_valid(oldid) > 0
	 && H5Iget_type(oldid) == H5I_DATASET )
	H5Dclose( oldid );

    dataset_ = DatasetID::get( mCast(hid_t,dsid) );
    return dataset_;
}


HDF5::DatasetID HDF5::WriterImpl::crTxtDS( const DataSetKey& dsky,
					   uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    const char* dsnm = dsky.dataSetName();
    if ( !dsnm || !*dsnm )
	return DatasetID::udf();

    const GroupID grp = ensureGroup( dsky.groupName(), uirv );
    if ( !grp.isValid() )
	return DatasetID::udf();

    nrdims_ = 1;
    TypeSet<hsize_t> dims( 1, (hsize_t)1 );
    TypeSet<hsize_t> maxdims( 1, (hsize_t)H5S_UNLIMITED );
    TypeSet<hsize_t> chunks( 1, (hsize_t)1 );

    const ::hid_t strtype = H5Tcopy( H5T_C_S1 );
    mCatchHDF( strtype,
	mAddErrDuringWrite();
	return DatasetID::udf() );

    H5Tset_size( strtype, H5T_VARIABLE );
    const ::hid_t space = H5Screate_simple( 1, dims.arr(),
					    maxdims.arr() );
    mCatchHDF( space,
	H5Tclose( strtype );
	mAddErrDuringWrite();
	return DatasetID::udf() );

    const ::hid_t dcpl = H5Pcreate( H5P_DATASET_CREATE );
    mCatchHDF( dcpl,
	H5Sclose( space );
	H5Tclose( strtype );
	mAddErrDuringWrite();
	return DatasetID::udf() );

    H5Pset_chunk( dcpl, 1, chunks.arr() );
    const ::hid_t grpid = grp.asInt();
    if ( H5Lexists(grpid,dsnm,H5P_DEFAULT) > 0 )
    {
	if ( atDataSet(dsnm) )
	{
	    const ::hid_t oldid = dataset_.asInt();
	    dataset_.setUdf();
	    if ( oldid >= 0 && H5Iis_valid(oldid) > 0
	      && H5Iget_type(oldid) == H5I_DATASET )
		H5Dclose( oldid );
	}

	H5Ldelete( grpid, dsnm, H5P_DEFAULT );
    }

    const ::hid_t dsid = H5Dcreate2( grpid, dsnm, strtype,
				     space, H5P_DEFAULT,
				     dcpl, H5P_DEFAULT );
    H5Pclose( dcpl );
    H5Sclose( space );
    mCatchHDF( dsid,
	H5Tclose( strtype );
	mAddErrDuringWrite();
	return DatasetID::udf() );

    const char* empty = "";
    if ( H5Dwrite(dsid, strtype, H5S_ALL,
		  H5S_ALL, H5P_DEFAULT,
		  &empty) < 0 )
    {
	H5Dclose( dsid );
	H5Tclose( strtype );
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return DatasetID::udf();
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
    if ( !h5ds.isValid() )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    const ::hid_t dsid = h5ds.asInt();
    if ( dsid <= 0 )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    const int nrdims = info.nrDims();
    if ( nrdims <= 0 )
	return;

    TypeSet<hsize_t> newdims( nrdims, (hsize_t)0 );
    for ( int idim=0; idim<nrdims; idim++ )
	newdims[idim] = info.getSize( idim );

    if ( H5Dset_extent(dsid, newdims.arr()) < 0 )
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
}


void HDF5::WriterImpl::ptSlab( const SlabSpec& spec, const void* data,
			       const DatasetID& h5ds, uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    if ( !h5ds.isValid() || !data )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    const ::hid_t filespace = H5Dget_space( h5ds.asInt() );
    mCatchErrDuringWrite( filespace );

    H5Sselect_all( filespace );
    TypeSet<hsize_t> counts;
    selectSlab( DataspaceID::get(mCast(hid_t,filespace)), spec, &counts );
    const ::hid_t memspace = H5Screate_simple( counts.size(),
					       counts.arr(),
					       nullptr );
    mCatchHDF( memspace,
	H5Sclose( filespace );
	mAddErrDuringWrite();
	return );

    const ::hid_t dtype = H5Dget_type( h5ds.asInt() );
    mCatchHDF( dtype,
	H5Sclose( memspace );
	H5Sclose( filespace );
	mAddErrDuringWrite();
	return );

    if ( H5Dwrite(h5ds.asInt(), dtype, memspace,
		  filespace, H5P_DEFAULT, data) < 0 )
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );

    H5Tclose( dtype );
    H5Sclose( memspace );
    H5Sclose( filespace );
}


void HDF5::WriterImpl::ptAll( const void* data, const DatasetID& h5ds,
			      uiRetVal& uirv )
{
    Threads::Locker locker( lock_ );
    if ( !h5ds.isValid() || !data )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
	return;
    }

    const ::hid_t filespace = H5Dget_space( h5ds.asInt() );
    mCatchErrDuringWrite( filespace );
    const ::hid_t dtype = H5Dget_type( h5ds.asInt() );
    mCatchHDF( dtype,
	H5Sclose( filespace );
	mAddErrDuringWrite();
	return );

    H5Sselect_all( filespace );
    if ( H5Dwrite(h5ds.asInt(), dtype, filespace,
		  filespace, H5P_DEFAULT, data) < 0 )
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );

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
    const ::hid_t grpid = grpobj.asInt();
    if ( !ds.isValid() || H5Iis_valid(ds.asInt()) <= 0 )
    {
	if ( H5Lexists(grpid,dsnm,H5P_DEFAULT) > 0 )
	{
	    const ::hid_t dsid = H5Dopen2( grpid, dsnm, H5P_DEFAULT );
	    mCatchErrDuringWrite( dsid );

	    const ::hid_t oldid = dataset_.asInt();
	    dataset_.setUdf();
	    if ( oldid >= 0 && H5Iis_valid(oldid) > 0
	      && H5Iget_type(oldid) == H5I_DATASET )
		H5Dclose( oldid );

	    ds = DatasetID::get( mCast(hid_t,dsid) );
	    dataset_ = ds;
	}
	else
	{
	    const int sz = mMAX( (int)bss.size(), 1 );
	    TypeSet<hsize_t> dims( 1, (hsize_t)sz );
	    TypeSet<hsize_t> maxdims( 1, (hsize_t)H5S_UNLIMITED );
	    TypeSet<hsize_t> chunks( 1, (hsize_t)1 );

	    ::hid_t strtype = H5Tcopy( H5T_C_S1 );
	    mCatchErrDuringWrite( strtype );

	    H5Tset_size( strtype, H5T_VARIABLE );
	    ::hid_t space = H5Screate_simple( 1, dims.arr(),
						    maxdims.arr() );
	    mCatchHDF( space,
		H5Tclose( strtype );
		mAddErrDuringWrite();
		return );

	    ::hid_t dcpl = H5Pcreate( H5P_DATASET_CREATE );
	    mCatchHDF( dcpl,
		H5Sclose( space );
		H5Tclose( strtype );
		mAddErrDuringWrite();
		return );

	    H5Pset_chunk( dcpl, 1, chunks.arr() );
	    ::hid_t dsid = H5Dcreate2( grpid, dsnm, strtype, space,
					     H5P_DEFAULT, dcpl, H5P_DEFAULT );
	    H5Pclose( dcpl );
	    H5Sclose( space );
	    H5Tclose( strtype );
	    mCatchErrDuringWrite( dsid );

	    const ::hid_t oldid = dataset_.asInt();
	    dataset_.setUdf();
	    if ( oldid >= 0 && H5Iis_valid(oldid) > 0
	      && H5Iget_type(oldid) == H5I_DATASET )
		H5Dclose( oldid );

	    ds = DatasetID::get( mCast(hid_t,dsid) );
	    dataset_ = ds;
	}
    }

    if ( !bss.isEmpty() )
    {
	reSzDS( Array1DInfoImpl(bss.size()), ds, uirv );
	if ( !uirv.isOK() )
	    return;
    }

    const hsize_t nrstrs = bss.size();
    char** strs = new char* [nrstrs];
    for ( hsize_t i=0; i<nrstrs; i++ )
	strs[i] = (char*)bss.get( (int)i ).buf();

    ::hid_t dtype = H5Dget_type( ds.asInt() );
    mCatchHDF( dtype,
	delete [] strs;
	mAddErrDuringWrite();
	return );
    ::hid_t space = H5Dget_space( ds.asInt() );
    mCatchHDF( space,
	H5Tclose( dtype );
	delete [] strs;
	mAddErrDuringWrite();
	return );

    H5Sselect_all( space );
    if ( H5Dwrite(ds.asInt(), dtype, space, space, H5P_DEFAULT, strs) < 0 )
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );

    delete [] strs;
    H5Tclose( dtype );
    H5Sclose( space );
}


bool HDF5::WriterImpl::rmObj( const DataSetKey& dsky )
{
    Threads::Locker locker( lock_ );
    if ( !fileid_.isValid() )
	return false;

    const char* dsname = dsky.dataSetName();
    if ( !dsname || !*dsname )
	return false;

    if ( haveDataSet() )
    {
	const ::hid_t oldid = dataset_.asInt();
	dataset_.setUdf();
	if ( oldid >= 0 && H5Iis_valid(oldid) > 0
	  && H5Iget_type(oldid) == H5I_DATASET )
	    H5Dclose( oldid );
    }

    const GroupID grp = selectGroup( dsky.groupName() );
    if ( !grp.isValid() )
	return false;

    const ::hid_t grpid = grp.asInt();
    if ( H5Lexists(grpid,dsname,H5P_DEFAULT) <= 0 )
	return false;

    if ( H5Ldelete(grpid, dsname, H5P_DEFAULT) < 0 )
	return false;

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
    if ( !ky || !*ky || !h5scope.isValid() )
	return;

    const ::hid_t scope = h5scope.asInt();
    if ( scope < 0 )
	return;

    const ::hid_t strtype = H5Tcopy( H5T_C_S1 );
    mCatchHDF( strtype, return );

    H5Tset_size( strtype, H5T_VARIABLE );
    const ::hid_t space = H5Screate( H5S_SCALAR );
    mCatchHDF( space, H5Tclose( strtype ); return );

    if ( H5Aexists(scope, ky) > 0 )
	H5Adelete( scope, ky );

    const ::hid_t attr = H5Acreate2( scope, ky,
				     strtype, space,
				     H5P_DEFAULT,
				     H5P_DEFAULT );
    mCatchHDF( attr,
	H5Sclose( space );
	H5Tclose( strtype );
	return );

    const char* s = (val && *val) ? val : "";
    H5Awrite( attr, strtype, &s );
    H5Aclose( attr );

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
