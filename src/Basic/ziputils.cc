/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          December 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: ziputils.cc,v 1.25 2012-09-07 06:38:45 cvsraman Exp $";

#include "ziputils.h"

#include "bufstring.h"
#include "file.h"
#include "filepath.h"
#include "strmprov.h"
#include "ziparchiveinfo.h"

#include <iostream>

#define mDirCheck( dir ) \
    if ( !File::exists(dir) ) \
    { \
	errmsg_ = dir; \
	errmsg_ += " does not exist"; \
	return false; \
    } \



ZipUtils::ZipUtils( const char* filelistnm )
    : filelistname_( filelistnm )
    , needfilelist_( !filelistname_.isEmpty() )
    , ziphdler_(*new ZipHandler)
{}

ZipUtils::~ZipUtils()
{ delete &ziphdler_; }

bool ZipUtils::Zip( const char* src, const char* dest )
{
   mDirCheck( src );
   mDirCheck( dest );

#ifdef __win__
    return doZip( src, dest );
#else
    // TODO on Linux
    return false;
#endif
}

bool ZipUtils::UnZip( const char* src, const char* dest )
{
    mDirCheck( src );
    mDirCheck( dest );
    return doUnZip( src, dest );
}

bool ZipUtils::doZip( const char* src, const char* dest )
{
    return true;
}

bool ZipUtils::doUnZip( const char* src, const char* dest )
{
    bool tempfile = false;
    FilePath orgfnm( filelistname_ );
    if ( needfilelist_ )
    {
	if ( !File::exists( orgfnm.pathOnly() ) ) 
	{
	    tempfile = true;
	    FilePath listfp( src );
	    if (  listfp.nrLevels() <= 1 )
		filelistname_ = orgfnm.fileName();
	    else
	    {
		listfp = listfp.pathOnly();
		listfp.add( orgfnm.fileName() );
		filelistname_ = listfp.fullPath();
	    }
	}    
    }

    bool res = false;
#ifdef __win__
    BufferString cmd( "cmd /c unzip -o \"", src );
    cmd.add( "\" -d \"" ).add( dest ).add( "\"");
    if ( needfilelist_ )
	cmd.add( " > " ).add( "\"" ).add( filelistname_ ).add( "\"" );
    res = ExecOSCmd( cmd );
#else
    BufferString cmd( "unzip -o ", src );
    cmd.add( " -d " ).add( dest ).add( " > " )
	.add( needfilelist_ ? filelistname_ : "/dev/null" );
    res = !system( cmd );
#endif

    if ( res && tempfile )
    {
	File::copy( filelistname_, orgfnm.fullPath() );
	File::remove( filelistname_ );
	return true;
    }

    errmsg_ = !res ? " Unzip Failed" : ""; 
    return res;
}

bool ZipUtils::makeZip( BufferString& src, TaskRunner* tr,
			ZipHandler::CompLevel cl )
{
    ziphdler_.setCompLevel( cl );
    if( !ziphdler_.initMakeZip(src) )
	return false;

    Zipper exec( ziphdler_ );
    return tr ? tr->execute( exec ) : exec.execute();
}

bool ZipUtils::appendToArchive( BufferString& srcfnm, BufferString& fnm,
				TaskRunner* tr, ZipHandler::CompLevel cl )
{
    bool res;
    ziphdler_.setCompLevel( cl );
    res = ziphdler_.initAppend( srcfnm, fnm );
    if ( !res )
	return false;

    Zipper exec( ziphdler_ );
    res = tr ? tr->execute( exec ) : exec.execute();
    return res;
}


int Zipper::nextStep()
{
    int ret;
    ret = ziphd_.openStrmToRead(ziphd_.getAllFiles().get(nrdone_));
    if ( ret == 0 )
    {
	ziphd_.closeDestStream();
	return ErrorOccurred();
    }

    if ( ret == 1 )
    {
	ret = ziphd_.doZCompress();
	ziphd_.closeSrcStream();
	if ( !ret )
	{
	    ziphd_.closeDestStream();
	    return ErrorOccurred();
	}

    }

    nrdone_++;
    if ( nrdone_ < totalNr() )
	return MoreToDo();
	
    int ptrlctn = ziphd_.getDestStream().tellp();
    ret = ziphd_.setCntrlDirHeader();
    if ( !ret )
    {
	ziphd_.closeDestStream();
	return ErrorOccurred();
    }

    ret = ziphd_.setEndOfCntrlDirHeader ( ptrlctn );
    if ( !ret )
    {
	ziphd_.closeDestStream();
	return ErrorOccurred();
    }

    ziphd_.closeDestStream();
    return Finished();
}

od_int64 Zipper::nrDone() const
{ return nrdone_; }

od_int64 Zipper::totalNr() const
{ return ziphd_.getAllFiles().size(); }

const char* Zipper::nrDoneText() const
{ return ( "Files" ); }

const char* Zipper::message() const
{ return ziphd_.errorMsg(); }

bool ZipUtils::unZipArchive( BufferString& srcfnm,BufferString& basepath,
							    TaskRunner* tr )
{
    bool ret = ziphdler_.unZipArchiveInIt( srcfnm, basepath );
    if( !ret )
	return false;
    UnZipper exec( ziphdler_ );
    ret = tr ? tr->execute( exec ) : exec.execute();
    return ret;
}

bool ZipUtils::unZipFile( BufferString& srcfnm, BufferString& fnm )
{
    bool ret = ziphdler_.unZipFile( srcfnm, fnm );
    return ret;
}


int UnZipper::nextStep()
{
    bool ret = ziphd_.readFileHeader(); 
    if ( ret == 0)
	return ErrorOccurred();

    nrdone_++;
    return nrDone() < totalNr() ? MoreToDo() : Finished();
}

od_int64 UnZipper::nrDone() const
{ return nrdone_; }

od_int64 UnZipper::totalNr() const
{ return ziphd_.getTotalFiles(); }

const char* UnZipper::nrDoneText() const
{ return ( "Files" ); }

const char* UnZipper::message() const
{ return ziphd_.errorMsg(); }
