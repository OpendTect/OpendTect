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
    : AccessImpl(*this)
{
}


HDF5::ReaderImpl::~ReaderImpl()
{
    closeFile();
}


void HDF5::ReaderImpl::openFile( const char* fnm, uiRetVal& uirv )
{
    if ( !File::exists(fnm) )
	{ uirv.add( uiStrings::phrCannotOpen( fnm ) ); return; }

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
}


void HDF5::ReaderImpl::getGroups( const GroupPath& pth,
				  BufferStringSet& nms ) const
{
    //TODO
}


ArrayNDInfo* HDF5::ReaderImpl::getDataSizes( const GroupPath& path,
					     uiRetVal& uirv ) const
{
    uirv.add( mTODONotImplPhrase() );
    return 0;
}
