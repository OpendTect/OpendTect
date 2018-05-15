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

#define mCatchErrDuringRead() \
    mCatchAdd2uiRv( uiStrings::phrErrDuringRead(fileName()) )


HDF5::ReaderImpl::ReaderImpl()
    : AccessImpl(*this)
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
    doCloseFile( *this );
    grpnms_.setEmpty();
}


template <class H5Dir>
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
	    if ( nm == DataSetKey::sGroupInfoDataSetName() )
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


void HDF5::ReaderImpl::getDataSets( const char* grpnm,
				    BufferStringSet& nms ) const
{
    nms.setEmpty();
    if ( !const_cast<ReaderImpl*>(this)->selectGroup(grpnm) )
	return;

    try
    {
	listObjs( group_, nms, false );
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
    else if ( nrdims_ < 1 )
	{ ErrMsg( "HDF5: Empty dataspace found" ); return 0; }

    try
    {
	const H5::DataSpace dataspace = dataset_.getSpace();
	mGetDataSpaceDims( dims, nrdims_, dataspace );

	ret = ArrayNDInfoImpl::create( nrdims_ );
	for ( DimIdxType idim=0; idim<nrdims_; idim++ )
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
	const H5::DataType& dt = dataset_.getDataType();
	bool issigned = true, isfp = true;
	if ( dt.getClass() == H5T_INTEGER )
	{
	    isfp = false;
	    issigned = dataset_.getIntType().getSign() != H5T_SGN_NONE;
	}
	ret = OD::GetDataRepType( isfp, issigned, dt.getSize() );
    }
    mCatchUnexpected( return ret );

    return ret;
}


void HDF5::ReaderImpl::gtInfo( IOPar& iop, uiRetVal& uirv ) const
{
    iop.setEmpty();
    if ( !haveScope(false) )
	mRetNeedScopeInUiRv()

    H5::DataSet groupinfdataset;
    const H5::DataSet* dset = &dataset_;
    int nrattrs = 0;
    try
    {
	if ( !validH5Obj(*dset) )
	{
	    groupinfdataset = group_.openDataSet(
					DataSetKey::sGroupInfoDataSetName() );
	    dset = &groupinfdataset;
	}
	nrattrs = dset->getNumAttrs();
    }
    catch ( ... )
	{ return; }

    const H5DataType h5dt = H5::PredType::C_S1;
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


const HDF5::ReaderImpl::H5DataType& HDF5::ReaderImpl::h5DataType() const
{
    // makes sure we get data one of our data types
    return h5DataTypeFor( getDataType() );
}


void HDF5::ReaderImpl::gtAll( void* data, uiRetVal& uirv ) const
{
    if ( !haveScope() )
	mRetNeedScopeInUiRv()

    try
    {
	dataset_.getSpace().selectAll();
	dataset_.read( data, h5DataType() );
    }
    mCatchErrDuringRead()
}


void HDF5::ReaderImpl::gtStrings( BufferStringSet& bss, uiRetVal& uirv ) const
{
    if ( !haveScope() )
	mRetNeedScopeInUiRv()

    const H5DataType h5dt = H5::PredType::C_S1;
    try
    {
	H5::DataSpace dataspace = dataset_.getSpace();
	mGetDataSpaceDims( dims, nrdims_, dataspace );

	const hsize_t nrstrs = dims[0];
	const hsize_t nrchars = dims[1];
	mDeclareAndTryAlloc( char*, buf, char [ nrstrs*nrchars ] );
	if ( !buf )
	    { uirv.set( uiStrings::phrCannotAllocateMemory() ); return; }
	ArrPtrMan<char> deleter = buf;

	dataspace.selectAll();
	dataset_.read( buf, h5dt );

	for ( int istr=0; istr<nrstrs; istr++ )
	{
	    bss.add( buf );
	    buf += nrchars;
	}
    }
    mCatchErrDuringRead()
}


void HDF5::ReaderImpl::gtPoints( const NDPosBufSet& posbufs, void* data,
				 uiRetVal& uirv ) const
{
    if ( !haveScope() )
	mRetNeedScopeInUiRv()

    try
    {
	H5::DataSpace inputdataspace = dataset_.getSpace();
	const hsize_t nrpts = (hsize_t)posbufs.size();

	mAllocVarLenArr( hsize_t, hdfcoordarr, nrdims_ * nrpts );
	if ( !mIsVarLenArrOK(hdfcoordarr) )
	    { uirv.add( uiStrings::phrCannotAllocateMemory() ); return; }
	for ( hsize_t ipt=0; ipt<nrpts; ipt++ )
	{
	    const NDPosBuf& posbuf = posbufs[ipt];
	    const int arroffs = ipt * nrdims_;
	    for ( DimIdxType idim=0; idim<nrdims_; idim++ )
		hdfcoordarr[arroffs + idim] = posbuf[idim];
	}
	inputdataspace.selectElements( H5S_SELECT_SET, nrpts, hdfcoordarr );

	H5::DataSpace outputdataspace( 1, &nrpts );
	dataset_.read( data, h5DataType(), outputdataspace, inputdataspace );
    }
    mCatchErrDuringRead()
}


void HDF5::ReaderImpl::gtSlab( const SlabSpec& spec, void* data,
			       uiRetVal& uirv ) const
{
    if ( !haveScope() )
	mRetNeedScopeInUiRv()

    TypeSet<hsize_t> counts;
    try
    {
	H5::DataSpace filedataspace = dataset_.getSpace();
	filedataspace.selectAll();
	selectSlab( filedataspace, spec, &counts );
	H5::DataSpace memdataspace( nrdims_, counts.arr() );
	const H5DataType& h5dt = h5DataType();
	dataset_.read( data, h5dt, memdataspace, filedataspace );
    }
    mCatchErrDuringRead()
}
