/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5accessimpl.h"

#include "envvars.h"
#include "hdf5readerimpl.h"
#include "hdf5writerimpl.h"
#include "od_ostream.h"

namespace HDF5{

namespace Lock{

Threads::Lock& hdf5InitLock()
{
    mDefineStaticLocalObject( Threads::Lock, thelock,
			      (false) );
    return thelock;
}

} // namespace Lock

static void closeDatasetHandle( HDF5::DatasetID& dsid )
{
    if ( !dsid.isValid() )
	return;

    const ::hid_t id = dsid.asInt();
    dsid.setUdf();
    if ( id >= 0 && H5Iis_valid(id) > 0
	 && H5Iget_type(id) == H5I_DATASET )
	H5Dclose( id );
}

static void closeGroupHandle( HDF5::GroupID& grpid, ::hid_t fileid )
{
    if ( !grpid.isValid() )
	return;

    const ::hid_t id = grpid.asInt();
    grpid.setUdf();
    if ( id >= 0 && !mIsUdf(id) && id != fileid
	 && H5Iis_valid(id) > 0
	 && H5Iget_type(id) == H5I_GROUP )
    {
	H5Gclose( id );
    }
}

static void closeGroupHandles( TypeSet<HDF5::hid_t>& grpidset,
			       ::hid_t fileid )
{
    for ( const HDF5::hid_t id : grpidset )
    {
	if ( id >= 0 && !mIsUdf(id) && id != fileid
	     && H5Iis_valid(id) > 0
	     && H5Iget_type(id) == H5I_GROUP )
	{
	    H5Gclose( id );
	}
    }

    grpidset.setEmpty();
}

} // namespace HDF5


HDF5::Reader* HDF5::AccessProviderImpl::getReader() const
{
    return new ReaderImpl;
}


HDF5::Writer* HDF5::AccessProviderImpl::getWriter() const
{
    return new WriterImpl;
}

static bool errprint_ = false;

void HDF5::AccessProviderImpl::initHDF5()
{
    Threads::Locker locker( Lock::hdf5InitLock() );
    initClass();
    const int defidx = factory().getNames().indexOf( sFactoryKeyword() );
    factory().setDefaultName( defidx );
    H5open();
    H5Eset_auto2( H5E_DEFAULT, nullptr, nullptr );
    H5close();
}

HDF5::AccessImpl::AccessImpl( ReaderImpl& rdr )
    : acc_(rdr)
{
}


HDF5::AccessImpl::AccessImpl( WriterImpl& wrr )
    : acc_(wrr)
{
}


HDF5::AccessImpl::~AccessImpl()
{
    Threads::Locker locker( lock_ );
    closeDatasetHandle( dataset_ );
    closeGroupHandles( previousgroupids_,
		       acc_.fileid_.isValid() ? acc_.fileid_.asInt() : -1 );
    closeGroupHandle( group_, acc_.fileid_.isValid()
		      ? acc_.fileid_.asInt() : -1 );
}

bool HDF5::AccessImpl::haveErrPrint()
{
    return errprint_;
}

void HDF5::AccessImpl::setErrPrint( bool yn )
{
    errprint_ = yn;
    if ( !yn )
	disableErrPrint();
}


void HDF5::AccessImpl::disableErrPrint()
{
    H5Eset_auto2( H5E_DEFAULT, nullptr, nullptr );
}


