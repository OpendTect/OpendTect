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


H5::DataType HDF5::WriterImpl::h5DataTypeFor( ODDataType datarep )
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


bool HDF5::WriterImpl::ensureGroup( const char* nm )
{
    try { file_->createGroup( nm ); }
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


void HDF5::WriterImpl::ptInfo( const DataSetKey& dsky, const IOPar& iop,
			       uiRetVal& uirv )
{
    if ( !file_ )
	mRetNoFileInUiRv()
    const hsize_t nrvals = iop.size();
    if ( nrvals < 1 )
	return;

    H5::DataSet dataset;
    bool notpresent = false;
    try
    {
	dataset = file_->openDataSet( dsky.fullDataSetName() );
    }
    mCatchAnyNoMsg( notpresent = true )
    if ( notpresent )
    {
	uirv.add( uiStrings::phrInternalErr("Write data before properties") );
	return;
    }

    const H5::DataType h5dt = H5::PredType::C_S1;
    const int nrchars = iop.maxContentSize( false ) + 1;
    hsize_t dims[1]; dims[0] = nrchars;
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
    mCatchAdd2uiRv( uiStrings::phrErrDuringWrite(fileName()) )
}


void HDF5::WriterImpl::ptData( const DataSetKey& dsky, const ArrayNDInfo& info,
			       const void* data, ODDataType dt, uiRetVal& uirv )
{
    if ( !file_ )
	mRetNoFileInUiRv()

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
	ensureGroup( dsky.groupName() );
	H5::DataSet dataset( file_->createDataSet( dsky.fullDataSetName(), h5dt,
					      dataspace, proplist ) );
	dataset.write( data, h5dt );
    }
    mCatchAdd2uiRv( uiStrings::phrErrDuringWrite(fileName()) )
}
