/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Salil Agarwal
 Date:          27 August 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: ziparchiveinfo.cc,v 1.8 2012-09-06 08:51:14 cvssalil Exp $";

#include "ziparchiveinfo.h"

#include "bufstringset.h"
#include "file.h"
#include "ziphandler.h"
#include "strmprov.h"

#include <iostream>

#define mLCompSize 20
#define mLUnCompSize 24
#define mLFnmLength 28
#define mLExtraFldLength 30
#define mLFileComntLength 32
#define mLRelOffset 42

ZipArchiveInfo::ZipArchiveInfo( BufferString& fnm )
    : ziphd_(*new ZipHandler)
{
    if ( readZipArchive( fnm ) )
	isok_ = true;
    else
	isok_ = false;
}


ZipArchiveInfo::~ZipArchiveInfo()
{
    delete &ziphd_;
}


bool ZipArchiveInfo::readZipArchive( BufferString& fnm )
{
    BufferString headerbuff, headerfnm;
    bool sigcheck;
    unsigned int ptrlocation;
    if ( !File::exists( fnm.buf() ) )
    {
	errormsg_ = fnm;
	errormsg_ += " :File does not exist";
	return false;
    }
    StreamData isd = StreamProvider( fnm ).makeIStream();
    if ( !isd.usable() )
    {
	errormsg_ = fnm;
	errormsg_ += ": Permission to access file denied";
	return false;
    }

    std::istream& src = *isd.istrm;
    ziphd_.readEndOfCntrlDirHeader( src );
    src.seekg( ziphd_.getOffsetOfCentralDir(), std::ios::beg );
    ptrlocation = src.tellg();
    for ( int i = 0; i < ziphd_.getTotalFiles(); i++ )
    {
	src.read( headerbuff.buf(), mCentralHeaderSize);
	mCntrlFileHeaderSigCheck( headerbuff, 0 );
	if ( !sigcheck )
	{
	    errormsg_ = fnm;
	    errormsg_ += ": File is corrupt";
	    return false;
	}

	src.seekg( ptrlocation + mCentralHeaderSize );
	src.read( headerfnm.buf(), *( (short*) (headerbuff.buf() + mLFnmLength) ) );
	FileInfo* fi = new FileInfo( headerfnm, 
				*( (unsigned int*) (headerbuff.buf() + mLCompSize) ),
				*( (unsigned int*) (headerbuff.buf() + mLUnCompSize) ),
				*( (unsigned int*) (headerbuff.buf() + mLRelOffset) ) );
	files_ += fi;
	ptrlocation = ptrlocation + *( (short*) (headerbuff.buf() + mLFnmLength) )
				  + *( (short*) (headerbuff.buf() + mLExtraFldLength) )
				  + *( (short*) (headerbuff.buf() + mLFileComntLength) )
				  + mCentralHeaderSize;
	src.seekg( ptrlocation );
    }
    
    isd.close();
    return true;
}

bool ZipArchiveInfo::getAllFnms( BufferStringSet& fnms )
{
    if ( !isok_ )
	return false;
    for( int i = 0; i < ziphd_.getTotalFiles(); i++ )
	fnms.add( files_[i]->fnm_ );
    return true;
}

od_int64 ZipArchiveInfo::getFCompSize( BufferString& fnm )
{
    if ( !isok_ )
	return -1;
    for( int i = 0; i < ziphd_.getTotalFiles(); i++ )
	if ( fnm.matches( files_[i]->fnm_ ) )
	    return files_[i]->compsize_;

    errormsg_ = fnm;
    errormsg_ += ": File not found";
    return -1;
}

od_int64 ZipArchiveInfo::getFCompSize( int idx )
{
    if ( !isok_ )
	return -1;
    if ( idx >= ziphd_.getTotalFiles() )
    {
	errormsg_ = "Index is out of range";
	return -1;
    }

    return files_[idx]->compsize_;
}
od_int64 ZipArchiveInfo::getFUnCompSize( BufferString& fnm )
{
    if ( !isok_ )
	return -1;
    for( int i = 0; i < ziphd_.getTotalFiles(); i++ )
	if ( fnm.matches( files_[i]->fnm_ ) )
	    return files_[i]->uncompsize_;
    
    errormsg_ = fnm;
    errormsg_ += ": File not found";
    return -1;
}

od_int64 ZipArchiveInfo::getFUnCompSize( int idx )
{
    if ( !isok_ )
	return -1;
    if ( idx >= ziphd_.getTotalFiles() )
    {
	errormsg_ = "Index is out of range";
	return -1;
    }

    return files_[idx]->uncompsize_;
}

od_int64 ZipArchiveInfo::getLocalHeaderOffset( BufferString& fnm )
{
    if ( !isok_ )
	return -1;
    fnm.buf()[fnm.size()] = '\0';
    for( int i = 0; i < ziphd_.getTotalFiles(); i++ )
    {
	files_[i]->fnm_.buf()[files_[i]->fnm_.size()] = '\0';
	if ( strstr( files_[i]->fnm_.buf(), fnm.buf() ) )
	    return files_[i]->localheaderoffset_;
    }
   
    errormsg_ = fnm;
    errormsg_ += ": File not found";
    return -1;
}

od_int64 ZipArchiveInfo::getLocalHeaderOffset( int idx )
{
    if ( !isok_ )
	return -1;
    if ( idx >= ziphd_.getTotalFiles() )
    {
	errormsg_ = "Index is out of range";
	return -1;
    }

    return files_[idx]->localheaderoffset_;
}

const char* ZipArchiveInfo::errorMsg()
{ return errormsg_.buf(); }