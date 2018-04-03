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
    , group_(0)
    , dataset_(0)
    , nrdims_(-1)
{
}


HDF5::AccessImpl::AccessImpl( WriterImpl& wrr )
    : acc_(wrr)
    , group_(0)
    , dataset_(0)
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
    return group_ && group_->getObjName() == grpnm;
}


HDF5::DataSetKey HDF5::AccessImpl::gtScope() const
{
    if ( !group_ )
	return DataSetKey();

    DataSetKey ret( group_->getObjName().c_str() );
    if ( dataset_ )
	ret.setDataSetName( dataset_->getObjName().c_str() );

    return ret;
}


od_int64 HDF5::AccessImpl::gtGroupID() const
{
    return group_ ? (od_int64)group_->getLocId()
		  : (acc_.file_ ? acc_.file_->getLocId() : -1);
}


bool HDF5::AccessImpl::atDataSet( const char* dsnm ) const
{
    return !dataset_ || !dsnm || !*dsnm ? false
	 : dataset_->getObjName() == dsnm;
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
	H5::Group grp = acc_.file_->openGroup( grpnm );
	delete dataset_; dataset_ = 0;
	delete group_; group_ = 0;
	group_ = new H5::Group( grp );
    }
    mCatchAnyNoMsg( haveselected = false )

    restoreErrPrint();
    return haveselected;
}


bool HDF5::AccessImpl::selectDataSet( const char* dsnm )
{
    if ( !group_ )
	{ pErrMsg("check successful selectGroup"); return false; }
    else if ( !dsnm || !*dsnm )
	return false;
    else if ( atDataSet(dsnm) )
	return true;

    bool haveselected = true;
    disableErrPrint();

    try
    {
	H5::DataSet ds = group_->openDataSet( dsnm );
	delete dataset_; dataset_ = 0; nrdims_ = -1;
	dataset_ = new H5::DataSet( ds );
	nrdims_ = (ArrayNDInfo::NrDimsType)dataset_->getSpace()
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


void HDF5::AccessImpl::doCloseFile( Access& acc )
{
    // cannot use acc_ here, it may already have been deleted
    // Thus, acc needs to be passed to this function

    if ( !acc.file_ )
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


H5::DataType HDF5::AccessImpl::h5DataTypeFor( ODDataType datarep )
{
    H5DataType ret = H5::PredType::IEEE_F32LE;

#   define mHandleCase(od,hdf) \
	case OD::od:	    ret = H5::PredType::hdf; break

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

    return ret;
}


void HDF5::AccessImpl::selectSlab( H5::DataSpace& ds, const SlabSpec& spec,
				   TypeSet<hsize_t>* pcounts ) const
{
    TypeSet<hsize_t> counts, offss, strides;
    if ( !pcounts )
	pcounts = &counts;
    const Access::NrDimsType nrdims = spec.size();
    mGetDataSpaceDims( dimsizes, nrdims, ds );

    for ( Access::DimIdxType idim=0; idim<nrdims; idim++ )
    {
	SlabDimSpec sds = spec[idim];
	if ( sds.count_ < 0 )
	    sds.count_ = (dimsizes[idim]-sds.start_) / sds.step_;
	*pcounts += sds.count_;
	offss += sds.start_;
	strides += sds.step_;
    }

    ds.selectHyperslab( H5S_SELECT_SET,
			pcounts->arr(), offss.arr(), strides.arr() );
}
