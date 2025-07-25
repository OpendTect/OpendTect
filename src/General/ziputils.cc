/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ziputils.h"

#include "bufstring.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "manobjectset.h"
#include "oddirs.h"
#include "oscommand.h"
#include "stringview.h"
#include "uistrings.h"
#include "winutils.h"
#include "ziparchiveinfo.h"

#include <stdlib.h>

#ifdef HAS_ZLIB
# include "zlib.h"
#endif


/*!
\brief Executor class which compresses files into zip format.
*/

class Zipper : public Executor
{ mODTextTranslationClass(Zipper)
public:
				Zipper(const char* fullfileordirnm,
				       const char* basepath,
				       const char* zipfnm,
				       ZipHandler::CompLevel);
				Zipper(const BufferStringSet& fullfilenms,
				       const char* basepath,
				       const char* zipfnm,
				       ZipHandler::CompLevel);
				~Zipper();

    uiString			uiMessage() const override;
    uiString			uiNrDoneText() const override;

private:
    od_int64			nrDone() const override;
    od_int64			totalNr() const override;
    double			progressFactor() const override;

    bool			doPrepare(od_ostream*) override;
    int				nextStep() override;
    bool			doFinish(bool,od_ostream*) override;

    ZipHandler			ziphd_;
    ObjectSet<ZipFileInfo>	fileinfos_;
    int				nrdone_ = 0;
    bool			isok_;
};


Zipper::Zipper( const BufferStringSet& srcfnms, const char* basepath,
		const char* zipfnm, ZipHandler::CompLevel cl )
    : Executor( "Making Zip Archive" )
{
    ziphd_.setCompLevel( cl );
    isok_ = ziphd_.initMakeZip( srcfnms, basepath, zipfnm );
}


Zipper::Zipper( const char* srcfnm, const char* basepath,
		const char* zipfnm,  ZipHandler::CompLevel cl )
    : Executor( "Making Zip Archive" )
{
    ziphd_.setCompLevel( cl );
    isok_ = ziphd_.initAppend( srcfnm, basepath, zipfnm );
}


Zipper::~Zipper()
{
    deepErase( fileinfos_ );
}


bool Zipper::doPrepare( od_ostream* strm )
{
    if ( !isok_ )
	return false;

    deepErase( fileinfos_ );
    return Executor::doPrepare( strm );
}


int Zipper::nextStep()
{
    auto* fileinfo = new ZipFileInfo;
    if ( !ziphd_.compressNextFile(*fileinfo) )
    {
	delete fileinfo;
	return ErrorOccurred();
    }

    fileinfos_.add( fileinfo );
    nrdone_++;
    return nrdone_ < ziphd_.getAllFileNames().size() ? MoreToDo() : Finished();
}


bool Zipper::doFinish( bool success, od_ostream* strm )
{
    if ( !success )
    {
	deepErase( fileinfos_ );
	return Executor::doFinish( success, strm );
    }

    if ( !ziphd_.setEndOfArchiveHeaders(fileinfos_) )
	return false;

    deepErase( fileinfos_ );
    return Executor::doFinish( success, strm );
}


od_int64 Zipper::nrDone() const
{ return ziphd_.getNrDoneSize(); }


od_int64 Zipper::totalNr() const
{ return ziphd_.getTotalSize(); }


double Zipper::progressFactor() const
{ return 1./mDef1MB; }


uiString Zipper::uiNrDoneText() const
{ return tr("MBytes read"); }


uiString Zipper::uiMessage() const
{
    const uiString errmsg = ziphd_.errMsg();
    if ( errmsg.isEmpty() )
	return tr("Archiving data");
    else
	return errmsg;
}


/*!
\brief Executor class which uncompresses files of zip format
*/

class UnZipper : public Executor
{ mODTextTranslationClass(UnZipper)
public:
				UnZipper(const char* zipfnm,
					 const char* destdir);
				~UnZipper();

    uiString			uiMessage() const override;
    uiString			uiNrDoneText() const override;

    static void			setFolderAttribs(ObjectSet<const ZipFileInfo>&);

private:
    od_int64			nrDone() const override;
    od_int64			totalNr() const override;
    double			progressFactor() const override;

    bool			doPrepare(od_ostream*) override;
    int				nextStep() override;
    bool			doFinish(bool success,od_ostream*) override;

    ZipHandler			ziphd_;
    ObjectSet<ZipFileInfo>	fileinfos_;
    int				nrdone_ = 0;
    bool			isok_;
};


