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
#include "nrbytes2string.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "oscommand.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "pythonaccess.h"
#include "uistrings.h"

#ifdef __win__
# include "winutils.h"
# include <direct.h>
#else
# include "sys/stat.h"
# include <unistd.h>
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
	nullptr
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
	if ( uri.startsWith( "file://", OD::CaseInsensitive ) )
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


class RecursiveCopier : public Executor
{ mODTextTranslationClass(RecursiveCopier);
public:
			RecursiveCopier( const char* from, const char* to,
					 bool preserve )
			    : Executor("Copying Folder")
			    , src_(from)
			    , dest_(to)
			    , preserve_(preserve)
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
    bool		preserve_;
    BufferString	src_;
    BufferString	dest_;
    uiString		msg_;

};


#define mErrRet(s1) { msg_ = s1; return ErrorOccurred(); }
int RecursiveCopier::nextStep()
{
    if ( !fileidx_ )
    {
	if ( File::exists(dest_) )
	{
	    if ( File::isDirectory(dest_) && !File::isSymLink(dest_) )
	    {
		if ( !File::removeDir(dest_) )
		    mErrRet(tr("Cannot overwrite %1").arg(dest_) )
	    }
	    else if ( !File::remove(dest_) )
		mErrRet(tr("Cannot overwrite %1").arg(dest_) )
	}

	if ( !File::createDir(dest_) )
	    mErrRet( uiStrings::phrCannotCreateDirectory(toUiString(dest_)) )
    }

    if ( fileidx_ >= filelist_.size() )
	return Finished();

    const BufferString& srcfile = *filelist_[fileidx_];
    const QDir srcdir( src_.buf() );
    BufferString relpath( srcdir.relativeFilePath(srcfile.buf()) );
    const BufferString destfile = FilePath(dest_,relpath).fullPath();
    if ( File::isSymLink(srcfile) )
    {
	const QFileInfo qfi( srcfile.buf() );
	const BufferString linkval( qfi.symLinkTarget() );
	if ( !createLink(linkval,destfile) )
	    mErrRet(
	       uiStrings::phrCannotCreate(tr("symbolic link %1").arg(destfile)))
    }
    else if ( isDirectory(srcfile) )
    {
	if ( !File::createDir(destfile) )
	    mErrRet( uiStrings::phrCannotCreateDirectory(toUiString(destfile)) )
    }
    else if ( !File::copy(srcfile,destfile,preserve_) )
	mErrRet( uiStrings::phrCannotCreate( tr("file %1").arg(destfile)) )

    fileidx_++;
    nrdone_ += getFileSize( srcfile );
    return fileidx_ >= filelist_.size() ? Finished() : MoreToDo();
}


class RecursiveDeleter : public Executor
{ mODTextTranslationClass(RecursiveDeleter)
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
			{ return tr( "Files removed" ); }

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
	    const QDir qdir( filename.buf() );
	    if ( qdir.entryInfoList(QDir::NoDotAndDotDot |
				    QDir::AllEntries).count() == 0 )
		res = File::removeDir( filename );
	}
	else
	    res = File::removeDir( filename );
    }
    else if( File::exists(filename) && !File::isDirectory(filename) )
	res = File::remove( filename );

    if ( !res )
    {
	uiString msg( tr("Failed to remove ") );
	msg.append( filename );
	msg_ = msg;
    }

    nrdone_++;
    return MoreToDo();
}


Executor* getRecursiveCopier( const char* from, const char* to, bool preserve )
{
    return !isSane(from) || !isSane(to) ? nullptr
	: new File::RecursiveCopier( from, to, preserve );
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

	if ( !isSymLink(curdir) )
	    makeRecursiveFileList( curdir, filelist, followlinks );
	else if ( followlinks )
	{
	    curdir = linkEnd( curdir );
	    if ( !filelist.isPresent(curdir) )
		makeRecursiveFileList( curdir, filelist, followlinks );
	}
    }
}


bool exists( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.exists( fnm );
}


Permissions getPermissions( const char* fnm )
{
    if ( !isSane(fnm) || !exists(fnm) )
	return Permissions::udf();

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.getPermissions( fnm );
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
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.isExecutable( fnm );
}


