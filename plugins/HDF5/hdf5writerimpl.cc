/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5writerimpl.h"
#include "uistrings.h"
#include "arrayndimpl.h"
#include "iopar.h"

unsigned szip_options_mask = H5_SZIP_EC_OPTION_MASK; // entropy coding
		     // nearest neighbour coding: H5_SZIP_NN_OPTION_MASK
unsigned szip_pixels_per_block = 32;
		    // can be an even number [2,32]


#define mCatchErrDuringWrite() \
    mCatchAdd2uiRv( uiStrings::phrErrDuringWrite(fileName()) )


HDF5::WriterImpl::WriterImpl()
    : AccessImpl(*this)
    , chunksz_(64)
{
}


HDF5::WriterImpl::~WriterImpl()
{
    closeFile();
}


void HDF5::WriterImpl::openFile( const char* fnm, uiRetVal& uirv )
{
    try
    {
	file_ = new H5::H5File( fnm, H5F_ACC_TRUNC );
    }
    mCatchAdd2uiRv( uiStrings::phrCannotOpen(fnm) )
}


void HDF5::WriterImpl::setChunkSize( int sz )
{
    chunksz_ = sz;
}


bool HDF5::WriterImpl::ensureGroup( const char* grpnm, uiRetVal& uirv )
{
    if ( !selectGroup(grpnm) )
    {
	try {
	    group_ = file_->createGroup( grpnm );
	}
	mCatchAdd2uiRv( tr("Cannot create Group '%1'").arg(grpnm) )
    }

    return uirv.isOK();
}


void HDF5::WriterImpl::crDS( const DataSetKey& dsky, const ArrayNDInfo& info,
			       ODDataType dt, uiRetVal& uirv )
{
    if ( !ensureGroup(dsky.groupName(),uirv) )
	return;

    nrdims_ = info.nrDims();
    TypeSet<hsize_t> dims, chunkdims;
    bool needchunk = false;
    for ( int idim=0; idim<nrdims_; idim++ )
    {
	const auto dimsz = info.getSize( idim );
	dims += dimsz;
	if ( dimsz > chunksz_ )
	    needchunk = true;
	chunkdims += chunksz_ < dimsz ? chunksz_ : dimsz;
    }

    const H5DataType h5dt = h5DataTypeFor( dt );
    try
    {
	H5::DataSpace dataspace( nrdims_, dims.arr() );
	H5::DSetCreatPropList proplist;
	if ( nrdims_ > 1 && needchunk )
	    proplist.setChunk( nrdims_, chunkdims.arr() );
	proplist.setSzip( szip_options_mask, szip_pixels_per_block );
	dataset_ = group_.createDataSet( dsky.dataSetName(),
					 h5dt, dataspace, proplist );
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptInfo( const IOPar& iop, uiRetVal& uirv,
			       const DataSetKey* ptrdsky )
{
    if ( iop.isEmpty() )
	return;

    DataSetKey dsky;
    if ( ptrdsky )
	dsky = *ptrdsky;
    else
    {
	if ( haveScope() )
	{
	    putAttrib( dataset_, iop, uirv );
	    return;
	}
	// No datasetkey, nothing open yet: work with file level
    }

    if ( !ensureGroup(dsky.groupName(),uirv) )
	return;

    H5::DataSet* useds = &dataset_;
    H5::DataSet ds;
    if ( !atDataSet(dsky.dataSetName()) )
    {
	const bool dsempty = dsky.dataSetEmpty();
	bool notpresent = dsempty;
	if ( !dsempty )
	{
	    try
	    {
		ds = group_.openDataSet( dsky.dataSetName() );
	    }
	    mCatchAnyNoMsg( notpresent = true )
	}

	if ( notpresent )
	{
	    if ( !dsempty )
	    {
		uirv.add( uiStrings::phrInternalErr(
			    "Use createDataSet first, then putInfo") );
		return;
	    }
	    hsize_t dims[1]; dims[0] = 1;
	    const char data = '\0';
	    const H5DataType h5dt = H5::PredType::C_S1;
	    try
	    {
		ds = group_.createDataSet( DataSetKey::sGroupInfoDataSetName(),
					    h5dt, H5::DataSpace(1,dims) );
		ds.write( &data, h5dt );
	    }
	    mCatchErrDuringWrite()
	}
	useds = &ds;
    }

    putAttrib( *useds, iop, uirv );
}


static BufferString getWriteStr( const char* inpstr, int nrchars )
{
    BufferString ret( inpstr );
    int termpos = ret.size();
    if ( termpos > nrchars-2 )
	termpos = nrchars - 1;
    else
	ret.addSpace( nrchars - termpos );
    *(ret.getCStr()+termpos) = '\0';
    return ret;
}


void HDF5::WriterImpl::putAttrib( H5::DataSet& h5ds, const IOPar& iop,
				  uiRetVal& uirv )
{
    const H5DataType h5dt = H5::PredType::C_S1;
    const int nrchars = iop.maxContentSize( false ) + 1;
    hsize_t dims[1]; dims[0] = nrchars;
    try
    {
	H5::DataSpace dataspace( 1, dims );
	for ( int idx=0; idx<iop.size(); idx++ )
	{
	    H5::Attribute attribute = h5ds.createAttribute(
				iop.getKey(idx), h5dt, dataspace );
	    BufferString str2write = getWriteStr( iop.getValue(idx), nrchars );
	    attribute.write( h5dt, str2write.getCStr() );
	}
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptStrings( const DataSetKey& dsky,
				  const BufferStringSet& bss, uiRetVal& uirv )
{
    if ( !ensureGroup(dsky.groupName(),uirv) )
	return;

    const hsize_t nrchars = bss.maxLength() + 1;
    hsize_t dims[2] = { (hsize_t)bss.size(), nrchars };
    mDeclareAndTryAlloc( char*, buf, char [ dims[0] * dims[1] ] );
    if ( !buf )
	{ uirv.set( uiStrings::phrCannotAllocateMemory() ); return; }
    ArrPtrMan<char> deleter = buf;

    for ( int idx=0; idx<bss.size(); idx++ )
    {
	const BufferString& inpstr = bss.get( idx );
	const BufferString paddedstr = getWriteStr( inpstr, nrchars );
	OD::sysMemCopy( buf + idx*nrchars, paddedstr.str(), nrchars );
    }

    const H5DataType h5dt = H5::PredType::C_S1;
    try
    {
	dataset_ = group_.createDataSet( dsky.dataSetName(),
				    h5dt, H5::DataSpace(2,dims) );
	dataset_.write( buf, h5dt );
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptAll( const void* data, uiRetVal& uirv )
{
    if ( !haveScope() )
	mRetNeedScopeInUiRv()

    try
    {
	H5::DataSpace dataspace = dataset_.getSpace();
	dataspace.selectAll();
	dataset_.write( data, dataset_.getDataType() );
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptSlab( const SlabSpec& spec,
			       const void* data, uiRetVal& uirv )
{
    if ( !haveScope() )
	mRetNeedScopeInUiRv()

    TypeSet<hsize_t> counts;
    try
    {
	H5::DataSpace filedataspace = dataset_.getSpace();
	selectSlab( filedataspace, spec, &counts );
	H5::DataSpace memdataspace( (NrDimsType)spec.size(), counts.arr() );
	dataset_.write( data, dataset_.getDataType(), memdataspace,
			filedataspace );
    }
    mCatchErrDuringWrite()
}
