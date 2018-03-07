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


void HDF5::AccessProviderImpl::initHDF5()
{
    initClass();
    H5::Exception::dontPrint();
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
    // cannot use acc_ here, it may have been deleted
    if ( acc.file_ )
    {
	try
	{
	    H5Fclose( acc.file_->getId() );
	    delete acc.file_;
	}
	mCatchUnexpected( (void)0 )
	acc.file_ = 0;
    }
}
