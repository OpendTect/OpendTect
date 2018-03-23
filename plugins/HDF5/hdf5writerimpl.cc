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
    if ( atGroup(grpnm) )
	return true;

    bool waspresent = true;
    try {
	delete dataset_; dataset_ = 0;
	delete group_; group_ = 0;
	group_ = new H5::Group( file_->openGroup(grpnm) );
    }
    mCatchAnyNoMsg( waspresent = false )

    if ( !waspresent || !group_ )
    {
	try {
	    group_ = new H5::Group( file_->createGroup(grpnm) );
	}
	mCatchAdd2uiRv( tr("Cannot create Group '%1'").arg(grpnm) )
    }

    return uirv.isOK();
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



void HDF5::WriterImpl::ptInfo( const DataSetKey& dsky, const IOPar& iop,
			       uiRetVal& uirv )
{
    const hsize_t nrvals = iop.size();
    if ( nrvals < 1 || !ensureGroup(dsky.groupName(),uirv) )
	return;

    const H5::DataType h5dt = H5::PredType::C_S1;
    H5::DataSet* useds = dataset_;
    H5::DataSet ds;
    hsize_t dims[1];
    if ( !atDataSet(dsky.dataSetName()) )
    {
	bool notpresent = false;
	try
	{
	    ds = group_->openDataSet( dsky.dataSetName() );
	}
	mCatchAnyNoMsg( notpresent = true )

	if ( notpresent )
	{
	    if ( !dsky.dataSetEmpty() )
	    {
		uirv.add( uiStrings::phrInternalErr(
			    "Use createDataSet first, then putInfo") );
		return;
	    }
	    dims[0] = 1; // 0 needed but seems like a bad idea
	    const char data = '\0';
	    try
	    {
		ds = group_->createDataSet( DataSetKey::sGroupInfoDataSetName(),
					    h5dt, H5::DataSpace(1,dims) );
		ds.write( &data, h5dt );
	    }
	    mCatchErrDuringWrite()
	}
	useds = &ds;
    }

    const int nrchars = iop.maxContentSize( false ) + 1;
    dims[0] = nrchars;
    try
    {
	H5::DataSpace dataspace( 1, dims );
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    H5::Attribute attribute = useds->createAttribute(
				iop.getKey(idx), h5dt, dataspace );
	    BufferString str2write;
	    getWriteStr( iop.getValue(idx), nrchars, str2write );
	    attribute.write( h5dt, str2write.getCStr() );
	}
    }
    mCatchErrDuringWrite()
}


void HDF5::WriterImpl::crDS( const DataSetKey& dsky, const ArrayNDInfo& info,
			       ODDataType dt, uiRetVal& uirv )
{
    if ( !ensureGroup(dsky.groupName(),uirv) )
	return;

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
	dataset_->write( data, dataset_->getDataType() );
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
