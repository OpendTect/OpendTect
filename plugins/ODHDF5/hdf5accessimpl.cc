/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "envvars.h"
#include "file.h"
#include "hdf5readerimpl.h"
#include "hdf5writerimpl.h"
#include "uistrings.h"


HDF5::Reader* HDF5::AccessProviderImpl::getReader() const
{
    return new ReaderImpl;
}


HDF5::Writer* HDF5::AccessProviderImpl::getWriter() const
{
    return new WriterImpl;
}


static H5E_auto_t hdf_err_print_fn;
static void* hdf_err_client_data;
static bool errprint_ = false;

void HDF5::AccessProviderImpl::initHDF5()
{
    initClass();

    H5::Exception::getAutoPrint( hdf_err_print_fn, &hdf_err_client_data );
#ifdef __debug__
    errprint_ = true;
#endif
    AccessImpl::setErrPrint( errprint_ );
}


HDF5::AccessImpl::AccessImpl( ReaderImpl& rdr )
    : acc_(rdr)
    , nrdims_(-1)
{
}


HDF5::AccessImpl::AccessImpl( WriterImpl& wrr )
    : acc_(wrr)
{
}


HDF5::AccessImpl::~AccessImpl()
{
}


bool HDF5::AccessImpl::haveErrPrint()
{
    return errprint_;
}


void HDF5::AccessImpl::setErrPrint( bool yn )
{
    errprint_ = yn;
    if ( yn )
	enableErrPrint();
    else
	disableErrPrint();
}


void HDF5::AccessImpl::disableErrPrint()
{
    H5::Exception::dontPrint();
}


void HDF5::AccessImpl::enableErrPrint()
{
    H5::Exception::setAutoPrint( hdf_err_print_fn, hdf_err_client_data );
}


void HDF5::AccessImpl::restoreErrPrint()
{
    if ( errprint_ )
	enableErrPrint();
}


const char* HDF5::AccessImpl::gtFileName() const
{
    if ( !acc_.file_ )
	return 0;

    mDeclStaticString(ret);
    ret.set( acc_.file_->getFileName().c_str() );
    return ret.str();
}


bool HDF5::AccessImpl::atGroup( const char*& grpnm ) const
{
    if ( !grpnm || !*grpnm )
	grpnm = "/";
    return haveGroup() && group_.getObjName() == grpnm;
}


HDF5::DataSetKey HDF5::AccessImpl::gtScope() const
{
    return DataSetKey( haveGroup() ? group_.getObjName().c_str() : "",
		       haveDataSet() ? dataset_.getObjName().c_str() : "" );
}


od_int64 HDF5::AccessImpl::gtGroupID() const
{
    const od_int64 grpid = haveGroup() ? group_.getLocId() : -1;
    if ( grpid < 0 )
	  return acc_.file_ ? acc_.file_->getLocId() : -1;
    return grpid;
}


bool HDF5::AccessImpl::atDataSet( const char* dsnm ) const
{
    return dsnm && *dsnm && haveDataSet() && dataset_.getObjName() == dsnm;
}


H5::Group* HDF5::AccessImpl::selectGroup( const char* grpnm ) const
{
    if ( !acc_.file_ )
	return nullptr;

    if ( !grpnm || !*grpnm )
	grpnm = "/";
    else
    {
	if ( atGroup(grpnm) )
	    return &group_;
	else if ( StringView(grpnm) != "/" &&
	  !H5Lexists(acc_.file_->getId(),grpnm,H5P_DEFAULT) )
	    return nullptr;
    }

    bool haveerr = false;
    try
    {
	group_ = acc_.file_->openGroup( grpnm );
    }
    mCatchAnyNoMsg( haveerr = true )

    return haveerr ? nullptr : &group_;
}


H5::DataSet* HDF5::AccessImpl::selectDataSet( const char* dsnm ) const
{
    if ( !dsnm || !*dsnm )
	return nullptr;
    else if ( atDataSet(dsnm) )
	return &dataset_;
    else if ( !haveGroup() || !H5Lexists(group_.getId(),dsnm,H5P_DEFAULT) )
	return nullptr;

    bool haverr = false;
    try
    {
	dataset_ = group_.openDataSet( dsnm );
	nrdims_ = (ArrayNDInfo::nr_dims_type)dataset_.getSpace()
						.getSimpleExtentNdims();
    }
    mCatchAnyNoMsg( haverr = true )

    return haverr ? nullptr : &dataset_;
}


