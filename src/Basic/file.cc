/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "file.h"

#include "bufstringset.h"
#include "commandlineparser.h"
#include "dirlist.h"
#include "envvars.h"
#include "errmsg.h"
#include "executor.h"
#include "filepath.h"
#include "filesystemaccess.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "pythonaccess.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "oscommand.h"
#include "uistrings.h"

#ifdef __win__
# include "winutils.h"
# include <direct.h>
#else
# include "sys/stat.h"
# include <unistd.h>
# include <utime.h>
#endif

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#define mMBFactor (1024*1024)
const char* not_implemented_str = "Not implemented";


mDefineNameSpaceEnumUtils(File,ViewStyle,"Examine View Style")
{
	"text",
	"table",
	"log",
	"bin",
	0
};


namespace File
{

static bool isSane( const char*& fnm )
{
    if ( !fnm )
	return false;
    mSkipBlanks( fnm );
    return *fnm;
}


static bool fnmIsURI( const char*& fnm )
{
    if ( !isSane(fnm) )
	return false;

    const char* protsep = FilePath::uriProtocolSeparator();
    const StringView uri( fnm );
    const BufferString ptrprotsep = uri.find( protsep );
    if ( !ptrprotsep.isEmpty() )
    {
	if ( uri.startsWith( "file://", CaseInsensitive ) )
	    fnm += 7;
	else if ( ptrprotsep.isEqual(fnm) )
	    fnm += StringView(protsep).size();
	else
	{
	    for ( const char* ptr=fnm; BufferString(ptr)!=ptrprotsep; ptr++ )
	    {
		if ( !isalnum(*ptr) || (ptr == fnm && isdigit(*ptr)) )
		    return false;
	    }
	    return true;
	}
    }

    return false;
}


static inline bool isLocal( const char* fnm )
{
    return isSane(fnm) && !fnmIsURI(fnm);
}


class RecursiveCopier : public Executor
{ mODTextTranslationClass(RecursiveCopier);
public:
			RecursiveCopier(const char* from,const char* to)
			    : Executor("Copying Folder")
			    , src_(from)
			    , dest_(to)
			    , msg_(tr("Copying files"))
			{
			    makeRecursiveFileList(src_,filelist_,false);
			    for ( int idx=0; idx<filelist_.size(); idx++ )
				totalnr_ += getFileSize( filelist_.get(idx) );
			}

    od_int64		nrDone() const override	{ return nrdone_ / mMBFactor; }
    od_int64		totalNr() const override { return totalnr_ / mMBFactor;}
    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override
			{ return tr("MBytes copied"); }

protected:

    int			nextStep() override;

    int			fileidx_	= 0;
    od_int64		totalnr_	= 0;
    od_int64		nrdone_		= 0;
    BufferStringSet	filelist_;
    BufferString	src_;
    BufferString	dest_;
    uiString		msg_;

};


#define mErrRet(s1) { msg_ = s1; return ErrorOccurred(); }
int RecursiveCopier::nextStep()
{
    if ( !fileidx_ )
    {
	if ( File::exists(dest_) && !File::remove(dest_) )
	    mErrRet(tr("Cannot overwrite %1").arg(dest_) )
	if( !File::createDir(dest_) )
	    mErrRet( uiStrings::phrCannotCreateDirectory(toUiString(dest_)) )
    }

    if ( fileidx_ >= filelist_.size() )
	return Finished();

    const BufferString& srcfile = *filelist_[fileidx_];
    QDir srcdir( src_.buf() );
    BufferString relpath( srcdir.relativeFilePath(srcfile.buf()) );
    const BufferString destfile = FilePath(dest_,relpath).fullPath();
    if ( File::isLink(srcfile) )
    {
	BufferString linkval = linkValue( srcfile );
	if ( !createLink(linkval,destfile) )
	    mErrRet(
	       uiStrings::phrCannotCreate(tr("symbolic link %1").arg(destfile)))
    }
    else if ( isDirectory(srcfile) )
    {
	if ( !File::createDir(destfile) )
	    mErrRet( uiStrings::phrCannotCreateDirectory(toUiString(destfile)) )
    }
    else if ( !File::copy(srcfile,destfile) )
	mErrRet( uiStrings::phrCannotCreate( tr("file %1").arg(destfile)) )

    fileidx_++;
    nrdone_ += getFileSize( srcfile );
    return fileidx_ >= filelist_.size() ? Finished() : MoreToDo();
}


class RecursiveDeleter : public Executor
{
public:
			RecursiveDeleter(const char* dirnm,
					 const BufferStringSet* externallist=0,
					 bool filesonly=false)
			    : Executor("Removing Files")
			    , dirname_(dirnm)
			    , filesonly_(filesonly)
			{
			    if ( !externallist )
			    {
				filelist_.add( dirname_ );
				makeRecursiveFileList( dirname_, filelist_  );
			    }
			    else
				filelist_ = *externallist;

			    totalnr_ = filelist_.size();
			}

