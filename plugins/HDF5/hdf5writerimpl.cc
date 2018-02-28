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
    : file_(0)
{
}


HDF5::WriterImpl::~WriterImpl()
{
    delete file_;
}


uiRetVal HDF5::WriterImpl::open( const char* fnm )
{
    uiRetVal uirv;

    try {
	file_ = new H5::H5File( fnm, H5F_ACC_TRUNC );
    }
    catch ( H5::FileIException error )
    {
	uirv.add( sHDF5Err().addMoreInfo( toUiString(error.getCDetailMsg()) ) );
    }
    catch ( ... )
    {
	uirv.add( uiStrings::phrErrDuringWrite( fnm ) );
    }

    return uirv;
}