UnZipper::UnZipper( const char* zipfnm, const char* destination )
    : Executor("Unpacking Archive")
{
    isok_ = ziphd_.initUnZipArchive( zipfnm, destination, fileinfos_ );
}


UnZipper::~UnZipper()
{
    deepErase( fileinfos_ );
}


bool UnZipper::doPrepare( od_ostream* strm )
{
    if ( !isok_ )
	return false;

    return Executor::doPrepare( strm );
}


int UnZipper::nextStep()
{
    ZipFileInfo* fileinfo = fileinfos_.validIdx(nrdone_)
			  ? fileinfos_.get( nrdone_ ) : nullptr;
    if ( !fileinfo || !ziphd_.extractNextFile(*fileinfo) )
	return ErrorOccurred();

    nrdone_++;
    return nrdone_ < ziphd_.getCumulativeFileCount() ? MoreToDo() : Finished();
}


bool UnZipper::doFinish( bool success, od_ostream* strm )
{
    if ( !success )
	return Executor::doFinish( success, strm );

    ObjectSet<const ZipFileInfo> dirinfos;
    for ( const auto* fileinfo : fileinfos_ )
    {
	if ( fileinfo->isDirectory() )
	    dirinfos.add( fileinfo );
    }

    setFolderAttribs( dirinfos );

    return Executor::doFinish( success, strm );
}


void UnZipper::setFolderAttribs( ObjectSet<const ZipFileInfo>& dirinfos )
{
    sort( dirinfos );
    for ( int idx=dirinfos.size()-1; idx>=0; idx-- )
    {
	const ZipFileInfo& dirinfo = *dirinfos.get( idx );
	const char* destfile = dirinfo.getFullFileName();
	if ( !dirinfo.getPermissions().isUdf() )
	    File::setPermissions( destfile, dirinfo.getPermissions() );

	if ( dirinfo.hasModTime() )
	    File::setTimes( destfile, dirinfo.times_, false );
    }
}


od_int64 UnZipper::nrDone() const
{ return ziphd_.getNrDoneSize(); }


od_int64 UnZipper::totalNr() const
{ return ziphd_.getTotalSize(); }


double UnZipper::progressFactor() const
{ return 1./mDef1MB; }


uiString UnZipper::uiNrDoneText() const
{ return tr("MBytes written"); }


uiString UnZipper::uiMessage() const
{
    const uiString errmsg = ziphd_.errMsg();
    if ( errmsg.isEmpty() )
	return tr("Extracting data");
    else
	return errmsg;
}


//! \brief MultiArchiveUnZipper

class MultiArchiveUnZipper : public Executor
{ mODTextTranslationClass(MultiArchiveUnZipper);
public:
MultiArchiveUnZipper( const BufferStringSet& archvs, const char* destination )
    : Executor("Unpacking Archives")
    , archives_(archvs)
    , dest_(destination)
{
    for ( int idx=0; idx<archives_.size(); idx++ )
    {
	ZipArchiveInfo info( archives_.get(idx) );
	totalnr_ += info.getTotalSize(true);
    }
}


bool doPrepare( od_ostream* strm ) override
{
    deepErase( dirinfos_ );
    return Executor::doPrepare( strm );
}


int nextStep() override
{
    ZipHandler zh;
    ObjectSet<ZipFileInfo> fileinfos;
    if ( !zh.initUnZipArchive(archives_.get(archidx_),dest_,fileinfos) )
    {
	deepErase( fileinfos );
	return ErrorOccurred();
    }

    const int nrfiles = zh.getCumulativeFileCount();
    if ( fileinfos.size() != nrfiles )
    {
	deepErase( fileinfos );
	return ErrorOccurred();
    }

    int prevnrdone = 0;
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	if ( !zh.extractNextFile(*fileinfos.get(idx)) )
	{
	    errmsg_ = zh.errMsg();
	    deepErase( fileinfos );
	    return ErrorOccurred();
	}

	const int curnrdone = zh.getNrDoneSize();
	nrdone_ += ( curnrdone - prevnrdone );
	prevnrdone = curnrdone;
    }

    for ( int idx=fileinfos.size()-1; idx>=0; idx-- )
    {
	const ZipFileInfo* fileinfo = fileinfos.get( idx );
	if ( fileinfo->isDirectory() )
	{
	    dirinfos_.add( fileinfo );
	    fileinfos.removeSingle( idx ); //no delete!
	}
    }

    deepErase( fileinfos );
    archidx_++;
    return archidx_ < archives_.size() ? MoreToDo() : Finished();
}


