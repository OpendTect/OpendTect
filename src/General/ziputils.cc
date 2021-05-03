/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Ranojay Sen
Date:          December 2011
________________________________________________________________________

-*/

#include "ziputils.h"

#include "bufstring.h"
#include "dirlist.h"
#include "manobjectset.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "winutils.h"
#include "oscommand.h"
#include "fixedstring.h"
#include "uistrings.h"
#include "ziparchiveinfo.h"

#include <stdlib.h>


#define mBytesToMBFactor 1048576

#define mDirCheck( dir ) \
    if ( !File::exists(dir) ) \
{ \
    errmsg_ = uiStrings::phrDoesntExist(toUiString(dir)); \
    return false; \
} \



ZipUtils::ZipUtils( const char* filelistnm )
    : filelistname_(filelistnm)
    , needfilelist_(!filelistname_.isEmpty())
{}


ZipUtils::~ZipUtils()
{}


mStartAllowDeprecatedSection

bool ZipUtils::Zip( const char* src, const char* dest )
{
    mDirCheck( src );
    BufferStringSet srcfiles;
    const DirList dl( src );
    for ( int idx=0; idx<dl.size(); idx++ )
	srcfiles.add( dl.fullPath(idx) );

    return makeZip( dest, srcfiles, errmsg_ );
}


bool ZipUtils::UnZip( const char* src, const char* dest )
{
    mDirCheck( src );
    mDirCheck( dest );
    return unZipArchive( src, dest, errmsg_ );
}

mStopAllowDeprecatedSection

bool ZipUtils::makeFileList( const char* zipfilenm, BufferStringSet& list,
			     uiString& errmsg )
{
    ZipArchiveInfo zipinfo( zipfilenm );
    if ( !zipinfo.getAllFnms(list) )
    {
	errmsg = toUiString( zipinfo.errorMsg() );
	return false;
    }

    return true;
}


bool ZipUtils::makeZip( const char* zipfnm, const char* src,
			uiString& errmsg, TaskRunner* tr,
			ZipHandler::CompLevel cl )
{
    BufferStringSet src2;
    src2.add( src );
    return makeZip( zipfnm, src2, errmsg, tr, cl );
}


bool ZipUtils::makeZip( const char* zipfnm, const BufferStringSet& src,
			uiString& errmsg, TaskRunner* tr,
			ZipHandler::CompLevel cl )
{
    Zipper exec( zipfnm, src, cl );
    if ( !TaskRunner::execute(tr,exec) )
    {
	errmsg = exec.uiMessage();
        return false;
    }

    return true;
}


bool ZipUtils::appendToArchive( const char* srcfnm, const char* fnm,
				uiString& errmsg, TaskRunner* tr,
				ZipHandler::CompLevel cl )
{
    Zipper exec( srcfnm, fnm, cl );
    if ( !TaskRunner::execute(tr,exec) )
    {
	errmsg = exec.uiMessage();
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
{ return ziphd_.getNrDoneSize()/mBytesToMBFactor; }


od_int64 Zipper::totalNr() const
{ return ziphd_.getTotalSize()/mBytesToMBFactor; }


uiString Zipper::uiNrDoneText() const
{ return tr("MBytes processed: "); }


uiString Zipper::uiMessage() const
{
    const uiString errmsg( mToUiStringTodo(ziphd_.errorMsg() ) );
    if ( errmsg.isEmpty() )
	return tr("Archiving data");
    else
	return errmsg;
}


bool ZipUtils::unZipArchive( const char* srcfnm,const char* basepath,
			     uiString& errmsg, TaskRunner* tr )
{
    UnZipper exec( srcfnm, basepath );
    if ( !TaskRunner::execute(tr,exec) )
    {
	errmsg = exec.uiMessage();
        return false;
    }

    return true;
}


bool ZipUtils::unZipArchives( const BufferStringSet& archvs,
			     const char* basepath,
			     uiString& errmsg, TaskRunner* taskrunner )
{
    ExecutorGroup execgrp( "Archive unpacker" );
    for ( int idx=0; idx<archvs.size(); idx++ )
    {
	const BufferString& archvnm = archvs.get( idx );
	UnZipper* exec = new UnZipper( archvnm, basepath );
	execgrp.add( exec );
    }

    if ( !(TaskRunner::execute(taskrunner,execgrp)) )
    {
	errmsg = execgrp.uiMessage();
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
    : Executor("Unpacking Archive")
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
{ return ziphd_.getNrDoneSize()/mBytesToMBFactor; }


od_int64 UnZipper::totalNr() const
{ return ziphd_.getTotalSize()/mBytesToMBFactor; }


uiString UnZipper::uiNrDoneText() const
{ return tr("MBytes Processed: "); }


uiString UnZipper::uiMessage() const
{
    const uiString errmsg( mToUiStringTodo(ziphd_.errorMsg()));
    if ( errmsg.isEmpty() )
	return tr("Extracting data");
    else
	return errmsg;
}
