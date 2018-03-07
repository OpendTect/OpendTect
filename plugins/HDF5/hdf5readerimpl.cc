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

    try
    {
	grpnms_.setEmpty();
	file_ = new H5::H5File( fnm, H5F_ACC_RDONLY );
	listDirs( *file_, grpnms_ );
    }
    catch ( H5::Exception& exc )
    {
	uirv.add( sHDF5Err().addMoreInfo( toUiString(exc.getCDetailMsg()) ) );
    }
    mCatchNonHDF(
	uirv.add( uiStrings::phrErrDuringRead(fnm)
		 .addMoreInfo(toUiString(exc_msg)) ) )
}


void HDF5::ReaderImpl::closeFile()
{
    doCloseFile( *this );
    grpnms_.setEmpty();
}


void HDF5::ReaderImpl::listDirs( const H5Dir& dir, BufferStringSet& nms ) const
{
    try
    {
	const int nrgrps = dir.getNumObjs();
	for ( int idir=0; idir<nrgrps; idir++ )
	{
	    std::string nm = dir.getObjnameByIdx( idir );
	    nms.add( nm.c_str() );

	    H5::Group grp = dir.openGroup( nm.c_str() );
	    BufferStringSet subnms;
	    listDirs( grp, subnms );
	    for ( int idx=0; idx<subnms.size(); idx++ )
		nms.add( BufferString( nm.c_str(), "/", subnms.get(idx) ) );
	}
    }
    mCatchUnexpected( return );
}


void HDF5::ReaderImpl::getGroups( BufferStringSet& nms ) const
{
    nms = grpnms_;
}


void HDF5::ReaderImpl::getDataSets( const char* grp,
				    BufferStringSet& nms ) const
{
}


ArrayNDInfo* HDF5::ReaderImpl::getDataSizes( const DataSetKey& dsky ) const
{
    return 0;
}