bool doFinish( bool success, od_ostream* strm ) override
{
    if ( !success )
	return Executor::doFinish( success, strm );

    UnZipper::setFolderAttribs( dirinfos_ );
    deepErase( dirinfos_ );

    return Executor::doFinish( success, strm );
}


od_int64 nrDone() const override
{ return nrdone_; }


od_int64 totalNr() const override
{ return totalnr_; }


double progressFactor() const override
{ return 1./mDef1MB; }


uiString uiNrDoneText() const override
{ return tr("MBytes written"); }


uiString uiMessage() const override
{
    if ( errmsg_.isEmpty() )
	return tr("Extracting data");
    else
	return errmsg_;
}

    const BufferStringSet&	archives_;
    ObjectSet<const ZipFileInfo> dirinfos_;
    const char*		dest_;
    int			archidx_ = 0;
    od_int64		nrdone_ = 0;
    od_int64		totalnr_ = 0;
    uiString		errmsg_;

};

// namespace ZipUtils


const char* ZipUtils::getZLibVersion()
{
#ifdef HAS_ZLIB
    return ZLIB_VERSION;
#else
    return nullptr;
#endif
}


bool ZipUtils::makeZip( const char* srcfnm, const char* basefp,
			const char* zipfilenm, uiString& errmsg,
			TaskRunner* taskrunner, ZipHandler::CompLevel cl )
{
    BufferString basepath;
    if ( basefp )
	basepath.set( basefp );
    else
    {
	const FilePath srcfp( srcfnm );
	if ( !srcfp.isEmpty() && srcfp.isAbsolute() )
	    basepath.set( srcfp.pathOnly() );
    }

    const BufferStringSet srcfnms( srcfnm );
    return makeZip( srcfnms, basepath.buf(), zipfilenm, errmsg, taskrunner, cl);
}


bool ZipUtils::makeZip( const BufferStringSet& srcfnms, const char* basepath,
			const char* zipfilenm, uiString& errmsg,
			TaskRunner* taskrunner, ZipHandler::CompLevel cl )
{
    Zipper exec( srcfnms, basepath, zipfilenm, cl );
    if ( !TaskRunner::execute(taskrunner,exec) )
    {
	errmsg = exec.uiMessage();
	return false;
    }

    return true;
}


bool ZipUtils::appendToArchive( const char* srcfnm, const char* basefp,
				const char* zipfilenm, uiString& errmsg,
				TaskRunner* taskrunner,
				ZipHandler::CompLevel cl )
{
    BufferString basepath;
    if ( basefp )
	basepath.set( basefp );
    else
    {
	const FilePath srcfp( srcfnm );
	if ( !srcfp.isEmpty() && srcfp.isAbsolute() )
	    basepath.set( srcfp.pathOnly() );
    }

    const BufferStringSet srcfnms( srcfnm );
    return makeZip( srcfnms, basepath.buf(), zipfilenm, errmsg, taskrunner, cl);
}


bool ZipUtils::unZipArchive( const char* zipfilenm, const char* dest,
			     uiString& errmsg, TaskRunner* taskrunner )
{
    UnZipper exec( zipfilenm, dest );
    if ( !TaskRunner::execute(taskrunner,exec) )
    {
	errmsg = exec.uiMessage();
	return false;
    }

    return true;
}


bool ZipUtils::unZipFile( const char* zipfilenm, const char* srcfnm,
			  const char* dest, uiString& errmsg,
			  TaskRunner* /* taskrunner */ )
{
    ZipHandler ziphdler;
    ZipFileInfo fileinfo;
    if ( !ziphdler.unZipFile(zipfilenm,srcfnm,dest,&fileinfo) )
    {
	errmsg = ziphdler.errMsg();
	return false;
    }

    if ( fileinfo.isDirectory() )
    {
	ObjectSet<const ZipFileInfo> dirinfos( &fileinfo );
	UnZipper::setFolderAttribs( dirinfos );
    }

    return true;
}


bool ZipUtils::unZipArchives( const BufferStringSet& zipfilenms,
			      const char* dest, uiString& errmsg,
			      TaskRunner* taskrunner )
{
    MultiArchiveUnZipper muz( zipfilenms, dest );
    if ( !TaskRunner::execute(taskrunner,muz) )
    {
	errmsg = muz.uiMessage();
	return false;
    }

    return true;
}


bool ZipUtils::makeFileList( const char* zipfilenm, BufferStringSet& list,
			     uiString& errmsg )
{
    const ZipArchiveInfo zipinfo( zipfilenm );
    if ( !zipinfo.getAllFnms(list) )
    {
	errmsg = zipinfo.errMsg();
	return false;
    }

    return true;
}
