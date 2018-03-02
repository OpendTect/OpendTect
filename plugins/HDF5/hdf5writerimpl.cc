/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5writerimpl.h"
#include "uistrings.h"


HDF5::WriterImpl::WriterImpl()
    : AccessImpl(*this)
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
    catch ( H5::FileIException error )
    {
	uirv.add( sHDF5Err().addMoreInfo( toUiString(error.getCDetailMsg()) ) );
    }
    catch ( ... )
    {
	uirv.add( uiStrings::phrErrDuringWrite(fnm) );
    }
}



int HDF5::WriterImpl::chunkSize() const
{
    if ( !file_ )
	mRetNoFile(return -1)
    //TODO
    return 64;
}


void HDF5::WriterImpl::setChunkSize( int sz )
{
    if ( !file_ )
	mRetNoFile(return)
    //TODO
}


void HDF5::WriterImpl::setDataType( OD::FPDataRepType datarep )
{
    if ( !file_ )
	mRetNoFile(return)

    H5::DataType dt;
#   define mHandleCase(od,hdf) \
	case OD::od:	    dt = H5::PredType::hdf; break;

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
	default:
	mHandleCase( F32, IEEE_F32LE );
    }

    //TODO
}


uiRetVal HDF5::WriterImpl::putInfo( const GroupPath& path, const IOPar& info )
{
    uiRetVal uirv;
    uirv.add( mTODONotImplPhrase() );
    return uirv;
}


uiRetVal HDF5::WriterImpl::putData( const GroupPath& path,
		const ArrayND<float>& data, const IOPar* info )
{
    uiRetVal uirv;
    uirv.add( mTODONotImplPhrase() );
    return uirv;
}