H5::H5Object* HDF5::AccessImpl::stScope( const DataSetKey* dsky )
{
    H5::Group* ret = stGrpScope( dsky );
    return !dsky || dsky->dataSetEmpty() ? (H5::H5Object*)ret
					 : (H5::H5Object*)stDSScope( *dsky );
}


H5::H5Object* HDF5::AccessImpl::stScope( const DataSetKey* dsky ) const
{
    AccessImpl& accimpl = const_cast<AccessImpl&>( *this );
    return accimpl.stScope( dsky );
}


H5::Group* HDF5::AccessImpl::stGrpScope( const DataSetKey* dsky )
{
    if ( !dsky )
	return acc_.file_;

    return selectGroup( dsky->groupName() );
}


H5::Group* HDF5::AccessImpl::stGrpScope( const DataSetKey* dsky ) const
{
    AccessImpl& accimpl = const_cast<AccessImpl&>( *this );
    return accimpl.stGrpScope( dsky );
}


H5::DataSet* HDF5::AccessImpl::stDSScope( const DataSetKey& dsky )
{
    if ( !haveGroup() || BufferString(dsky.groupName()) !=
				StringView( group_.getObjName().c_str() ) )
    {
	H5::Group* grp = stGrpScope( &dsky );
	if ( !grp )
	    return nullptr;
    }

    return selectDataSet( dsky.dataSetName() );
}


H5::DataSet* HDF5::AccessImpl::stDSScope( const DataSetKey& dsky ) const
{
    AccessImpl& accimpl = const_cast<AccessImpl&>( *this );
    return accimpl.stDSScope( dsky );
}


bool HDF5::AccessImpl::validH5Obj( const H5::H5Object& obj )
{
    return obj.getId() >= 0 && !obj.getObjName().empty();
}


bool HDF5::AccessImpl::haveGroup() const
{
    return validH5Obj( group_ );
}


bool HDF5::AccessImpl::haveDataSet() const
{
    return validH5Obj( dataset_ );
}


void HDF5::AccessImpl::doCloseFile( Access& acc )
{
    // cannot use acc_ here, it may already have been deleted
    // Thus, acc needs to be passed to this function

    if ( !acc.file_ || !acc.myfile_ )
	return;

    H5::H5File* h5file = acc.file_;
    acc.file_ = nullptr;
    try
    {
	H5Fclose( h5file->getId() );
	delete h5file;
    }
    mCatchUnexpected( return )
}


const H5::PredType& HDF5::AccessImpl::h5DataTypeFor( ODDataType datarep )
{
#   define mHandleCase(odtyp,hdftyp) \
	case OD::odtyp:	    return H5::PredType::hdftyp;

    switch ( datarep )
    {
	mHandleCase( SI8, STD_I8LE );
	mHandleCase( UI8, STD_U8LE );
	mHandleCase( SI16, STD_I16LE );
	mHandleCase( UI16, STD_U16LE );
	mHandleCase( SI32, STD_I32LE );
	mHandleCase( UI32, STD_U32LE );
	mHandleCase( SI64, STD_I64LE );
	mHandleCase( F64, IEEE_F64LE );
	default: break;
    }

    return H5::PredType::IEEE_F32LE;
}


void HDF5::AccessImpl::selectSlab( H5::DataSpace& ds, const SlabSpec& spec,
				   TypeSet<hsize_t>* pcounts ) const
{
    TypeSet<hsize_t> counts, offss, strides;
    if ( !pcounts )
	pcounts = &counts;
	const Access::nr_dims_type nrdims
		= mCast(Access::nr_dims_type,spec.size());
    mGetDataSpaceDims( dimsizes, nrdims, ds );

    for ( Access::dim_idx_type idim=0; idim<nrdims; idim++ )
    {
	SlabDimSpec sds = spec[idim];
	if ( sds.count_ < 0 )
	    sds.count_ = (((ArrayNDInfo::idx_type)dimsizes[idim])-sds.start_)
						/ sds.step_;
	*pcounts += sds.count_;
	offss += sds.start_;
	strides += sds.step_;
    }

    ds.selectHyperslab( H5S_SELECT_SET,
			pcounts->arr(), offss.arr(), strides.arr() );
}
