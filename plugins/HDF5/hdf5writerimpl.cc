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


bool HDF5::WriterImpl::ensureGroup( const char* grpnm )
{
    if ( group_ && group_->getObjName() == grpnm )
	return true;

    try {
	delete dataset_; dataset_ = 0;
	delete group_; group_ = 0;
	group_ = new H5::Group( file_->createGroup(grpnm) );
    }
    mCatchAnyNoMsg( return false )

    return true;
}


static void getWriteStr( const char* inpstr, int nrchars, BufferString& ret )
{
    ret.set( inpstr );
    int termpos = ret.size();
    if ( termpos > nrchars-2 )
	termpos = nrchars - 1;
    else
	ret.addSpace( nrchars - termpos );
    *(ret.getCStr()+termpos) = '\0';
}



void HDF5::WriterImpl::ptInfo( const DataSetKey& reqdsky, const IOPar& iop,
			       uiRetVal& uirv )
{
    const hsize_t nrvals = iop.size();
    if ( nrvals < 1 )
	return;

    DataSetKey dsky( reqdsky );
    const H5::DataType h5dt = H5::PredType::C_S1;
    H5::DataSet dataset;
    bool notpresent = false;
    const BufferString fulldsname( dsky.fullDataSetName() );
    try
    {
	dataset = file_->openDataSet( fulldsname );
    }
    mCatchAnyNoMsg( notpresent = true )

    hsize_t dims[1];
    if ( notpresent )
    {
	if ( reqdsky.hasDataSet() )
	{
	    uirv.add( uiStrings::phrInternalErr(
			"Use createDataSet first, then putInfo") );
	    return;
	}
	dims[0] = 1; // 0 needed but seems like a bad idea
	try
	{
	    dataset = file_->createDataSet( fulldsname, h5dt,
					    H5::DataSpace(1,dims) );
	    float data = 0.f;
	    dataset.write( &data, h5dt );
	}
	mCatchErrDuringWrite()
    }

    const int nrchars = iop.maxContentSize( false ) + 1;
    dims[0] = nrchars;
    try
    {
	H5::DataSpace dataspace( 1, dims );
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    H5::Attribute attribute = dataset.createAttribute(
				iop.getKey(idx), h5dt, dataspace );
	    BufferString str2write;
	    getWriteStr( iop.getValue(idx), nrchars, str2write );
	    attribute.write( H5::PredType::C_S1, str2write.getCStr() );
	}
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::crDS( const DataSetKey& dsky, const ArrayNDInfo& info,
			       ODDataType dt, uiRetVal& uirv )
{
    if ( !ensureGroup(dsky.groupName()) )
    {
	uirv.add( sHDF5Err(tr("Cannot create group '%1'")
		    .arg( dsky.groupName() ) ) );
	return;
    }

    const int nrdims = info.nrDims();
    TypeSet<hsize_t> dims, chunkdims;
    for ( int idim=0; idim<nrdims; idim++ )
    {
	dims += info.getSize( idim );
	chunkdims += chunksz_;
    }

    const H5DataType h5dt = h5DataTypeFor( dt );
    try
    {
	H5::DataSpace dataspace( nrdims, dims.arr() );
	H5::DSetCreatPropList proplist;
	if ( nrdims > 1 )
	    proplist.setChunk( nrdims, chunkdims.arr() );
	proplist.setSzip( szip_options_mask, szip_pixels_per_block );
	delete dataset_; dataset_ = 0;
	dataset_ = new H5::DataSet( group_->createDataSet( dsky.dataSetName(),
				    h5dt, dataspace, proplist ) );
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptAll( const void* data, uiRetVal& uirv )
{
    if ( !dataset_ )
	mRetNeedScopeInUiRv()

    try
    {
	dataset_->write( data, dataset_->getSpace().getSimpleExtentType() );
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::ptSlab( const SlabSpec& spec,
			       const void* data, uiRetVal& uirv )
{
    if ( !dataset_ )
	mRetNeedScopeInUiRv()

    mPutInternalInUiRv( uirv, "TODO: implement", return )
}
