/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5writerimpl.h"
#include "uistrings.h"
#include "H5Cpp.h"


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


void HDF5::WriterImpl::setDims( const ArrayNDInfo& arrinf )
{
    //TODO
}


int HDF5::WriterImpl::chunkSize() const
{
    //TODO
    return 64;
}


void HDF5::WriterImpl::setChunkSize( int sz )
{
    //TODO
}
