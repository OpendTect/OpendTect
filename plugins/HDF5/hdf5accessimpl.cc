/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5readerimpl.h"
#include "hdf5writerimpl.h"
#include "uistrings.h"
#include "file.h"
#include "envvars.h"
#include "H5Cpp.h"


HDF5::Reader* HDF5::AccessProviderImpl::getReader() const
{
    return new HDF5::ReaderImpl;
}


HDF5::Writer* HDF5::AccessProviderImpl::getWriter() const
{
    return new HDF5::WriterImpl;
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
    const od_int64 grpid = group_.getLocId();
    if ( grpid < 0 )
	  return acc_.file_ ? acc_.file_->getLocId() : -1;
    return grpid;
}


bool HDF5::AccessImpl::atDataSet( const char* dsnm ) const
{
    return dsnm && *dsnm && haveDataSet() && dataset_.getObjName() == dsnm;
}


bool HDF5::AccessImpl::selectGroup( const char* grpnm )
{
    if ( !grpnm || !*grpnm )
	grpnm = "/";
    if ( atGroup(grpnm) )
	return true;
    else if ( !acc_.file_ )
	return false;

    bool haveselected = true;
    disableErrPrint();

    try
    {
	group_ = acc_.file_->openGroup( grpnm );
	dataset_ = H5::DataSet();
    }
    mCatchAnyNoMsg( haveselected = false )

    restoreErrPrint();
    return haveselected;
}


bool HDF5::AccessImpl::selectDataSet( const char* dsnm )
{
    if ( !dsnm || !*dsnm )
	return false;
    else if ( atDataSet(dsnm) )
	return true;

    bool haveselected = true;
    disableErrPrint();

    try
    {
	dataset_ = group_.openDataSet( dsnm );
	nrdims_ = (ArrayNDInfo::NrDimsType)dataset_.getSpace()
						.getSimpleExtentNdims();
    }
    mCatchAnyNoMsg( haveselected = false )

    restoreErrPrint();
    return haveselected;
}


bool HDF5::AccessImpl::stScope( const DataSetKey& dsky )
{
    if ( !selectGroup(dsky.groupName()) )
	return false;
    else if ( dsky.dataSetEmpty() )
	return true;

    return selectDataSet( dsky.dataSetName() );
}


bool HDF5::AccessImpl::validH5Obj( const H5::H5Object& obj )
{
    return obj.getId() >= 0 && !obj.getObjName().empty();
}


bool HDF5::AccessImpl::haveScope( bool needds ) const
{
    if ( !haveGroup() )
	return false;

    return !needds || haveDataSet();
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
    acc.file_ = 0;
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
	const Access::NrDimsType nrdims = mCast(Access::NrDimsType,spec.size());
    mGetDataSpaceDims( dimsizes, nrdims, ds );

    for ( Access::DimIdxType idim=0; idim<nrdims; idim++ )
    {
	SlabDimSpec sds = spec[idim];
	if ( sds.count_ < 0 )
	    sds.count_ = ((mCast(ArrayNDInfo::IdxType,dimsizes[idim]))-sds.start_)
						/ sds.step_;
	*pcounts += sds.count_;
	offss += sds.start_;
	strides += sds.step_;
    }

    ds.selectHyperslab( H5S_SELECT_SET,
			pcounts->arr(), offss.arr(), strides.arr() );
}
