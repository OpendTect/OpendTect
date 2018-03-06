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
    try {
	file_ = new H5::H5File( fnm, H5F_ACC_TRUNC );
    }
    catch ( H5::Exception exc )
    {
	uirv.add( sHDF5Err().addMoreInfo( toUiString(exc.getCDetailMsg()) ) );
    }
    catch ( ... )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fnm) );
    }
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



#define mRetInternalErr() \
    mRetNoFile( uirv.set( uiStrings::phrInternalErr(e_msg) ); return; )

void HDF5::WriterImpl::ptInfo( const DataSetKey& dsky, const IOPar& info,
			       uiRetVal& uirv )
{
    if ( !file_ )
	mRetInternalErr()

    uirv.add( mTODONotImplPhrase() );
}


void HDF5::WriterImpl::ptData( const DataSetKey& dsky, const ArrayNDInfo& info,
			       const Byte* data, ODDataType dt, uiRetVal& uirv )
{
    if ( !file_ )
	mRetInternalErr()

    const int nrdims = info.getNDim();
    TypeSet<hsize_t> dims, chunkdims;
    for ( int idim=0; idim<nrdims; idim++ )
    {
	dims += info.getSize( idim );
	chunkdims += chunksz_;
    }

    try {
	H5::DataSpace dataspace( nrdims, dims.arr() );

	H5::DSetCreatPropList proplist;
	if ( nrdims > 1 )
	    proplist.setChunk( nrdims, chunkdims.arr() );
	unsigned szip_options_mask = H5_SZIP_EC_OPTION_MASK;
	unsigned szip_pixels_per_block = 16;
	proplist.setSzip( szip_options_mask, szip_pixels_per_block );
	const H5DataType h5dt = h5DataTypeFor( dt );
	H5::DataSet dataset( file_->createDataSet( dsky.fullName(), h5dt,
					      dataspace, proplist ) );
	dataset.write( data, h5dt );
    }
    catch ( H5::Exception exc )
    {
	uirv.add( sHDF5Err().addMoreInfo( toUiString(exc.getCDetailMsg()) ) );
    }
    catch ( ... )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fileName()) );
    }
}
