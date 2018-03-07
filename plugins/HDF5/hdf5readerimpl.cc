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
	listObjs( *file_, grpnms_, true );
    }
    mCatchAdd2uiRv( uiStrings::phrErrDuringRead(fnm) )
}


void HDF5::ReaderImpl::closeFile()
{
    doCloseFile( *this );
    grpnms_.setEmpty();
}


void HDF5::ReaderImpl::listObjs( const H5Dir& dir, BufferStringSet& nms,
				 bool wantgroups ) const
{
    const bool islevel0 = nms.isEmpty();
    try
    {
	const int nrobjs = dir.getNumObjs();
	for ( int iobj=0; iobj<nrobjs; iobj++ )
	{
	    const std::string nmstr = dir.getObjnameByIdx( iobj );
	    const BufferString nm( nmstr.c_str() );
	    const H5O_type_t h5objtyp = dir.childObjType( nm );
	    if ( wantgroups && h5objtyp != H5O_TYPE_GROUP )
		continue;
	    if ( !wantgroups && h5objtyp != H5O_TYPE_DATASET )
		continue;

	    nms.add( wantgroups && islevel0 ? BufferString("/",nm) : nm );

	    if ( wantgroups )
	    {
		H5::Group grp = dir.openGroup( nm );
		BufferStringSet subnms;
		listObjs( grp, subnms, true );
		for ( int idx=0; idx<subnms.size(); idx++ )
		    nms.add( BufferString( nm.str(), "/", subnms.get(idx) ) );
	    }
	}
    }
    mCatchUnexpected( return );
}


void HDF5::ReaderImpl::getGroups( BufferStringSet& nms ) const
{
    nms = grpnms_;
}


void HDF5::ReaderImpl::getDataSets( const char* grpnm,
				    BufferStringSet& nms ) const
{
    nms.setEmpty();
    H5::Group grp;
    try
    {
	grp = file_->openGroup( grpnm );
    }
    mCatchAnyNoMsg( return )

    try
    {
	listObjs( grp, nms, false );
    }
    mCatchUnexpected( return );
}


ArrayNDInfo* HDF5::ReaderImpl::getDataSizes( const DataSetKey& dsky ) const
{
    return 0;
}
