/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5readerimpl.h"
#include "uistrings.h"
#include "file.h"


HDF5::ReaderImpl::ReaderImpl()
    : file_(0)
{
}


HDF5::ReaderImpl::~ReaderImpl()
{
    delete file_;
}


uiRetVal HDF5::ReaderImpl::open( const char* fnm )
{
    uiRetVal uirv;
    if ( !File::exists(fnm) )
    {
	uirv.add( uiStrings::phrCannotOpen( fnm ) );
	return uirv;
    }

    try {
	file_ = new H5::H5File( fnm, H5F_ACC_RDONLY );
    }
    catch ( H5::FileIException error )
    {
	uirv.add( sHDF5Err().addMoreInfo( toUiString(error.getCDetailMsg()) ) );
    }
    catch ( ... )
    {
	uirv.add( uiStrings::phrErrDuringRead( fnm ) );
    }

    return uirv;
}


void HDF5::ReaderImpl::getGroups( const GroupPath& pth,
				  BufferStringSet& nms ) const
{
    //TODO impl
}


HDF5::GroupID HDF5::ReaderImpl::groupIDFor( const GroupPath& pth ) const
{
    //TODO impl
    return -1;
}
