/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5readerimpl.h"
#include "hdf5writerimpl.h"
#include "uistrings.h"
#include "file.h"
#include "H5Cpp.h"


HDF5::Reader* HDF5::AccessProviderImpl::getReader() const
{
    return new HDF5::ReaderImpl;
}


HDF5::Writer* HDF5::AccessProviderImpl::getWriter() const
{
    return new HDF5::WriterImpl;
}


HDF5::AccessImpl::AccessImpl( ReaderImpl& rdr )
    : acc_(rdr)
{
}


HDF5::AccessImpl::AccessImpl( WriterImpl& wrr )
    : acc_(wrr)
{
}


HDF5::AccessImpl::~AccessImpl()
{
}


const char* HDF5::AccessImpl::gtFileName() const
{
    if ( !acc_.file_ )
	return 0;

    mDeclStaticString(ret);
    ret.set( acc_.file_->getFileName().c_str() );
    return ret.str();
}


void HDF5::AccessImpl::doCloseFile( Access& acc )
{
    // cannot use non-static fn, acc_ may have been deleted
    if ( acc.file_ )
    {
	H5Fclose( acc.file_->getId() );
	delete acc.file_;
	acc.file_ = 0;
    }
}
