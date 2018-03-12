/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5readerimpl.h"
#include "uistrings.h"
#include "arrayndinfo.h"
#include "file.h"
#include "iopar.h"


HDF5::ReaderImpl::ReaderImpl()
    : AccessImpl(*this)
    , group_(0)
    , dataset_(0)
{
}


HDF5::ReaderImpl::~ReaderImpl()
{
    closeFile();
}


void HDF5::ReaderImpl::openFile( const char* fnm, uiRetVal& uirv )
{
    if ( !File::exists(fnm) )
	{ uirv.add( uiStrings::phrCannotOpen( fnm ) ); return; }

    try
    {
	grpnms_.setEmpty();
	file_ = new H5::H5File( fnm, H5F_ACC_RDONLY );
	listObjs( *file_, grpnms_, true );
    }
    mCatchAdd2uiRv( uiStrings::phrErrDuringRead(fnm) )
}


void HDF5::ReaderImpl::closeFile()
{
    delete dataset_; dataset_ = 0;
    delete group_; group_ = 0;
    doCloseFile( *this );
    grpnms_.setEmpty();
}


void HDF5::ReaderImpl::listObjs( const H5Dir& dir, BufferStringSet& nms,
				 bool wantgroups ) const
{
    const bool islevel0 = nms.isEmpty();
    try
    {
	const int nrobjs = dir.getNumObjs();
	for ( int iobj=0; iobj<nrobjs; iobj++ )
	{
	    const std::string nmstr = dir.getObjnameByIdx( iobj );
	    const BufferString nm( nmstr.c_str() );
	    if ( nm == sGroupInfoDataSetName() )
		continue;

	    const H5O_type_t h5objtyp = dir.childObjType( nm );
	    if ( (wantgroups  && h5objtyp != H5O_TYPE_GROUP)
	      || (!wantgroups && h5objtyp != H5O_TYPE_DATASET) )
		continue;

	    nms.add( wantgroups && islevel0 ? BufferString("/",nm) : nm );

	    if ( wantgroups )
	    {
		H5::Group grp = dir.openGroup( nm );
		BufferStringSet subnms;
		listObjs( grp, subnms, true );
		for ( int idx=0; idx<subnms.size(); idx++ )
		    nms.add( BufferString( nm.str(), "/", subnms.get(idx) ) );
	    }
	}
    }
    mCatchUnexpected( return );
}


void HDF5::ReaderImpl::getGroups( BufferStringSet& nms ) const
{
    nms = grpnms_;
}


bool HDF5::ReaderImpl::selectGroup( const char* grpnm )
{
    if ( !grpnm || !*grpnm )
	grpnm = "/";

    if ( group_ && group_->getObjName() == grpnm )
	return true;

    try
    {
	delete dataset_; dataset_ = 0;
	delete group_; group_ = 0;
	group_ = new H5::Group( file_->openGroup(grpnm) );
    }
    mCatchAnyNoMsg( return false )

    return true;
}


bool HDF5::ReaderImpl::selectDataSet( const char* dsnm )
{
    if ( !group_ )
	{ pErrMsg("check successful selectGroup"); return false; }
    if ( dataset_ && dataset_->getObjName() == dsnm )
	return true;

    try
    {
	delete dataset_; dataset_ = 0;
	dataset_ = new H5::DataSet( group_->openDataSet(dsnm) );
    }
    mCatchAnyNoMsg( return false )

    return true;
}


bool HDF5::ReaderImpl::setScope( const DataSetKey& dsky )
{
    if ( !selectGroup(dsky.groupName()) )
	return false;
    if ( !dsky.hasDataSet() )
	return true;

    return selectDataSet( dsky.dataSetName() );
}


void HDF5::ReaderImpl::getDataSets( const char* grpnm,
				    BufferStringSet& nms ) const
{
    nms.setEmpty();
    if ( !const_cast<ReaderImpl*>(this)->selectGroup(grpnm) )
	return;

    try
    {
	listObjs( *group_, nms, false );
    }
    mCatchUnexpected( return );
}


ArrayNDInfo* HDF5::ReaderImpl::getDataSizes() const
{
    ArrayNDInfo* ret = 0;
    if ( !file_ )
	mRetNoFile( return ret )
    else if ( !haveScope() )
	return ret;

    try
    {
	const H5::DataSpace dataspace = dataset_->getSpace();
	const int nrdims = dataspace.getSimpleExtentNdims();
	if ( nrdims < 1 )
	    { ErrMsg( "HDF5: Empty dataspace found" ); return 0; }

	TypeSet<hsize_t> dims( nrdims, (hsize_t)0 );
	dataspace.getSimpleExtentDims( dims.arr() );

	ret = ArrayNDInfoImpl::create( nrdims );
	for ( int idim=0; idim<nrdims; idim++ )
	    ret->setSize( idim, (int)dims[idim] );
    }
    mCatchUnexpected( return ret );

    return ret;
}


HDF5::ODDataType HDF5::ReaderImpl::getDataType() const
{
    ODDataType ret = OD::F32;
    if ( !file_ )
	mRetNoFile( return ret )
    else if ( !haveScope() )
	return ret;

    try
    {
	const H5::DataType dt = dataset_->getDataType();
	bool issigned = true, isfp = true;
	if ( dt.getClass() == H5T_INTEGER )
	{
	    isfp = false;
	    issigned = dataset_->getIntType().getSign() == H5T_SGN_NONE;
	}
	ret = OD::GetDataRepType( isfp, issigned, dt.getSize() );
    }
    mCatchUnexpected( return ret );

    return ret;
}


void HDF5::ReaderImpl::gtInfo( IOPar& iop, uiRetVal& uirv ) const
{
    iop.setEmpty();
    if ( !file_ )
	mRetNoFileInUiRv()
    else if ( !haveScope(false) )
	mRetNeedScopeInUiRv()

    H5::DataSet groupinfdataset;
    const H5::DataSet* dset = dataset_;
    int nrattrs = 0;
    try
    {
	if ( !dset )
	{
	    groupinfdataset = group_->openDataSet( sGroupInfoDataSetName() );
	    dset = &groupinfdataset;
	}
	nrattrs = dset->getNumAttrs();
    }
    catch ( ... ) {}

    const H5::DataType h5dt = H5::PredType::C_S1;
    for ( int idx=0; idx<nrattrs; idx++ )
    {
	try {
	    const H5::Attribute attr = dset->openAttribute( (unsigned int)idx );
	    const std::string ky = attr.getName();
	    if ( ky.empty() )
		continue;

	    std::string valstr;
	    attr.read( h5dt, valstr );
	    iop.set( ky.c_str(), valstr.c_str() );
	}
	mCatchUnexpected( continue );
    }
}

#define mCatchErrDuringRead() \
        mCatchAdd2uiRv( uiStrings::phrErrDuringRead(fileName()) )

void HDF5::ReaderImpl::gtAll( void* data, uiRetVal& uirv ) const
{
    if ( !file_ )
	mRetNoFileInUiRv()
    else if ( !haveScope() )
	mRetNeedScopeInUiRv()

    // make sure we get data one of our data types
    ODDataType oddt = getDataType();
    const H5::DataType h5dt = h5DataTypeFor( oddt );

    try
    {
	dataset_->read( data, h5dt );
    }
    mCatchErrDuringRead()
}