namespace HDF5
{

const char* HDF5::AccessImpl::gtFileName() const
{
    if ( !acc_.fileid_.isValid() )
	return nullptr;

    mDeclStaticString( res );
    res.setBufSize( 2048 );
    if ( H5Fget_name(acc_.fileid_.asInt(), res.getCStr(),
		     res.bufSize() ) < 0 )
	return nullptr;

    return res.buf();
}


bool HDF5::AccessImpl::atGroup( const char*& grpnm ) const
{
    if ( !grpnm || !*grpnm )
	grpnm = "/";

    if ( !group_.isValid() )
	return StringView( grpnm ) == "/";

    BufferString objname (2048, true);
    if ( H5Iget_name(group_.asInt(), objname.getCStr(),
		     objname.bufSize()) <= 0 )
	return false;

    return StringView( grpnm ) == objname;
}


HDF5::DataSetKey HDF5::AccessImpl::gtScope() const
{
    BufferString grpname, dsname;
    if ( dataset_.isValid() )
    {
	BufferString buf (2048, true);
	if ( H5Iget_name(dataset_.asInt(), buf.getCStr(),
			 buf.bufSize()) > 0 )
	{
	    BufferString fullpath( buf );
	    const char* lastslash = fullpath.findLast( '/' );
	    if ( lastslash && *(lastslash+1) )
		dsname.set( lastslash + 1 );

	    if ( lastslash > fullpath.buf() )
	    {
		BufferString grppart( fullpath );
		grppart.getCStr()[lastslash - fullpath.buf()] = '\0';
		grpname = grppart;
	    }
	}
    }

    else if ( group_.isValid() )
    {
	BufferString buf (2048, true);
	if ( H5Iget_name(group_.asInt(), buf.getCStr(),
			 buf.bufSize()) > 0 )
	    grpname.set( buf );
    }

    return DataSetKey( grpname, dsname );
}


od_int64 HDF5::AccessImpl::gtGroupID() const
{
    if ( group_.isValid() )
	return group_.asInt();

    return acc_.fileid_.isValid() ? acc_.fileid_.asInt() : -1;
}


bool HDF5::AccessImpl::atDataSet( const char* dsnm ) const
{
    if ( !dsnm || !*dsnm || !dataset_.isValid() )
	return false;

    char objname[2048];
    if ( H5Iget_name(dataset_.asInt(), objname, sizeof(objname)) <= 0 )
	return false;

    BufferString nm( objname );
    const char* lastslash = nm.findLast( '/' );
    const BufferString basename( lastslash ? lastslash + 1 : nm.buf() );
    return basename == dsnm;
}



HDF5::DatasetID HDF5::AccessImpl::selectDataSet( const char* dsnm ) const
{
    if ( !dsnm || !*dsnm )
	return DatasetID::get( H5I_INVALID_HID );

    if ( atDataSet(dsnm) )
	return dataset_;

    if ( !group_.isValid() )
	return DatasetID::get( H5I_INVALID_HID );

    const ::hid_t grpid = group_.asInt();
    if ( H5Lexists(grpid, dsnm, H5P_DEFAULT) <= 0 )
	return DatasetID::get( H5I_INVALID_HID );

    const ::hid_t dsid = H5Dopen2( grpid, dsnm,
				   H5P_DEFAULT );
    if ( dsid < 0 )
	return DatasetID::get( H5I_INVALID_HID );

    AccessImpl& self = const_cast<AccessImpl&>( *this );
    closeDatasetHandle( self.dataset_ );

    self.dataset_ = DatasetID::get( mCast(hid_t,dsid) );
    const ::hid_t space = H5Dget_space( dsid );
    self.nrdims_ = H5Sget_simple_extent_ndims( space );
    H5Sclose( space );
    return self.dataset_;
}

HDF5::LocationID HDF5::AccessImpl::stLocation( const DataSetKey* ) const
{
    if ( !acc_.fileid_.isValid() )
	return LocationID::get( H5I_INVALID_HID );

    return LocationID::get( mCast(hid_t,acc_.fileid_.asInt()) );
}


HDF5::LocationID HDF5::AccessImpl::stLocation( const DataSetKey* dsky )
{
    return const_cast<const AccessImpl*>( this )->stLocation( dsky );
}


HDF5::ObjectID HDF5::AccessImpl::stScope( const DataSetKey* dsky )
{
    return const_cast<const AccessImpl*>( this )->stScope( dsky );
}


HDF5::ObjectID HDF5::AccessImpl::stScope( const DataSetKey* dsky ) const
{
    if ( !acc_.fileid_.isValid() )
	return ObjectID::get( H5I_INVALID_HID );

    if ( !dsky )
	return ObjectID::get( acc_.fileid_.asInt() );

    if ( dsky->dataSetEmpty() )
    {
	const GroupID grp = stGrpScope( dsky );
	return grp.isValid() ? ObjectID::get( grp.asInt() )
		     : ObjectID::get( H5I_INVALID_HID );
    }

    const DatasetID ds = stDSScope( *dsky );
    return ds.isValid()
	? ObjectID::get( ds.asInt() )
	: ObjectID::get( H5I_INVALID_HID );
}


HDF5::GroupID AccessImpl::selectGroup( const char* grpnm ) const
{
    if ( !acc_.fileid_.isValid() )
	return GroupID::get( H5I_INVALID_HID );

    AccessImpl& self = const_cast<AccessImpl&>( *this );
    const char* path = (!grpnm || !*grpnm || StringView(grpnm)=="/")
		       ? "/"
		       : grpnm;
    const hid_t fid = acc_.fileid_.asInt();
    if ( StringView(path) != "/" )
    {
	htri_t exists = H5Lexists( fid, path, H5P_DEFAULT );
	if ( exists <= 0 )
	    return GroupID::get( H5I_INVALID_HID );
    }

    hid_t grpid = H5Gopen2( fid, path, H5P_DEFAULT );
    if ( grpid < 0 )
	return GroupID::get( H5I_INVALID_HID );

    if ( self.group_.isValid() )
	self.previousgroupids_.add( self.group_.asInt() );

    self.group_ = GroupID::get( mCast(hid_t,grpid) );
    return self.group_;
}


HDF5::GroupID HDF5::AccessImpl::stGrpScope( const DataSetKey* dsky )
{
    if ( !acc_.fileid_.isValid() )
	return GroupID::get( H5I_INVALID_HID );

    if ( !dsky || !dsky->groupName()[0] )
	return selectGroup( "/" );

    return selectGroup( dsky->groupName() );
}


HDF5::GroupID HDF5::AccessImpl::stGrpScope( const DataSetKey* dsky ) const
{
    return const_cast<AccessImpl*>( this )->stGrpScope( dsky );
}


HDF5::DatasetID HDF5::AccessImpl::stDSScope( const DataSetKey& dsky )
{
    return const_cast<const AccessImpl*>( this )->stDSScope( dsky );
}


HDF5::DatasetID HDF5::AccessImpl::stDSScope( const DataSetKey& dsky ) const
{
    AccessImpl& self = const_cast<AccessImpl&>( *this );
    DatasetID invalidid;
    invalidid.setUdf();

    if ( !acc_.fileid_.isValid() )
    {
	if ( self.dataset_.isValid() )
	{
	    const ::hid_t oldds = self.dataset_.asInt();
	    self.dataset_.setUdf();
	    if ( oldds > 0 && H5Iis_valid(oldds) > 0 )
		H5Dclose( oldds );
	}
	self.nrdims_ = -1;
	return invalidid;
    }

    BufferString path = dsky.fullDataSetName();
    if ( !path.startsWith("/") )
	path.insertAt( 0, "/" );

    const ::hid_t fileid = acc_.fileid_.asInt();
    if ( H5Lexists(fileid, path.buf(), H5P_DEFAULT) <= 0 )
    {
	if ( self.dataset_.isValid() )
	{
	    const ::hid_t oldds = self.dataset_.asInt();
	    self.dataset_.setUdf();
	    if ( oldds > 0 && H5Iis_valid(oldds) > 0 )
		H5Dclose( oldds );
	}
	self.nrdims_ = -1;
	return invalidid;
    }

    const ::hid_t dsid = H5Dopen2( fileid, path.buf(),
				   H5P_DEFAULT );
    if ( dsid < 0 )
    {
	if ( self.dataset_.isValid() )
	{
	    const ::hid_t oldds = self.dataset_.asInt();
	    self.dataset_.setUdf();
	    if ( oldds > 0 && H5Iis_valid(oldds) > 0 )
		H5Dclose( oldds );
	}
	self.nrdims_ = -1;
	return invalidid;
    }

    if ( self.dataset_.isValid() )
    {
	const ::hid_t oldds = self.dataset_.asInt();
	self.dataset_.setUdf();
	if ( oldds > 0 && H5Iis_valid(oldds) > 0 )
	    H5Dclose( oldds );
    }

    const ::hid_t space = H5Dget_space( dsid );
    self.nrdims_ = H5Sget_simple_extent_ndims( space );
    H5Sclose( space );
    self.dataset_ = DatasetID::get( mCast(hid_t,dsid) );
    return self.dataset_;
}

bool HDF5::AccessImpl::validH5Obj( const ObjectID& obj )
{
    return obj.isValid() && H5Iis_valid( obj.asInt() ) > 0;
}

bool HDF5::AccessImpl::haveGroup() const
{
    return group_.isValid() && validH5Obj( ObjectID::get(group_.asInt()) );
}

bool HDF5::AccessImpl::haveDataSet() const
{
    return dataset_.isValid() && validH5Obj( ObjectID::get(dataset_.asInt()) );
}

void HDF5::AccessImpl::doCloseFile( Access& acc )
{
    const ::hid_t did = dataset_.asInt();
    dataset_.setUdf();
    if ( did >= 0 && H5Iis_valid(did) > 0 )
    {
	H5Dclose( did );
    }

    const ::hid_t gid = group_.asInt();
    group_.setUdf();
    if ( gid >= 0 && H5Iis_valid(gid) > 0 )
    {
	H5Gclose( gid );
    }

    closeGroupHandles( previousgroupids_, acc.fileid_.asInt() );
    nrdims_ = -1;

    if ( !acc.myfile_ )
	return;

    const ::hid_t fid = acc.fileid_.asInt();
    acc.fileid_.setUdf();
    if ( fid >= 0 && H5Iis_valid(fid) > 0 )
    {
	H5Fclose( fid );
    }
}

HDF5::DatatypeID HDF5::AccessImpl::h5DataTypeFor( ODDataType datarep )
{
#   define mHandleCase(odtyp,hdftyp) \
    case OD::odtyp: return DatatypeID::get( mCast(hid_t,H5T_NATIVE_##hdftyp) );

    switch ( datarep )
    {
	mHandleCase( SI8, INT8 );
	mHandleCase( UI8, UINT8 );
	mHandleCase( SI16, INT16 );
	mHandleCase( UI16, UINT16 );
	mHandleCase( SI32, INT32 );
	mHandleCase( UI32, UINT32 );
	mHandleCase( SI64, INT64 );
	mHandleCase( UI64, UINT64 );
	mHandleCase( F32, FLOAT );
	mHandleCase( F64, DOUBLE );
	default: break;
    }
#undef mHandleCase

    return DatatypeID::get( mCast(hid_t,H5T_NATIVE_FLOAT) );
}



void HDF5::AccessImpl::selectSlab( const DataspaceID& ds,
	const SlabSpec& spec,
	TypeSet<hsize_t>* pcounts ) const
{
    TypeSet<hsize_t> counts, offss, strides;
    if ( !pcounts ) pcounts = &counts;

    const ::hid_t spaceid = ds.asInt();
    const int nrdims = H5Sget_simple_extent_ndims( spaceid );
    TypeSet<hsize_t> dimsizes( nrdims, 0 );
    H5Sget_simple_extent_dims( spaceid, dimsizes.arr(),
			       nullptr );
    for ( int idim=0; idim<nrdims; idim++ )
    {
	SlabDimSpec sds = spec[idim];
	if ( sds.count_ < 0 )
	{
	    sds.count_ = (dimsizes[idim]-sds.start_) / sds.step_;
	}

	*pcounts += sds.count_;
	offss += sds.start_;
	strides += sds.step_;
    }
    H5Sselect_hyperslab( spaceid, H5S_SELECT_SET,
			 offss.arr(), strides.arr(),
			 pcounts->arr(),    nullptr );
}

} // namespace HDF5
