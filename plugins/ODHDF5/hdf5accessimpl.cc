/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hdf5accessimpl.h"

#include "envvars.h"
//#include "filepath.h"
#include "hdf5common.h"
#include "hdf5readerimpl.h"
#include "hdf5writerimpl.h"
#include "od_ostream.h"

namespace HDF5 {

Threads::Lock& hdf5InitLock()
{
    mDefineStaticLocalObject( Threads::Lock, thelock,
			      (false) );
    return thelock;
}


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

static BufferString getH5ObjName( ::hid_t id )
{
    BufferString buf( 2048, true );
    if ( H5Iget_name(id,buf.getCStr(),buf.bufSize()) > 0 )
	return buf;

    return BufferString::empty();
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
    Threads::Locker locker( hdf5InitLock() );
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


const char* HDF5::AccessImpl::gtFileName() const
{
    if ( !acc_.fileid_.isValid() )
	return nullptr;

    mDeclStaticString( res );
    res.setBufSize( 2048 );
    if ( H5Fget_name(acc_.fileid_.asInt(),res.getCStr(),res.bufSize() ) < 0 )
	return nullptr;

    return res.buf();
}


bool HDF5::AccessImpl::atGroup( const char*& grpnm ) const
{
    if ( !grpnm || !*grpnm )
	grpnm = "/";

    if ( !haveGroup() )
	return false;

    const BufferString objname = getH5ObjName( group_.asInt() );
    return objname.isEmpty() ? false : StringView( grpnm ) == objname;
}


HDF5::DataSetKey HDF5::AccessImpl::gtScope() const
{
    BufferString grpname, dsname;
    if ( haveGroup() )
	grpname = getH5ObjName( group_.asInt() );

    if ( haveDataSet() )
	dsname = getH5ObjName( dataset_.asInt() );

    return DataSetKey( grpname.buf(), dsname.buf() );
}


od_int64 HDF5::AccessImpl::gtGroupID() const
{
    const od_int64 grpid = haveGroup() ? group_.asInt() : -1;
    if ( grpid < 0 )
	return acc_.fileid_.isValid() ? acc_.fileid_.asInt() : -1;

    return grpid;
}


bool HDF5::AccessImpl::atDataSet( const char* dsnm ) const
{
    if ( !dsnm || !*dsnm || !haveDataSet() )
	return false;

    const BufferString dsname = getH5ObjName( dataset_.asInt() );
    return dsname.isEmpty() ? false : dsname == dsnm;
}


HDF5::GroupID HDF5::AccessImpl::selectGroup( const char* grpnm ) const
{
    if ( !acc_.fileid_.isValid() )
	return GroupID::udf();

    if ( !grpnm || !*grpnm )
	grpnm = "/";
    else
    {
	if ( atGroup(grpnm) )
	    return group_;
	else if ( StringView(grpnm) != "/" &&
		!H5Lexists(acc_.fileid_.asInt(),grpnm,H5P_DEFAULT) )
	    return GroupID::udf();
    }

    bool haveerr = false;
    hid_t grpid = GroupID::udf().asInt();
    try
    {
	grpid = H5Gopen2( acc_.fileid_.asInt(), grpnm, H5P_DEFAULT );
    }
    mCatchAnyNoMsg( haveerr = true )

    if ( haveerr )
	return GroupID::udf();

    group_ = GroupID::get( mCast(hid_t,grpid) );
    previousgroupids_.add( grpid );
    return group_;
}


HDF5::DatasetID HDF5::AccessImpl::selectDataSet( const char* dsnm ) const
{
    if ( !dsnm || !*dsnm )
	return DatasetID::udf();

    if ( atDataSet(dsnm) )
	return dataset_;

    if ( !haveGroup() )
	return DatasetID::udf();

    const ::hid_t grpid = group_.asInt();
    bool haverr = false;
    try
    {
	if ( !H5Lexists(grpid,dsnm,H5P_DEFAULT) )
	    return DatasetID::udf();

	const ::hid_t dsid = H5Dopen2( grpid, dsnm, H5P_DEFAULT );
	if ( dsid < 0 )
	    return DatasetID::udf();

	closeDatasetHandle( dataset_ );
	dataset_ = DatasetID::get( mCast(hid_t,dsid) );
	const ::hid_t space = H5Dget_space( dsid );
	nrdims_ = H5Sget_simple_extent_ndims( space );
	H5Sclose( space );
    }
    mCatchAnyNoMsg( haverr = true )

    return haverr ? DatasetID::udf() : dataset_;
}


HDF5::LocationID HDF5::AccessImpl::stLocation( const DataSetKey* dsky ) const
{
    return getNonConst( this )->stLocation( dsky );
}


HDF5::LocationID HDF5::AccessImpl::stLocation( const DataSetKey* dsky )
{
    const ObjectID objid = stScope( dsky );
    return LocationID( objid.asInt() );
}


HDF5::ObjectID HDF5::AccessImpl::stScope( const DataSetKey* dsky ) const
{
    return getNonConst( this )->stScope( dsky );
}


HDF5::ObjectID HDF5::AccessImpl::stScope( const DataSetKey* dsky )
{
    const GroupID grpid = stGrpScope( dsky );
    if ( !dsky || dsky->dataSetEmpty() )
	return ObjectID( grpid.asInt() );

    const DatasetID dsid = stDSScope( *dsky );
    return ObjectID( dsid.asInt() );
}


HDF5::GroupID HDF5::AccessImpl::stGrpScope( const DataSetKey* dsky ) const
{
    return getNonConst( this )->stGrpScope( dsky );
}


HDF5::GroupID HDF5::AccessImpl::stGrpScope( const DataSetKey* dsky )
{
    if ( !dsky )
	return GroupID( acc_.fileid_.asInt() );

    return selectGroup( dsky->groupName() );
}


HDF5::DatasetID HDF5::AccessImpl::stDSScope( const DataSetKey& dsky ) const
{
    return getNonConst( this )->stDSScope( dsky );
}


HDF5::DatasetID HDF5::AccessImpl::stDSScope( const DataSetKey& dsky )
{
    if ( !haveGroup() || getH5ObjName(group_.asInt()) != dsky.groupName() )
    {
	const GroupID grpid = stGrpScope( &dsky );
	if ( !grpid.isValid() )
	    return DatasetID::udf();
    }

    return selectDataSet( dsky.dataSetName() );
}


bool HDF5::AccessImpl::validH5Obj( const ObjectID& obj )
{
    return obj.asInt() >= 0 && !getH5ObjName( obj.asInt() ).isEmpty();
}


bool HDF5::AccessImpl::haveGroup() const
{
    return validH5Obj( ObjectID::get(group_.asInt()) );
}


bool HDF5::AccessImpl::haveDataSet() const
{
    return validH5Obj( ObjectID::get(dataset_.asInt()) );
}


void HDF5::AccessImpl::doCloseFile( Access& acc )
{
    /*const ::hid_t did = dataset_.asInt();
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
    */

    if ( !acc.fileid_.isValid() || !acc.myfile_ )
	return;

    const ::hid_t fid = acc.fileid_.asInt();
    const BufferString filenm = gtFileName();
    acc.fileid_.setUdf();
    try
    {
#ifdef __debug__
	if ( DBG::isOn(DGB_HDF5) )
	    od_cout() << "Close: " << fid << " " << filenm << od_endl;
#endif
	H5Fclose( fid );
    }
    mCatchUnexpected( return )
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
    if ( !pcounts )
	pcounts = &counts;

    const ::hid_t spaceid = ds.asInt();
    const int nrdims = H5Sget_simple_extent_ndims( spaceid );
    TypeSet<hsize_t> dimsizes( nrdims, 0 );
    H5Sget_simple_extent_dims( spaceid, dimsizes.arr(), nullptr );
    for ( int idim=0; idim<nrdims; idim++ )
    {
	SlabDimSpec sds = spec[idim];
	if ( sds.count_ < 0 )
	    sds.count_ = (dimsizes[idim]-sds.start_) / sds.step_;

	*pcounts += sds.count_;
	offss += sds.start_;
	strides += sds.step_;
    }
    H5Sselect_hyperslab( spaceid, H5S_SELECT_SET,
			 offss.arr(), strides.arr(),
			 pcounts->arr(), nullptr );
}