    od_int64		nrDone() const override		{ return nrdone_; }
    od_int64		totalNr() const override	{ return totalnr_; }
    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override
			{ return mToUiStringTodo( "Files removed" ); }

    int			nextStep() override;

protected:

    od_int64		fileidx_	= 0;
    od_int64		totalnr_;
    od_int64		nrdone_		= 0;
    BufferStringSet	filelist_;
    BufferString	dirname_;
    uiString		msg_;
    bool		filesonly_;
};


int RecursiveDeleter::nextStep()
{
    if ( nrdone_ >= totalnr_ )
	return Finished();

    fileidx_ = totalnr_ - ( nrdone_ + 1 );
    const BufferString& filename = filelist_.get( fileidx_ );
    bool res = true;
    if ( File::isDirectory(filename) )
    {
	if ( filesonly_ )
	{
	    if ( isDirEmpty(filename) )
		res = File::removeDir( filename );
	}
	else
	    res = File::removeDir( filename );
    }
    else if( File::exists(filename) && !File::isDirectory(filename) )
	res = File::remove( filename );

    if ( !res )
    {
	uiString msg( mToUiStringTodo("Failed to remove ") );
	msg.append( filename );
	msg_ = msg;
    }

    nrdone_++;
    return MoreToDo();
}


Executor* getRecursiveCopier( const char* from, const char* to )
{
    return !isSane(from) || !isSane(to) ? nullptr
	: new File::RecursiveCopier( from, to );
}


Executor* getRecursiveDeleter( const char* dirname,
			       const BufferStringSet* externallist,
			       bool filesonly )
{
    return !isSane(dirname) ? nullptr
	: new File::RecursiveDeleter( dirname, externallist, filesonly );
}


void makeRecursiveFileList( const char* dir, BufferStringSet& filelist,
			    bool followlinks )
{
    if ( !isSane(dir) )
	return;

    DirList files( dir, FilesInDir );
    for ( int idx=0; idx<files.size(); idx++ )
    {
	if ( !followlinks )
	    filelist.add( files.fullPath(idx) );
	else
	    filelist.addIfNew( files.fullPath(idx) );
    }

    DirList dirs( dir, DirsInDir );
    for ( int idx=0; idx<dirs.size(); idx++ )
    {
	BufferString curdir( dirs.fullPath(idx) );
	if ( !followlinks )
	    filelist.add( curdir );
	else if ( !filelist.addIfNew(curdir) )
	    continue;

	if ( !isLink(curdir) )
	    makeRecursiveFileList( curdir, filelist, followlinks );
	else if ( followlinks )
	{
	    curdir = linkTarget( curdir );
	    if ( !filelist.isPresent(curdir) )
		makeRecursiveFileList( curdir, filelist, followlinks );
	}
    }
}


od_int64 getFileSize( const char* fnm, bool followlink )
{
    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.getFileSize( fnm, followlink );
}


bool exists( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.exists( fnm );
}


bool isEmpty( const char* fnm )
{
    return getFileSize( fnm ) < 1;
}


bool isDirEmpty( const char* dirnm )
{
    if ( !isLocal(dirnm) )
	return true;

    const QDir qdir( dirnm );
    return qdir.entryInfoList(QDir::NoDotAndDotDot|
			      QDir::AllEntries).count() == 0;
}


bool isFile( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.isFile( fnm );
}


bool isDirectory( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.isDirectory( fnm );
}


bool isURI( const char* fnm )
{
    return isSane(fnm) && fnmIsURI(fnm);
}


BufferString findExecutable( const char* exenm, const BufferStringSet& paths,
			     bool includesyspath )
{
    BufferString ret;
    if ( includesyspath )
	ret = QStandardPaths::findExecutable( exenm, QStringList() );

    if ( !paths.isEmpty() )
    {
	QStringList qpaths;
	for ( const auto* path : paths )
	    qpaths.append( path->buf() );

	const BufferString tmp = QStandardPaths::findExecutable( exenm,
								 qpaths );
	if ( !tmp.isEmpty() )
	    ret = tmp;
    }
    return ret;
}

const char* getCanonicalPath( const char* dir )
{
    mDeclStaticString( ret );
    const QDir qdir( dir );
    ret = qdir.canonicalPath();
    return ret.buf();
}


const char* getAbsolutePath( const char* dir, const char* relfnm )
{
    mDeclStaticString( ret );
    const QDir qdir( dir );
    ret = qdir.absoluteFilePath( relfnm );
    return ret.buf();
}


const char* getRelativePath( const char* reltodir, const char* fnm )
{
    BufferString reltopath = getCanonicalPath( reltodir );
    BufferString path = getCanonicalPath( fnm );
    mDeclStaticString( ret );
    const QDir qdir( reltopath.buf() );
    ret = qdir.relativeFilePath( path.buf() );
    return ret.isEmpty() ? fnm : ret.buf();
}


bool isLink( const char* fnm )
{
    if ( !isLocal(fnm) )
	return false;

    QFileInfo qfi( fnm );
    return qfi.isSymLink();
}


void hide( const char* fnm, bool yn )
{
    if ( !isLocal(fnm) || !exists(fnm) )
	return;

#ifdef __win__
    const int attr = GetFileAttributes( fnm );
    if ( yn )
    {
	if ( (attr & FILE_ATTRIBUTE_HIDDEN) == 0 )
	    SetFileAttributes( fnm, attr | FILE_ATTRIBUTE_HIDDEN );
    }
    else
    {
	if ( (attr & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN )
	    SetFileAttributes( fnm, attr & ~FILE_ATTRIBUTE_HIDDEN );
    }
#else
    (void)yn;
#endif
}


bool isHidden( const char* fnm )
{
    if ( !isLocal(fnm) )
	return false;

    QFileInfo qfi( fnm );
    return qfi.isHidden();
}


bool isReadable( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.isReadable( fnm );
}


bool isWritable( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.isWritable( fnm );
}


bool isExecutable( const char* fnm )
{
    if ( !isLocal(fnm) )
	return false;

    QFileInfo qfi( fnm );
    return qfi.isReadable() && qfi.isExecutable();
}


void setSystemFileAttrib( const char* fnm, bool yn )
{
    if ( !File::exists(fnm) )
	return;

#ifdef __win__
    DWORD attr = GetFileAttributes( fnm );
    if ( yn )
	SetFileAttributes( fnm, attr | FILE_ATTRIBUTE_SYSTEM );
    else
	SetFileAttributes( fnm, attr & ~FILE_ATTRIBUTE_SYSTEM );
#else
    (void)yn;
#endif
}


bool hasSystemFileAttrib( const char* fnm )
{
    if ( !File::exists(fnm) )
	return false;

#ifdef __win__
    DWORD attr = GetFileAttributes( fnm );
    return attr & FILE_ATTRIBUTE_SYSTEM;
#else
    return false;
#endif
}


bool isFileInUse( const char* fnm )
{
    return isInUse( fnm );
}


bool isInUse( const char* fnm )
{
    if ( !exists(fnm) )
	return false;

#ifdef __win__
    if ( isURI(fnm) )
	return false;

    return WinUtils::isFileInUse( fnm );
#else
    return false;
#endif
}


bool createDir( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.createDirectory( fnm );
}


bool listDir( const char* dirnm, DirListType dlt, BufferStringSet& fnames,
	      const char* mask )
{
    if ( !isSane(dirnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( dirnm );
    if ( !fsa.listDirectory(dirnm,dlt,fnames,mask) )
	return false;

    fnames.sort();
    return true;
}


bool rename( const char* oldname, const char* newname, uiString* errmsg )
{
    if ( !isSane(oldname) || !isSane(newname) )
	return false;

    if ( OD::FileSystemAccess::getProtocol(oldname) !=
	 OD::FileSystemAccess::getProtocol(newname) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( newname );
    return fsa.rename( oldname, newname );
}


bool createLink( const char* fnm, const char* linknm )
{
    if ( !isLocal(fnm) )
	return false;

    BufferString fulllinknm( linknm );
    if ( __iswin__ )
    {
	if ( !firstOcc(linknm,".lnk")  )
	    fulllinknm += ".lnk";
    }

    return QFile::link( fnm, fulllinknm.buf() );
}


bool saveCopy( const char* from, const char* to )
{
    if ( !isLocal(from) || !isLocal(to) )
	return false;

    if ( isDirectory(from) )
	return false;
    if ( !exists(to) )
	return File::copy( from, to );

    const BufferString tmpfnm( to, ".tmp" );
    if ( !File::rename(to,tmpfnm) )
	return false;

    const bool res = File::copy( from, to );
    res ? File::remove( tmpfnm ) : File::rename( tmpfnm, to );
    return res;
}


bool copy( const char* from, const char* to, BufferString* errmsg )
{
    if ( !isSane(from) || !isSane(to) )
	return false;

    if ( OD::FileSystemAccess::getProtocol(from) !=
	 OD::FileSystemAccess::getProtocol(to) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( from );

    if ( fsa.isDirectory(from) || fsa.isDirectory(to) )
	return copyDir( from, to, errmsg );

    uiString uimsg;
    const bool res = fsa.copy( from, to, errmsg ? &uimsg : nullptr );
    if ( errmsg )
	errmsg->set( uimsg );
    return res;
}


bool copyDir( const char* from, const char* to, BufferString* errmsg )
{
    if ( !isLocal(from) || !isLocal(to) )
	return false;

    if ( !exists(from) || exists(to) )
	return false;

    uiString errmsgloc;
    if ( !checkDir(from,true,&errmsgloc) || !checkDir(to,false,&errmsgloc) )
    {
	if ( errmsg && !errmsgloc.isEmpty() )
	    errmsg->add( errmsgloc.getFullString() );
	return false;
    }

    PtrMan<Executor> copier = getRecursiveCopier( from, to );
    const bool res = copier->execute();
    if ( !res && errmsg )
	errmsg->add( copier->uiMessage().getFullString() );

    return res;
}


bool resize( const char* fnm, od_int64 newsz )
{
    if ( !isLocal(fnm) )
	return false;
    else if ( newsz < 0 )
	return remove( fnm );

    return QFile::resize( fnm, newsz );
}


bool remove( const char* fnm )
{
    if ( !isSane(fnm) )
	return true;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.remove( fnm );
}


bool removeDir( const char* dirnm )
{
    if ( !isSane(dirnm) )
	return true;

    const auto& fsa = OD::FileSystemAccess::get( dirnm );
    return fsa.remove( dirnm, true );
}


bool changeDir( const char* dir )
{
    if ( !isLocal(dir) )
	return false;

#ifdef __win__
    return _chdir( dir )==0;
#else
    return chdir( dir )==0;
#endif
}


bool checkDir( const char* fnm, bool forread, uiString* errmsg )
{
    if ( !isSane(fnm) )
    {
	if ( errmsg )
	    *errmsg = ::toUiString( "Please specify a folder name" );
	return false;
    }

    const auto& fsa = OD::FileSystemAccess::get( fnm );

    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );

    const bool success = forread ? fsa.isReadable( dirnm )
				 : fsa.isWritable( dirnm );
    if ( !success && errmsg )
    {
	errmsg->set( forread ? "Cannot read in folder: "
			     : "Cannot write in folder: " );
	errmsg->append( dirnm );
	errmsg->appendPhrase( uiStrings::phrCheckPermissions() );
    }

    return success;
}


bool makeWritable( const char* fnm, bool yn, bool recursive )
{
    if ( !isSane(fnm) || !exists(fnm) )
	return false;
    else if ( fnmIsURI(fnm) )
	return true;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.setWritable( fnm, yn, recursive );
}


bool makeReadOnly(const char* fnm, bool recursive)
{
    return File::makeWritable( fnm, false, recursive );
}


bool makeExecutable( const char* fnm, bool yn )
{
    if ( __iswin__ )
	return true;

    if ( !isSane(fnm) )
	return false;
    else if ( fnmIsURI(fnm) )
	return !yn;

    OS::MachineCommand mc( "chmod" );
    mc.addArg( yn ? "+r+x" : "-x" ).addArg( fnm );
    return mc.execute();
}


bool setPermissions( const char* fnm, const char* perms, bool recursive )
{
    if ( __iswin__ )
	return false;

    if ( !isLocal(fnm) )
	return false;

    OS::MachineCommand mc( "chmod" );
    if ( recursive && isDirectory(fnm) )
	mc.addArg( "-R" );
    mc.addArg( perms ).addArg( fnm );
    return mc.execute();
}


bool getContent( const char* fnm, BufferString& bs )
{
    if ( !isSane(fnm) )
	return false;

    bs.setEmpty();
    if ( !fnm || !*fnm ) return false;

    od_istream stream( fnm );
    if ( stream.isBad() )
	return false;

    return !stream.isOK() ? true : stream.getAll( bs );
}


od_int64 getKbSize( const char* fnm )
{
    od_int64 kbsz = getFileSize( fnm ) / 1024;
    return kbsz;
}


BufferString getFileSizeString( od_int64 filesz ) // filesz in kB
{
    BufferString szstr;
    if ( filesz > 1024 )
    {
	const bool doGb = filesz > 1048576;
	const int nr = doGb ? mNINT32(filesz/10485.76) : mNINT32(filesz/10.24);
	szstr = nr/100;
	const int rest = nr%100;
	szstr += rest < 10 ? ".0" : "."; szstr += rest;
	szstr += doGb ? " GB" : " MB";
    }
    else if ( filesz == 0 )
	szstr = "< 1 kB";
    else
    {
	szstr = filesz;
	szstr += " kB";
    }

    return szstr;
}


BufferString getFileSizeString( const char* fnm )
{ return getFileSizeString( getKbSize(fnm) ); }


const char* timeCreated( const char* fnm, const char* fmt )
{
    mDeclStaticString( ret );
    const auto& fsa = OD::FileSystemAccess::get( fnm );
    ret = fsa.timeCreated( fnm );

    const StringView fmtstr = fmt;
    if ( !fmtstr.isEmpty() )
    {

	QDateTime qdt =
		QDateTime::fromString( ret.buf(), Qt::ISODate ).toLocalTime();
	ret = qdt.toString( fmt );
    }

    if ( ret.isEmpty() )
	ret.set( "-" );

    return ret.buf();
}


const char* timeLastModified( const char* fnm, const char* fmt )
{
    mDeclStaticString( ret );
    const auto& fsa = OD::FileSystemAccess::get( fnm );
    ret = fsa.timeLastModified( fnm );

    const StringView fmtstr = fmt;
    if ( !fmtstr.isEmpty() )
    {
	QDateTime qdt =
		QDateTime::fromString( ret.buf(), Qt::ISODate ).toLocalTime();
	ret = qdt.toString( fmt );
    }

    if ( ret.isEmpty() )
	ret.set( "-" );

    return ret.buf();
}


od_int64 getTimeInSeconds( const char* fnm, bool lastmodif )
{
    if ( !isLocal(fnm) || isEmpty(fnm) )
	return 0;

    const QFileInfo qfi( fnm );
    const QDateTime dt = lastmodif ? qfi.lastModified()
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
		     : qfi.birthTime();
#else
		     : qfi.created();
#endif

    return dt.toSecsSinceEpoch();
}


od_int64 getTimeInMilliSeconds( const char* fnm, bool lastmodif )
{
    const QFileInfo qfi( fnm );
    const QTime qtime = lastmodif ? qfi.lastModified().time()
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				: qfi.birthTime().time();
#else
				: qfi.created().time();
#endif

    const QTime daystart( 0, 0, 0, 0 );
    return daystart.msecsTo( qtime );
}


bool waitUntilExists( const char* fnm, double maxwaittm,
			    double* actualwaited )
{
    if ( actualwaited )
	*actualwaited = 0;
    if ( exists(fnm) )
	return true;

    const int msecsstart = Time::getMilliSeconds();
    const double checkincr = 0.1;
    double waittm = 0;
    bool appeared = true;
    while ( !exists(fnm) )
    {
	waittm += checkincr;
	if ( waittm > maxwaittm )
	    { appeared = false; break; }
	Threads::sleep( checkincr );
    }

    if ( actualwaited )
	*actualwaited = 1000. * (Time::getMilliSeconds() - msecsstart);

    return appeared;
}


const char* linkValue( const char* linknm )
{
    if ( !isSane(linknm) )
	return "";
    else if ( fnmIsURI(linknm) )
	return linknm;

#ifdef __win__
    return linkTarget( linknm );
#else
    mDeclStaticString( ret );
    ret.setMinBufSize( 1024 );
    const int len = readlink( linknm, ret.getCStr(), 1024 );
    if ( len < 0 )
	return linknm;

    ret[len] = '\0';
    return ret.buf();
#endif
}


const char* linkTarget( const char* linknm )
{
    if ( !isSane(linknm) )
	return "";
    else if ( fnmIsURI(linknm) )
	return linknm;

    mDeclStaticString( ret );
    const QFileInfo qfi( linknm );
    ret = qfi.isSymLink() ? qfi.symLinkTarget()
			  : linknm;
    return ret.buf();
}


const char* linkEnd( const char* linknm )
{
    mDeclStaticString( ret );
    const QFileInfo qfi( linknm );
    ret.set( qfi.canonicalFilePath() );
    return ret.buf();
}


const char* getCurrentPath()
{
    mDeclStaticString( ret );
    ret = QDir::currentPath();
    return ret.buf();
}


const char* getHomePath()
{
    mDeclStaticString( ret );
    ret = QDir::homePath();
    return ret.buf();
}


const char* sKeyODTMPDIR() { return "OD_TMPDIR"; }

BufferString& temppathstr()
{
    mDeclStaticString(ret);
    return ret;
}


const char* getTempPath()
{
    BufferString& ret = temppathstr();
    if ( !ret.isEmpty() )
	return ret.buf();

    const BufferString userpath( GetEnvVar(sKeyODTMPDIR()) );
    if ( !userpath.isEmpty() && isDirectory(userpath.str()) &&
	 isWritable(userpath.str()) )
    {
	ret.set( userpath );
	return ret.buf();
    }

    ret = QDir::tempPath();
    if ( __iswin__ )
	ret.replace( '/', '\\' );

    return ret.buf();
}


const char* getUserAppDataPath()
{
    mDeclStaticString( ret );
    ret = QStandardPaths::locate( QStandardPaths::GenericDataLocation, "",
				      QStandardPaths::LocateDirectory );
    return ret.buf();
}


const char* getRootPath( const char* path )
{
    mDeclStaticString( ret );
    const QDir qdir( path );
    ret = qdir.rootPath();
    return ret.buf();
}


const char* asciiFilesFilter()
{ return "ASCII Files (*.dat)"; }

const char* textFilesFilter()
{ return "Text Files (*.txt)"; }

const char* allFilesFilter()
{
    return __iswin__ ? "All Files (*.*)" : "All Files (*)";
}


bool launchViewer( const char* fnm, const ViewPars& vp )
{
    if ( !exists(fnm) )
	return false;

    OS::MachineCommand mc( "od_FileBrowser" );
    mc.addKeyedArg( ViewPars::sKeyFile(), fnm );
    mc.addKeyedArg( ViewPars::sKeyMaxLines(), vp.maxnrlines_ );
    mc.addKeyedArg( ViewPars::sKeyStyle(), toString(vp.style_) );
    if ( vp.editable_ )
	mc.addFlag( ViewPars::sKeyEdit() );

    return mc.execute( OS::RunInBG );
}


static bool canApplyScript( const char* scriptfnm )
{
    if ( !__iswin__ )
	return true;

    od_ostream strm( scriptfnm );
    if ( !strm.isOK() )
	return false;

    strm.add( "ECHO OFF" ).addNewLine()
	.add( "ECHO \'Expected output\'");
    strm.close();

    OS::MachineCommand cmd( scriptfnm );
    BufferString stdoutstr, stderrstr;
    const bool res = cmd.execute( stdoutstr, &stderrstr );
    remove( scriptfnm );
    return !res || !stderrstr.contains("is blocked by group policy");
}


bool initTempDir()
{
    if ( !__iswin__ )
	return true;

#ifdef __win__
    const bool hasapplocker = hasAppLocker();
#else
    bool hasapplocker = false;
#endif
    if ( !hasapplocker ||
	 !OD::PythonAccess::needCheckRunScript() ||
	 GetEnvVarYN("OD_DISABLE_APPLOCKER_TEST",false) )
	return true;

    FilePath targetfp( getTempPath(),
		       FilePath::getTempFileName("applocker_test", "bat") );
    BufferString tempfnm = targetfp.fullPath();
    if ( canApplyScript(tempfnm) )
	return true;

    BufferStringSet errmsgs;
    errmsgs.add( BufferString(
	"AppLocker prevents executing scripts in the folder: '",
	targetfp.pathOnly(), "'" ) );
    errmsgs.add("Some functionality of OpendTect might not work");

    const BufferString basedatadirstr( GetBaseDataDir() );
    if ( !File::isDirectory(basedatadirstr) )
    {
	errmsgs.insertAt(
            new BufferString("Data root is not set and thus cannot be used "
            "to store temporary files."), 1 );
	OD::DisplayErrorMessage( errmsgs.cat() );
	return false;
    }

    targetfp.set( GetBaseDataDir() ).add( "LogFiles" );
    BufferString targetpath( targetfp.fullPath() );
    bool logfpexists = exists( targetpath );
    bool logfpcreated = logfpexists || (!logfpexists && createDir(targetpath) );
    bool logfwritable = isWritable( targetpath );
    bool logfpmadewritable = logfwritable || (!logfwritable &&
			     makeWritable( targetpath, true, false) );
    if ( !logfpcreated || !logfpmadewritable )
    {
	errmsgs.insertAt(
		new BufferString("Cannot redirect temporary files to '",
				 targetpath,"' to store temporary files."), 1);
	errmsgs.insertAt(
		new BufferString("Cannot create that directory, "
				 "please check permissions"), 2 );
	OD::DisplayErrorMessage( errmsgs.cat() );
	return false;
    }

    targetfp.add( "Temp" ); targetpath.set( targetfp.fullPath() );
    logfpexists = exists( targetpath );
    logfpcreated = logfpexists || (!logfpexists && createDir(targetpath));
    logfwritable = isWritable( targetpath );
    logfpmadewritable = logfwritable || (!logfwritable &&
			makeWritable( targetpath, true, false ));
    if ( !logfpcreated || !logfpmadewritable )
    {
	errmsgs.insertAt(
		new BufferString("Cannot redirect temporary files to '",
				 targetpath,"' to store temporary files."), 1);
	errmsgs.insertAt(
		new BufferString("Cannot create that directory, "
				 "please check permissions"), 2);
	OD::DisplayErrorMessage( errmsgs.cat() );
	return false;
    }

    tempfnm = FilePath( targetpath,
	FilePath::getTempFileName("applocker_test","bat") ).fullPath();
    const bool res = canApplyScript( tempfnm );
    if ( !res )
    {
	errmsgs.insertAt(
		new BufferString("AppLocker also prevents executing scripts "
				 "in the replacement folder: '",
				 targetfp.fullPath(), "'"), 1);
	OD::DisplayErrorMessage( errmsgs.cat() );
	return false;
    }

    temppathstr().set( targetfp.fullPath() );

    OS::MachineCommand mc( "Echo", true );
    BufferString stdoutstr, stderrstr;
    mc.execute( stdoutstr, &stderrstr );
    if ( stderrstr.contains("is blocked by group policy") )
    {
	errmsgs.get(0).set(stderrstr);
	errmsgs.insertAt(
		new BufferString("AppLocker prevents executing the script '",
				 mc.program(), "'" ), 1 );
	OD::DisplayErrorMessage( errmsgs.cat() );
	return false;
    }

    return res;
}

} // namespace File