bool isHidden( const char* fnm )
{
    const Permissions perms = getPermissions( fnm );
    return perms.isUdf() ? false : perms.isHidden();
}


bool isSystem( const char* fnm )
{
    const Permissions perms = getPermissions( fnm );
    return perms.isUdf() ? false : perms.isSystem();
}


bool isLocal( const char* fnm )
{
    return isSane(fnm) && !fnmIsURI(fnm);
}


bool isURI( const char* fnm )
{
    return isSane(fnm) && fnmIsURI(fnm);
}


Type getType( const char* fnm, bool followlinks )
{
    if ( !isSane(fnm) )
	return Type::Unknown;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.getType( fnm, followlinks );
}


bool isFile( const char* fnm )
{
    const Type typ = getType( fnm, true );
    return typ == Type::File;
}


bool isDirectory( const char* fnm )
{
    const Type typ = getType( fnm, true );
    return typ == Type::Directory;
}


bool isSymLink( const char* fnm )
{
    const Type typ = getType( fnm, false );
    return typ == Type::SymLink || typ == Type::Alias || typ == Type::Shortcut;
}


bool isSymbolicLink( const char* fnm )
{
    const Type typ = getType( fnm, false );
    return typ == Type::SymLink;
}


bool isShortcut( const char* fnm )
{
    if ( __iswin__ )
    {
	const Type typ = getType( fnm, false );
	return typ == Type::Shortcut;
    }

    return false;
}


