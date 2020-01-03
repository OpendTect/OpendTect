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
#include "odjson.h"

#define mCatchErrDuringRead() \
    mCatchAdd2uiRv( uiStrings::phrErrDuringRead(fileName()) )


HDF5::ReaderImpl::ReaderImpl()
    : AccessImpl(*this)
{
}


HDF5::ReaderImpl::ReaderImpl( const H5::H5File& h5file )
    : AccessImpl(*this)
{
    myfile_ = false;
    file_ = const_cast<H5::H5File*>( &h5file );
}


HDF5::ReaderImpl::~ReaderImpl()
{
    closeFile();
}


void HDF5::ReaderImpl::openFile( const char* fnm, uiRetVal& uirv, bool )
{
    if ( !File::exists(fnm) )
	{ uirv.add( uiStrings::phrCannotOpenForRead( fnm ) ); return; }

    try
    {
	H5::H5File* newfile = new H5::H5File( fnm, H5F_ACC_RDONLY );
	closeFile();
	myfile_ = true;
	file_ = newfile;
    }
    mCatchAdd2uiRv( uiStrings::phrErrDuringRead(fnm) )
}


template <class H5Dir>
void HDF5::ReaderImpl::listObjs( const H5Dir& dir, BufferStringSet& nms,
				 bool wantgroups ) const
{
    const bool islevel0 = nms.isEmpty();
    try
    {
	const int nrobjs = mCast(int,dir.getNumObjs());
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
    nms.setEmpty();
    if ( file_ )
	listObjs( *file_, nms, true );
}


void HDF5::ReaderImpl::getDataSets( const char* grpnm,
				    BufferStringSet& nms ) const
{
    nms.setEmpty();
    if ( !const_cast<ReaderImpl*>(this)->selectGroup(grpnm) )
	return;

    listObjs( group_, nms, false );
}


ArrayNDInfo* HDF5::ReaderImpl::gtDataSizes() const
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
	for ( dim_idx_type idim=0; idim<nrdims_; idim++ )
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


bool HDF5::ReaderImpl::hasAttribute( const char* attrnm ) const
{
    if ( !file_ )
	return false;

    try
    {
	return file_->attrExists( attrnm );
    }
    catch ( ... )
	{ return false; }
}


uiRetVal HDF5::ReaderImpl::readJSonAttribute( const char* attrnm,
					  OD::JSON::ValueSet* vs ) const
{
    uiRetVal uirv;
    if ( !attrnm )
    {
	uirv.set( tr("Valid attribute name required") );
	return uirv;
    }

    if ( !vs )
    {
	uirv.set( tr("Valid OD::JSON::ValueSet* required") );
	return uirv;
    }
    if ( !hasAttribute( attrnm ) )
    {
	uirv.set( tr("No attribute named: %1").arg(attrnm) );
	return uirv;
    }

    vs->setEmpty();

    const H5::Attribute attr = file_->openAttribute( attrnm );
    std::string valstr;
    attr.read( attr.getDataType(), valstr );
    uirv = vs->parseJSon( const_cast<char*>( valstr.c_str() ),
							    valstr.size() );
    return uirv;
}


BufferString HDF5::ReaderImpl::readAttribute( const char* attrnm ) const
{
    BufferString res;
    if ( hasAttribute( attrnm ) )
    {
	const H5::Attribute attr = file_->openAttribute( attrnm );
	std::string valstr;
	attr.read( attr.getDataType(), valstr );
	res.set( valstr.c_str() );
    }

    return res;
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

    for ( int idx=0; idx<nrattrs; idx++ )
    {
	try {
	    const H5::Attribute attr = dset->openAttribute( (unsigned int)idx );
	    const std::string ky = attr.getName();
	    if ( ky.empty() )
		continue;

	    std::string valstr;
	    attr.read( attr.getDataType(), valstr );
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

    try
    {
	H5::DataSpace dataspace = dataset_.getSpace();
	mGetDataSpaceDims( dims, nrdims_, dataspace );
	const hsize_t nrstrs = dims[0];
	char** strs = new char* [ nrstrs ];
	ArrPtrMan<char*> deleter = strs;

	dataspace.selectAll();
	dataset_.read( strs, dataset_.getDataType() );

	for ( int istr=0; istr<nrstrs; istr++ )
	    bss.add( strs[istr] );
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
	for ( NDPosBufSet::idx_type ipt=0; ipt<nrpts; ipt++ )
	{
	    const NDPosBuf& posbuf = posbufs[ipt];
	    const int arroffs = ipt * nrdims_;
	    for ( dim_idx_type idim=0; idim<nrdims_; idim++ )
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
