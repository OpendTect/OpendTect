/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Ranojay Sen
Date:          December 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "ziputils.h"

#include "bufstring.h"
#include "manobjectset.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "strmprov.h"
#include "winutils.h"
#include "ziparchiveinfo.h"

#include <iostream>

#define mBytesToKBFactor 1024

#define mDirCheck( dir ) \
    if ( !File::exists(dir) ) \
{ \
    errmsg_ = dir; \
    errmsg_ += " does not exist"; \
    return false; \
} \



ZipUtils::ZipUtils( const char* filelistnm )
    : filelistname_(filelistnm)
    , needfilelist_(!filelistname_.isEmpty())
{}


ZipUtils::~ZipUtils()
{}

bool ZipUtils::Zip( const char* src, const char* dest )
{
    mDirCheck( src );
    return doZip( src, dest );
}


bool ZipUtils::UnZip( const char* src, const char* dest )
{
    mDirCheck( src );
    mDirCheck( dest );
    return doUnZip( src, dest );
}


bool ZipUtils::doZip( const char* src, const char* dest )
{
    FilePath srcfp( src );
    BufferString newsrc = srcfp.fileName();
    FilePath zipcomfp( GetBinPlfDir(), "zip" );
    BufferString cmd( zipcomfp.fullPath() );
    cmd += " -r \""; 
    cmd += dest;
    cmd += "\"";
    cmd += " ";
    cmd += newsrc;
#ifndef __win__
    File::changeDir( srcfp.pathOnly() );
    const bool ret = !system( cmd );
    File::changeDir( GetSoftwareDir(0) );
    return ret;
#else
    const bool ret = execProc( cmd, true, false, srcfp.pathOnly() );
    return ret;
#endif
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

    if ( !res )
    {
        errmsg_ = "Unzip failed in the command: ";
        errmsg_ += cmd;
    }
    else
    {
        errmsg_.setEmpty();
    }

    return res;
}


bool ZipUtils::makeZip( const char* zipfnm, const char* src,
			BufferString& errmsg, TaskRunner* tr,
			ZipHandler::CompLevel cl )
{
    BufferStringSet src2;
    src2.add( src );
    return makeZip( zipfnm, src2, errmsg, tr, cl );
}


bool ZipUtils::makeZip( const char* zipfnm, const BufferStringSet& src,
       			BufferString& errmsg, TaskRunner* tr,
			ZipHandler::CompLevel cl )
{
    Zipper exec( zipfnm, src, cl );
    if ( !TaskRunner::execute(tr,exec) )
    {
        errmsg = exec.message();
        return false;
    }

    return true;
}


bool ZipUtils::appendToArchive( const char* srcfnm, const char* fnm,
				BufferString& errmsg, TaskRunner* tr,
				ZipHandler::CompLevel cl )
{
    Zipper exec( srcfnm, fnm, cl );
    if ( !TaskRunner::execute(tr,exec) )
    {
        errmsg = exec.message();
        return false;
    }

    return true;
}


Zipper::Zipper( const char* zipfnm, const BufferStringSet& srcfnms,
                ZipHandler::CompLevel cl )
    : Executor( "Making Zip Archive" )
    , nrdone_(0)	    
{ 
    ziphd_.setCompLevel( cl );
    isok_ = ziphd_.initMakeZip( zipfnm, srcfnms );
}


Zipper::Zipper( const char* zipfnm, const char* srcfnm,
                ZipHandler::CompLevel cl )
    : Executor( "Making Zip Archive" )
    , nrdone_(0)	    
{ 
    ziphd_.setCompLevel( cl );
    isok_ = ziphd_.initAppend( zipfnm, srcfnm );
}


int Zipper::nextStep()
{
    if ( !isok_ || !ziphd_.compressNextFile() ) 
	return ErrorOccurred();

    nrdone_++;
    if ( nrdone_ < ziphd_.getAllFileNames().size() )
	return MoreToDo();

    return ziphd_.setEndOfArchiveHeaders() ? Finished() : ErrorOccurred();
}


od_int64 Zipper::nrDone() const
{ return ziphd_.getNrDoneSize()/mBytesToKBFactor; }


od_int64 Zipper::totalNr() const
{ return ziphd_.getTotalSize()/mBytesToKBFactor; }


const char* Zipper::nrDoneText() const
{ return "Files archived"; }


const char* Zipper::message() const
{ 
    const FixedString errmsg( ziphd_.errorMsg() );
    if ( errmsg.isEmpty() )
	return "Archiving files";
    else
	return errmsg;
}


bool ZipUtils::unZipArchive( const char* srcfnm,const char* basepath,
			     BufferString& errmsg, TaskRunner* tr )
{
    UnZipper exec( srcfnm, basepath );
    if ( !TaskRunner::execute(tr,exec) )
    {
        errmsg = exec.message();
        return false;
    }

    return true;
}


bool ZipUtils::unZipArchives( const BufferStringSet& archvs,const char* basepath,
			     BufferString& errmsg, TaskRunner* tr )
{
    ExecutorGroup execgrp( "Archive unpacker" );
    for ( int idx=0; idx<archvs.size(); idx++ )
    {
	const BufferString& archvnm = archvs.get( idx );
	UnZipper* exec = new UnZipper( archvnm, basepath );
	execgrp.add( exec );
    }

    if ( !(TaskRunner::execute(tr,execgrp)) )
    {
	errmsg = execgrp.message();
	return false;
    }

    return true;
}


bool ZipUtils::unZipFile( const char* srcfnm, const char* fnm, const char* path,
			  BufferString& errmsg )
{
    ZipHandler ziphdler;
    if ( !ziphdler.unZipFile(srcfnm,fnm,path) )
    {
        errmsg = ziphdler.errorMsg();
        return false;
    }

    return true;
}


UnZipper::UnZipper( const char* zipfnm,const char* destination )
    : Executor("Unpacking Files")
    , nrdone_(0)	    
{ isok_ = ziphd_.initUnZipArchive( zipfnm, destination ); }


int UnZipper::nextStep()
{
    if ( !isok_ || !ziphd_.extractNextFile() )
	return ErrorOccurred();

    nrdone_++;
    return nrdone_ < ziphd_.getCumulativeFileCount() ? MoreToDo() : Finished();
}


od_int64 UnZipper::nrDone() const
{ return ziphd_.getNrDoneSize()/mBytesToKBFactor; }


od_int64 UnZipper::totalNr() const
{ return ziphd_.getTotalSize()/mBytesToKBFactor; }


const char* UnZipper::nrDoneText() const
{ return "Files unpacked"; }


const char* UnZipper::message() const
{ 
    const FixedString errmsg( ziphd_.errorMsg() );
    if ( errmsg.isEmpty() )
	return "Unpacking files";
    else
	return errmsg;
}

