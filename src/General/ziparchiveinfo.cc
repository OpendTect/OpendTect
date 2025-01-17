/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ziparchiveinfo.h"

#include "bufstringset.h"
#include "uistrings.h"
#include "ziphandler.h"

#include <iostream>


ZipArchiveInfo::ZipArchiveInfo( const char* fnm )
    : srcfnm_(fnm)
{
    isok_ = readZipArchive();
}


ZipArchiveInfo::~ZipArchiveInfo()
{
    deepErase( fileinfo_ );
}


bool ZipArchiveInfo::readZipArchive()
{
    ZipHandler ziphd;
    if ( ziphd.getArchiveInfo(srcfnm_.buf(),fileinfo_) )
	return true;

    errormsg_ = ziphd.errMsg();
    return false;
}


bool ZipArchiveInfo::getAllFnms( BufferStringSet& fnms ) const
{
    if ( !isok_ )
	return false;

    for ( const auto* fileinfo : fileinfo_ )
	fnms.add( fileinfo->getFileName() );

    return true;
}


od_int64 ZipArchiveInfo::getTotalSize( bool uncomp ) const
{
    od_int64 ret = 0;
    if ( !isok_ )
	return ret;

    for( int idx=0; idx<fileinfo_.size(); idx++ )
	ret += (uncomp ? getFileUnCompSize(idx) : getFileCompSize(idx) );

    return ret;
}


void ZipArchiveInfo::setFileNotPresentError( const char* fnm )
{
    errormsg_ = tr("File '%1' is not present in the archive '%2'")
		.arg( fnm ).arg( srcfnm_ );
}


od_int64 ZipArchiveInfo::getFileCompSize( const char* fnm ) const
{
    if ( !isok_ )
	return -1;

    const StringView filenm( fnm );
    for ( const auto* fileinfo : fileinfo_ )
	if ( filenm.matches(fileinfo->getFileName()) )
	    return fileinfo->compsize_;

    getNonConst(*this).setFileNotPresentError( fnm );
    return -1;
}


od_int64 ZipArchiveInfo::getFileCompSize( int idx ) const
{
    if ( !isok_ )
	return -1;

    if ( !fileinfo_.validIdx(idx) )
    {
	errormsg_ = tr("Index is out of range");
	return -1;
    }

    return fileinfo_[idx]->compsize_;
}


od_int64 ZipArchiveInfo::getFileUnCompSize( const char* fnm ) const
{
    if ( !isok_ )
	return -1;

    const StringView filenm( fnm );
    for ( const auto* fileinfo : fileinfo_ )
	if ( filenm.matches(fileinfo->getFileName()) )
	    return fileinfo->uncompsize_;

    getNonConst(*this).setFileNotPresentError( fnm );
    return -1;
}


od_int64 ZipArchiveInfo::getFileUnCompSize( int idx ) const
{
    if ( !isok_ )
	return -1;

    if ( !fileinfo_.validIdx(idx) )
    {
	errormsg_ = tr("Index is out of range");
	return -1;
    }

    return fileinfo_[idx]->uncompsize_;
}


const ZipFileInfo* ZipArchiveInfo::getInfo( const char* fnm ) const
{
    if ( !isok_ )
	return nullptr;

    const StringView filenm( fnm );
    for ( const auto* fileinfo : fileinfo_ )
    {
	if ( filenm.matches(fileinfo->getFileName()) )
	    return fileinfo;
    }

    getNonConst(*this).setFileNotPresentError( fnm );
    return nullptr;
}


bool ZipArchiveInfo::get( const char* fnm, ZipFileInfo& retinfo ) const
{
    const ZipFileInfo* info = getInfo( fnm );
    if ( !info )
	return false;

    retinfo = *info;
    return true;
}


uiString ZipArchiveInfo::errMsg() const
{
    return errormsg_;
}