bool isInUse( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.isInUse( fnm );
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


bool createDir( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.createDirectory( fnm );
}


bool createLink( const char* fnm, const char* linknm )
{
    if ( !isSane(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.createLink( fnm, linknm );
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
    {
	if ( errmsg )
	    errmsg->appendPhrase( od_static_tr("rename",
						"Irregular name found.") );

	return false;
    }

    if ( OD::FileSystemAccess::getProtocol(oldname) !=
	 OD::FileSystemAccess::getProtocol(newname) )
    {
	if ( errmsg )
	    errmsg->appendPhrase( od_static_tr("rename",
			    "Incompatible target and destination protocols") );

	return false;
    }

    const auto& fsa = OD::FileSystemAccess::get( newname );
    return fsa.rename( oldname, newname, errmsg );
}


bool copy( const char* from, const char* to, bool preserve,
	   uiString* errmsg, TaskRunner* taskrun )
{
    if ( !isSane(from) || !isSane(to) )
	return false;

    const auto& fromfsa = OD::FileSystemAccess::get( from );
    const auto& tofsa = OD::FileSystemAccess::get( to );
    const bool islink = isSymLink( from );
    BufferString fromfnm( from );
    if ( islink && !preserve )
    {
	fromfnm = linkEnd( from );
	preserve = true;
    }

    if ( (isDirectory(fromfnm) || isDirectory(to)) && !islink )
	return copyDir( fromfnm, to, preserve, errmsg, taskrun );

    if ( !fromfsa.isLocal() && !tofsa.isLocal() && &fromfsa != &tofsa )
    {
	// probably never supported?
	return false;
    }

    const auto& cpfsa = !fromfsa.isLocal() ? fromfsa : tofsa;
    const bool res = cpfsa.copy( fromfnm, to, preserve, errmsg );

    return res;
}


bool copyDir( const char* from, const char* to, bool preserve,
	      uiString* errmsg, TaskRunner* taskrun )
{
    if ( !isLocal(from) || !isLocal(to) )
	return false;

    if ( !exists(from) || exists(to) )
	return false;

    if ( !checkDir(from,true,errmsg) || !checkDir(to,false,errmsg) )
	return false;

    const bool islink = isSymLink( from );
    BufferString fromfnm( from );
    if ( islink )
    {
	if ( preserve )
	    return copy( from, to, preserve, errmsg, taskrun );

	fromfnm = linkEnd( from );
	preserve = true;
    }

    PtrMan<Executor> copier = getRecursiveCopier( fromfnm, to, preserve );
    if ( !copier )
	return false;

    const bool res = TaskRunner::execute( taskrun, *copier.ptr() );
    if ( !res && errmsg )
	*errmsg = copier->uiMessage();

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
    bool recursive = false;
    const File::Type typ = getType( fnm );
    if ( typ == File::Type::Directory )
    {
	const bool isempty = isEmpty( fnm );
	if ( !isempty )
	{
	    recursive = true;
	    pFreeFnErrMsg( "Use File::removeDir to recursively remove a "
			   "non-empty directory" );
	}
    }

    return fsa.remove( fnm, recursive );
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
	{
	    *errmsg = od_static_tr( "File::checkDir",
				    "Please specify a folder name" );
	}

	return false;
    }

    const auto& fsa = OD::FileSystemAccess::get( fnm );

    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );

    const bool success = forread ? fsa.isReadable( dirnm )
				 : fsa.isWritable( dirnm );
    if ( !success && errmsg )
    {
	*errmsg = forread ? od_static_tr( "File::checkDir",
					  "Cannot read in folder: %1" )
			  : od_static_tr( "File::checkDir",
					  "Cannot write in folder: %1" );
	errmsg->arg( dirnm );
	errmsg->appendPhrase( uiStrings::phrCheckPermissions() );
    }

    return success;
}


bool setPermissions( const char* fnm, const Permissions& perms )
{
    if ( !isSane(fnm) || !exists(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.setPermissions( fnm, perms );
}


bool setReadOnly( const char* fnm, bool recursive )
{
    return setWritable( fnm, false, recursive );
}


bool setWritable( const char* fnm, bool yn, bool recursive )
{
    if ( !isSane(fnm) || !exists(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.setWritable( fnm, yn, recursive );
}


bool setExecutable( const char* fnm, bool yn, bool recursive )
{
    if ( __iswin__ )
	return false;

    if ( !isSane(fnm) || !exists(fnm) )
	return false;

    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.setExecutable( fnm, yn, recursive );
}


bool setHiddenFileAttrib( const char* fnm, bool yn )
{
    if ( !__iswin__ || !isLocal(fnm) )
	return false;

    Permissions perms = getPermissions( fnm );
    if ( perms.isUdf() )
	return false;

    perms.setHidden( yn );
    return setPermissions( fnm, perms );
}


bool setSystemFileAttrib( const char* fnm, bool yn )
{
    if ( !__iswin__ || !isSane(fnm) || !isLocal(fnm) )
	return false;

    Permissions perms = getPermissions( fnm );
    if ( perms.isUdf() )
	return false;

    perms.setSystem( yn );
    return setPermissions( fnm, perms );
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


bool isEmpty( const char* fnm )
{
    if ( isDirectory(fnm) )
    {
	BufferStringSet fnames;
	if ( !listDir(fnm,AllEntriesInDir,fnames) )
	    return false;

	return fnames.isEmpty();
    }

    return getFileSize( fnm ) < 1;
}


od_int64 getFileSize( const char* fnm, bool followlink )
{
    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.getFileSize( fnm, followlink );
}


od_int64 getKbSize( const char* fnm )
{
    od_int64 kbsz = getFileSize( fnm ) / mDef1KB;
    return kbsz;
}


BufferString getFileSizeString( od_int64 fileszbytes, File::SizeUnit fsu )
{
    BufferString szstr;
    if ( fileszbytes < 0 )
    {
	szstr = "File not found";
	return szstr;
    }

    if ( fileszbytes < 1 )
    {
	szstr = "empty";
	return szstr;
    }

    NrBytesToStringCreator converter( fileszbytes );
    szstr.add( converter.getString(fileszbytes) );
    return szstr;
}


BufferString getFileSizeString( const char* fnm, File::SizeUnit fsu )
{
    if ( !File::exists(fnm) )
	return getFileSizeString( -1, fsu );

    if ( !File::isReadable(fnm) )
    {
	BufferString ret( "unknown" );
	return ret;
    }

    return getFileSizeString( getFileSize(fnm), fsu );
}


const char* linkEnd( const char* linknm )
{
    mDeclStaticString( ret );
    const auto& fsa = OD::FileSystemAccess::get( linknm );
    ret = fsa.linkEnd( linknm );
    return ret.buf();
}


const char* timeCreated( const char* fnm, const char* fmt, bool followlink )
{
    mDeclStaticString( ret );
    const auto& fsa = OD::FileSystemAccess::get( fnm );
    ret = fsa.timeCreated( fnm, followlink );

    const StringView fmtstr = fmt;
    if ( !fmtstr.isEmpty() )
    {
	const QDateTime qdt =
		QDateTime::fromString( ret.buf(), Qt::ISODate ).toLocalTime();
	ret = qdt.toString( fmt );
    }

    if ( ret.isEmpty() )
	ret.set( "-" );

    return ret.buf();
}


const char* timeLastModified( const char* fnm, const char* fmt, bool followlink)
{
    mDeclStaticString( ret );
    const auto& fsa = OD::FileSystemAccess::get( fnm );
    ret = fsa.timeLastModified( fnm, followlink );

    const StringView fmtstr = fmt;
    if ( !fmtstr.isEmpty() )
    {
	const QDateTime qdt =
		QDateTime::fromString( ret.buf(), Qt::ISODate ).toLocalTime();
	ret = qdt.toString( fmt );
    }

    if ( ret.isEmpty() )
	ret.set( "-" );

    return ret.buf();
}


od_int64 getTimeInSeconds( const char* fnm, bool lastmodif, bool followlink )
{
    const od_int64 ret = getTimeInMilliSeconds( fnm, lastmodif, followlink );
    return ret < 0 ? -1 : od_int64 (ret / 1000);
}


od_int64 getTimeInMilliSeconds( const char* fnm, bool lastmodif,bool followlink)
{
    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.getTimeInMilliSeconds( fnm, lastmodif, followlink );
}


bool getTimes( const char* fnm, Time::FileTimeSet& times, bool followlink )
{
    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.getTimes( fnm, times, followlink );
}


bool setTimes( const char* fnm, const Time::FileTimeSet& times, bool followlink)
{
    const auto& fsa = OD::FileSystemAccess::get( fnm );
    return fsa.setTimes( fnm, times, followlink );
}


bool waitUntilExists( const char* fnm, double maxwaittm,
			    double* actualwaited )
{
    if ( actualwaited )
	*actualwaited = 0.;
    if ( exists(fnm) )
	return true;

    const od_int64 msecsstart = Time::getMilliSeconds();
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


int maxPathLength()
{
    return FILENAME_MAX <= 0 ? mUdf(int) : FILENAME_MAX;
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
			     setWritable( targetpath, true ) );
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
			setWritable( targetpath, true ) );
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


// Permissions

Permissions::Permissions( int perms, bool isuid, bool isgid, bool sticky )
    : IntegerID(perms)
    , isuid_(isuid)
    , isgid_(isgid)
    , sticky_(sticky)
{
}


static QFileDevice::Permission getQPerm( const Permission& perm )
{
    switch ( perm ) {
	case Permission::ReadOwner: return QFileDevice::ReadOwner;
	case Permission::WriteOwner: return QFileDevice::WriteOwner;
	case Permission::ExeOwner: return QFileDevice::ExeOwner;
	case Permission::ReadUser: return QFileDevice::ReadUser;
	case Permission::WriteUser: return QFileDevice::WriteUser;
	case Permission::ExeUser: return QFileDevice::ExeUser;
	case Permission::ReadGroup: return QFileDevice::ReadGroup;
	case Permission::WriteGroup: return QFileDevice::WriteGroup;
	case Permission::ExeGroup: return QFileDevice::ExeGroup;
	case Permission::ReadOther: return QFileDevice::ReadOther;
	case Permission::WriteOther: return QFileDevice::WriteOther;
	case Permission::ExeOther: return QFileDevice::ExeOther;
	default: return QFileDevice::ReadOwner;
    }
}


bool Permissions::testFlag( const Permission& perm ) const
{
    const QFile::Permission qperm = getQPerm( perm );
    const QFile::Permissions qperms( asInt() );
    return qperms.testFlag( qperm );
}


bool Permissions::isHidden() const
{
#ifdef __win__
    const int attr = asInt();
    return attr & FILE_ATTRIBUTE_HIDDEN;
#else
    return false;
#endif
}


bool Permissions::isSystem() const
{
#ifdef __win__
    const int attr = asInt();
    return attr & FILE_ATTRIBUTE_SYSTEM;
#else
    return false;
#endif
}


Permissions& Permissions::setFlag( const Permission& perm, bool on )
{
    const QFile::Permission qperm = getQPerm( perm );
    QFile::Permissions qperms( asInt() );
    qperms.setFlag( qperm, on );
    this->set( qperms );
    return *this;
}


Permissions& Permissions::setHidden( bool yn )
{
#ifdef __win__
    int attr = asInt();
    if ( yn )
	attr |= FILE_ATTRIBUTE_HIDDEN;
    else
	attr &= ~FILE_ATTRIBUTE_HIDDEN;

    set( attr );
#endif

    return *this;
}


Permissions& Permissions::setSystem( bool yn )
{
#ifdef __win__
    int attr = asInt();
    if ( yn )
	attr |= FILE_ATTRIBUTE_SYSTEM;
    else
	attr &= ~FILE_ATTRIBUTE_SYSTEM;

    set( attr );
#endif

    return *this;
}


Permissions Permissions::getFrom( int st_mode, int uid )
{
#ifdef __win__
    return udf();
#else
# ifdef __debug__
    const bool mUnusedVar isfile = S_ISREG( st_mode );
    const bool mUnusedVar isdir = S_ISDIR( st_mode );
    const bool mUnusedVar islink = S_ISLNK( st_mode );
# endif
    const int mode = st_mode - (st_mode & S_IFMT);
    const int procuid = getuid();
    const bool isowned = mIsUdf(uid) || uid < 0 || uid == procuid;
    const bool canreaduser = mode & S_IRUSR;
    const bool canwriteuser = mode & S_IWUSR;
    int ret = 0;
    if ( canreaduser )
    {
	ret += int (File::Permission::ReadUser);
	if ( isowned )
	    ret += int (File::Permission::ReadOwner);
    }

    if ( canwriteuser )
    {
	ret += int (File::Permission::WriteUser);
	if ( isowned )
	    ret += int (File::Permission::WriteOwner);
    }

    if ( mode & S_IXUSR )
    {
	ret += int (File::Permission::ExeUser);
	if ( isowned )
	    ret += int (File::Permission::ExeOwner);
    }

    if ( mode & S_IRGRP )
	ret += int (File::Permission::ReadGroup);
    if ( mode & S_IWGRP )
	ret += int (File::Permission::WriteGroup);
    if ( mode & S_IXGRP )
	ret += int (File::Permission::ExeGroup);
    if ( mode & S_IROTH )
	ret += int (File::Permission::ReadOther);
    if ( mode & S_IWOTH )
	ret += int (File::Permission::WriteOther);
    if ( mode & S_IXOTH )
	ret += int (File::Permission::ExeOther);

    return Permissions( ret );
#endif
}


Permissions Permissions::getDefault( bool forfile )
{
#ifdef __win__
    static Permissions fileperms( FILE_ATTRIBUTE_ARCHIVE );
    static Permissions dirperms( FILE_ATTRIBUTE_DIRECTORY );
#else
    static Permissions fileperms( 26214 );
    static Permissions dirperms( 30583 );
    static bool retmask = false;
    if ( !retmask )
    {
	const mode_t procmask = umask( 002 ); //get
	umask( procmask ); //restore
	const Permissions maskperms = Permissions::getFrom( procmask, getuid());
	fileperms.setFlag( File::Permission::ReadOwner, true );
	fileperms.setFlag( File::Permission::WriteOwner, true );
	fileperms.setFlag( File::Permission::ReadUser,
			   !maskperms.testFlag(File::Permission::ReadUser) );
	fileperms.setFlag( File::Permission::WriteUser,
			   !maskperms.testFlag(File::Permission::WriteUser) );
	fileperms.setFlag( File::Permission::ReadGroup,
			   !maskperms.testFlag(File::Permission::ReadGroup) );
	fileperms.setFlag( File::Permission::WriteGroup,
			   !maskperms.testFlag(File::Permission::WriteGroup) );
	fileperms.setFlag( File::Permission::ReadOther,
			   !maskperms.testFlag(File::Permission::ReadOther) );
	fileperms.setFlag( File::Permission::WriteOther,
			   !maskperms.testFlag(File::Permission::WriteOther) );
	dirperms = fileperms;
	dirperms.setFlag( File::Permission::ExeOwner,
			  !maskperms.testFlag(File::Permission::ExeOwner) );
	dirperms.setFlag( File::Permission::ExeUser,
			  !maskperms.testFlag(File::Permission::ExeUser) );
	dirperms.setFlag( File::Permission::ExeGroup,
			  !maskperms.testFlag(File::Permission::ExeGroup) );
	dirperms.setFlag( File::Permission::ExeOther,
			  !maskperms.testFlag(File::Permission::ExeOther) );
	retmask = true;
    }
#endif

    return forfile ? fileperms : dirperms;
}


int Permissions::get_st_mode( const Type& typ ) const
{
#ifdef __win__
    return mUdf(int);
#else
    int ret = 0;
    if ( typ == Type::File )
	ret += S_IFREG;
    else if ( typ == Type::Directory )
	ret += S_IFDIR;
    else if ( typ == Type::SymLink )
    {
	ret += S_IFLNK + ACCESSPERMS;
    }
    else if ( typ == Type::Character )
	ret += S_IFCHR;
    else if ( typ == Type::Block )
	ret += S_IFBLK;
    else if ( typ == Type::Fifo )
	ret += S_IFIFO;
    else if ( typ == Type::Socket )
	ret += S_IFSOCK + ACCESSPERMS;

    if ( typ == Type::File || typ == Type::Directory )
    {
	const QFile::Permissions qperms( asInt() );
	if ( qperms.testFlag(QFileDevice::ReadUser) )
	    ret += S_IRUSR;
	if ( qperms.testFlag(QFileDevice::WriteUser) )
	    ret += S_IWUSR;
	if ( qperms.testFlag(QFileDevice::ExeUser) )
	    ret += S_IXUSR;
	if ( qperms.testFlag(QFileDevice::ReadGroup) )
	    ret += S_IRGRP;
	if ( qperms.testFlag(QFileDevice::WriteGroup) )
	    ret += S_IWGRP;
	if ( qperms.testFlag(QFileDevice::ExeGroup) )
	    ret += S_IXGRP;
	if ( qperms.testFlag(QFileDevice::ReadOther) )
	    ret += S_IROTH;
	if ( qperms.testFlag(QFileDevice::WriteOther) )
	    ret += S_IWOTH;
	if ( qperms.testFlag(QFileDevice::ExeOther) )
	    ret += S_IXOTH;
	if ( isuid_ )
	    ret += S_ISUID;
	if ( isgid_ )
	    ret += S_ISGID;
	if ( sticky_ )
	    ret += S_ISVTX;
    }

    return ret;
#endif
}


int Permissions::get_st_mode( const char* fnm )
{
    if ( __iswin__ )
	return mUdf(int);

    const Permissions perms = getPermissions( fnm );
    const Type typ = getType( fnm );
    return perms.get_st_mode( typ );
}


// Deprecated implementations

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


bool isDirEmpty( const char* dirnm )
{
    if ( !isLocal(dirnm) )
	return true;

    const QDir qdir( dirnm );
    return qdir.entryInfoList(QDir::NoDotAndDotDot|
			      QDir::AllEntries).count() == 0;
}


bool saveCopy( const char* from, const char* to, bool preserve )
{
    if ( !isLocal(from) || !isLocal(to) )
	return false;

    if ( isDirectory(from) )
	return false;
    if ( !exists(to) )
	return File::copy( from, to, preserve );

    const BufferString tmpfnm( to, ".tmp" );
    if ( !File::rename(to,tmpfnm) )
	return false;

    const bool res = copy( from, to, preserve );
    res ? File::remove( tmpfnm ) : File::rename( tmpfnm, to );
    return res;
}

} // namespace File
