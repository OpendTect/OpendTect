/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Salil Agarwal
Date:          27 August 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "ziparchiveinfo.h"

#include "bufstringset.h"
#include "ziphandler.h"

#include <iostream>


ZipArchiveInfo::ZipArchiveInfo( const char* fnm )
    : ziphd_(*new ZipHandler)
{
    isok_ = readZipArchive( fnm );
}


ZipArchiveInfo::~ZipArchiveInfo()
{
    deepErase( fileinfo_ );
    delete &ziphd_;
}


bool ZipArchiveInfo::readZipArchive( const char* srcfnm )
{
    if ( ziphd_.getArchiveInfo(srcfnm,fileinfo_) )
	return true;

    return false;
}


bool ZipArchiveInfo::getAllFnms( BufferStringSet& fnms ) const
{
    if ( !isok_ )
	return false;

    for( od_int32 idx=0; idx<fileinfo_.size(); idx++ )
	fnms.add( fileinfo_[idx]->fnm_ );

    return true;
}


od_int64 ZipArchiveInfo::getFileCompSize( const char* fnm ) const
{
    if ( !isok_ )
	return -1;

    BufferString filenm = fnm;
    for( od_int32 idx=0; idx<fileinfo_.size(); idx++ )
	if ( filenm.matches( fileinfo_[idx]->fnm_ ) )
	    return fileinfo_[idx]->compsize_;

    errormsg_ = fnm;
    errormsg_ += ": File not found";
    return -1;
}


od_int64 ZipArchiveInfo::getFileCompSize( od_int32 idx ) const
{
    if ( !isok_ )
	return -1;

    if ( fileinfo_.validIdx(idx) )
    {
	errormsg_ = "Index is out of range";
	return -1;
    }

    return fileinfo_[idx]->compsize_;
}
od_int64 ZipArchiveInfo::getFileUnCompSize( const char* fnm ) const
{
    if ( !isok_ )
	return -1;

    BufferString filenm = fnm;
    for( od_int32 idx=0; idx<fileinfo_.size(); idx++ )
	if ( filenm.matches( fileinfo_[idx]->fnm_ ) )
	    return fileinfo_[idx]->uncompsize_;

    errormsg_ = fnm;
    errormsg_ += ": File not found";
    return -1;
}

od_int64 ZipArchiveInfo::getFileUnCompSize( od_int32 idx ) const
{
    if ( !isok_ )
	return -1;

    if ( fileinfo_.validIdx(idx) )
    {
	errormsg_ = "Index is out of range";
	return -1;
    }

    return fileinfo_[idx]->uncompsize_;
}


od_int64 ZipArchiveInfo::getLocalHeaderOffset( const char* fnm ) const
{
    if ( !isok_ )
	return -1;

    BufferString filenm = fnm;
    for( od_int32 idx=0; idx<fileinfo_.size(); idx++ )
    {
	if ( filenm.matches( fileinfo_[idx]->fnm_ ) )
	    return fileinfo_[idx]->localheaderoffset_;
    }

    errormsg_ = fnm;
    errormsg_ += ": File not found";
    return -1;
}


od_int64 ZipArchiveInfo::getLocalHeaderOffset( od_int32 idx ) const
{
    if ( !isok_ )
	return -1;

    if ( fileinfo_.validIdx(idx) )
    {
	errormsg_ = "Index is out of range";
	return -1;
    }

    return fileinfo_[idx]->localheaderoffset_;
}


const char* ZipArchiveInfo::errorMsg() const
{ 
    if ( errormsg_.size() != 0 )
	return errormsg_.buf();

    return ziphd_.errorMsg();
}
