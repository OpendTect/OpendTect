/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		3-5-1994
 Contents:	File utitlities
________________________________________________________________________

-*/

#include "file.h"
#include "fileview.h"
#include "filesystemaccess.h"

#include "filepath.h"
#include "bufstringset.h"
#include "commandlineparser.h"
#include "dirlist.h"
#include "envvars.h"
#include "staticstring.h"
#include "executor.h"
#include "ptrman.h"
#include "od_istream.h"
#include "oddirs.h"
#include "oscommand.h"
#include "timefun.h"
#include "uistrings.h"


#ifdef __win__
# include <direct.h>
#else
# include "sys/stat.h"
# include <unistd.h>
#endif

#ifndef OD_NO_QT
# include <QDateTime>
# include <QDir>
# include <QFile>
# include <QFileInfo>
# include <QProcess>
# include <QStandardPaths>
#endif


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

template<>
void EnumDefImpl<File::ViewStyle>::init()
{
    uistrings_ += mEnumTr("Text",0);
    uistrings_ += uiStrings::sTable();
    uistrings_ += uiStrings::sLog();
    uistrings_ += mEnumTr("Bin",0);
}


static inline bool isSane( const char*& fnm )
{
    if ( !fnm )
	return false;
    mSkipBlanks( fnm );
    return *fnm;
}

static inline bool fnmIsURI( const char*& fnm )
{
    if ( !isSane(fnm) )
	return false;

    const char* protsep = File::Path::uriProtocolSeparator();
    const FixedString uri( fnm );
    const char* ptrprotsep = uri.find( protsep );
    if ( ptrprotsep )
    {
	if ( uri.startsWith( "file://", CaseInsensitive ) )
	    fnm += 7;
	else if ( ptrprotsep == fnm )
	    fnm += FixedString(protsep).size();
	else
	{
	    for ( const char* ptr=fnm; ptr!=ptrprotsep; ptr++ )
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


namespace File
{

class RecursiveCopier : public Executor
{ mODTextTranslationClass(RecursiveCopier);
public:
			RecursiveCopier(const char* from,const char* to)
			    : Executor("Copying folder")
			    , src_(from),dest_(to),fileidx_(0)
			    , totalnr_(0),nrdone_(0)
			    , msg_(tr("Copying files"))
			{
			    makeRecursiveFileList(src_,filelist_,false);
			    for ( int idx=0; idx<filelist_.size(); idx++ )
				totalnr_ += getFileSize( filelist_.get(idx) );
			}

    od_int64		nrDone() const		{ return nrdone_ / mMBFactor; }
    od_int64		totalNr() const		{ return totalnr_ / mMBFactor; }
    uiString		message() const { return msg_; }
    uiString		nrDoneText() const	{ return tr("MBytes copied"); }

protected:

    int			nextStep();

    int			fileidx_;
    od_int64		totalnr_;
    od_int64		nrdone_;
    BufferStringSet	filelist_;
    BufferString	src_;
    BufferString	dest_;
    uiString		msg_;

};

} // namespace File


#define mErrRet(s1) { msg_ = s1; return ErrorOccurred(); }
int File::RecursiveCopier::nextStep()
{
#ifdef OD_NO_QT
    return ErrorOccurred();
#else
    if ( fileidx_ >= filelist_.size() )
	return Finished();

    if ( !fileidx_ )
    {
	if ( File::exists(dest_) && !File::remove(dest_) )
	    mErrRet(tr("Cannot overwrite %1").arg(dest_) )
	if( !File::createDir(dest_) )
	    mErrRet( uiStrings::phrCannotCreateDirectory(dest_) )
    }

    const BufferString& srcfile = filelist_.get( fileidx_ );
    QDir srcdir( src_.buf() );
    BufferString relpath( srcdir.relativeFilePath(srcfile.buf()) );
    const BufferString destfile = Path(dest_,relpath).fullPath();
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
	    mErrRet( uiStrings::phrCannotCreateDirectory(destfile) )
    }
    else if ( !File::copy(srcfile,destfile) )
	mErrRet( uiStrings::phrCannotCreate( tr("file %1").arg(destfile)) )

    fileidx_++;
    nrdone_ += getFileSize( srcfile );
    return MoreToDo();
#endif
}


namespace File
{

class RecursiveDeleter : public Executor
{ mODTextTranslationClass(RecursiveDeleter)
public:
			RecursiveDeleter(const char* dirnm,
					 const BufferStringSet* externallist=0,
					 bool filesonly=false)
			    : Executor("Removing Files")
			    , dirname_(dirnm)
			    , nrdone_(0)
			    , fileidx_(0)
			    , filesonly_(filesonly)
			{
			    if ( externallist )
				filelist_ = *externallist;
			    else
			    {
				filelist_.add( dirname_ );
				makeRecursiveFileList( dirname_, filelist_ );
			    }

			    totalnr_ = filelist_.size();
			}

    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }
    uiString		message() const { return msg_; }
    uiString		nrDoneText() const
			{ return tr("Number of files removed"); }

    int			nextStep();

protected:

    od_int64		fileidx_;
    od_int64		totalnr_;
    od_int64		nrdone_;
    BufferStringSet	filelist_;
    BufferString	dirname_;
    uiString		msg_;
    bool		filesonly_;
};

} // namespace File


int File::RecursiveDeleter::nextStep()
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
    else if( File::exists(filename) )
	res = File::remove( filename );

    if ( !res )
    {
	msg_ = tr("Failed to remove '%1'").arg( filename );
	return ErrorOccurred();
    }

    nrdone_++;
    return MoreToDo();
}


Executor* File::getRecursiveCopier( const char* from, const char* to )
{
    return !isSane(from) || !isSane(to) ? 0
	 : new File::RecursiveCopier( from, to );
}

Executor* File::getRecursiveDeleter( const char* dirname,
			       const BufferStringSet* externallist,
			       bool filesonly )
{ return !isSane(dirname) ? 0
       : new File::RecursiveDeleter( dirname, externallist, filesonly ); }


void File::makeRecursiveFileList( const char* dir, BufferStringSet& filelist,
				  bool followlinks )
{
    if ( !isSane(dir) )
	return;

    DirList files( dir, File::FilesInDir );
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

	if ( !File::isLink(curdir) )
	    makeRecursiveFileList( curdir, filelist, followlinks );
	else if ( followlinks )
	{
	    curdir = File::linkTarget( curdir );
	    if ( !filelist.isPresent(curdir) )
		makeRecursiveFileList( curdir, filelist, followlinks );
	}
    }
}


od_int64 File::getFileSize( const char* fnm, bool followlink )
{
    const SystemAccess& fsa = SystemAccess::get( fnm );
    return fsa.getFileSize( fnm, followlink );
}


bool File::exists( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const SystemAccess& fsa = SystemAccess::get( fnm );
    return fsa.exists( fnm );
}


bool File::isEmpty( const char* fnm )
{
    return getFileSize( fnm ) < 1;
}


bool File::isDirEmpty( const char* dirnm )
{
    if ( !isLocal(dirnm) )
	return true;

#ifndef OD_NO_QT
    const QDir qdir( dirnm );
    return qdir.entryInfoList(QDir::NoDotAndDotDot|
			      QDir::AllEntries).count() == 0;
#else
    return false;
#endif
}


bool File::isFile( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const SystemAccess& fsa = SystemAccess::get( fnm );
    return fsa.isFile( fnm );
}


bool File::isDirectory( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const SystemAccess& fsa = SystemAccess::get( fnm );
    return fsa.isDirectory( fnm );
}


bool File::isURI( const char*& fnm )
{
    return isSane(fnm) && fnmIsURI(fnm);
}


BufferString File::findExecutable( const char* exenm,
				   const BufferStringSet& paths,
				   bool includesyspath )
{
#ifndef OD_NO_QT
    BufferString ret;
    if ( includesyspath )
	ret = QStandardPaths::findExecutable( exenm, QStringList() );

    if ( !paths.isEmpty() )
    {
	QStringList qpaths;
	for ( const auto path : paths )
	    qpaths.append( path->str() );
	const BufferString tmp = QStandardPaths::findExecutable(exenm, qpaths);
	if ( !tmp.isEmpty() )
	    ret = tmp;
    }
    return ret;
#else
    pFreeFnErrMsg(not_implemented_str);
    return BufferString::empty();
#endif
}


const char* File::getCanonicalPath( const char* dir )
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    const QDir qdir( dir );
    ret = qdir.canonicalPath();
#else
    pFreeFnErrMsg(not_implemented_str);
    ret = dir;
#endif
    return ret.buf();
}


const char* File::getRelativePath( const char* reltodir, const char* fnm )
{
#ifndef OD_NO_QT
    BufferString reltopath = getCanonicalPath( reltodir );
    BufferString path = getCanonicalPath( fnm );
    mDeclStaticString( ret );
    const QDir qdir( reltopath.buf() );
    ret = qdir.relativeFilePath( path.buf() );
    return ret.isEmpty() ? fnm : ret.buf();
#else
    pFreeFnErrMsg(not_implemented_str);
    return fnm;
#endif
}


bool File::isLink( const char* fnm )
{
    if ( !isLocal(fnm) )
	return false;

#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    return qfi.isSymLink();
#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif
}


void File::hide( const char* fnm, bool yn )
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
#endif
}


bool File::isHidden( const char* fnm )
{
    if ( !isLocal(fnm) )
	return false;

#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    return qfi.isHidden();
#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif
}


bool File::isReadable( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const SystemAccess& fsa = SystemAccess::get( fnm );
    return fsa.isReadable( fnm );
}


bool File::isWritable( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const SystemAccess& fsa = SystemAccess::get( fnm );
    return fsa.isWritable( fnm );
}


bool File::isExecutable( const char* fnm )
{
    if ( !isLocal(fnm) )
	return false;

#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    return qfi.isReadable() && qfi.isExecutable();
#else
    struct stat st_buf;
    int status = stat(fnm, &st_buf);
    if ( status != 0 )
	return false;

    return st_buf.st_mode & S_IXUSR;
#endif
}



bool File::isInUse( const char* fnm )
{
#ifdef __win__
    if ( isURI(fnm) )
	return false;

    HANDLE handle = CreateFileA( fnm,
				 GENERIC_READ | GENERIC_WRITE,
				 0,
				 0,
				 OPEN_EXISTING,
				 0,
				 0 );
    const bool ret = handle == INVALID_HANDLE_VALUE;
    CloseHandle( handle );
    return ret;
#else
    return false;
#endif
}


bool File::createDir( const char* fnm )
{
    if ( !isSane(fnm) )
	return false;

    const SystemAccess& fsa = SystemAccess::get( fnm );
    return fsa.createDirectory( fnm );
}


bool File::listDir( const char* dirnm, DirListType dlt, BufferStringSet& fnames,
		    const char* mask )
{
    if ( !isSane(dirnm) )
	return false;

    const SystemAccess& fsa = SystemAccess::get( dirnm );
    if ( !fsa.listDirectory(dirnm,dlt,fnames,mask) )
	return false;

    fnames.sort();
    return true;
}


bool File::rename( const char* oldname, const char* newname )
{
    if ( !isSane(oldname) || !isSane(newname) )
	return false;

    if ( SystemAccess::getProtocol(oldname) !=
	 SystemAccess::getProtocol(newname) )
	return false;

    const SystemAccess& fsa = SystemAccess::get( newname );
    return fsa.rename( oldname, newname );
}


bool File::createLink( const char* fnm, const char* linknm )
{
    if ( !isLocal(fnm) )
	return false;

#ifndef OD_NO_QT
#ifdef __win__
    BufferString winlinknm( linknm );
    if ( !firstOcc(linknm,".lnk")  )
	winlinknm += ".lnk";
    return QFile::link( fnm, winlinknm.buf() );
#else
    return QFile::link( fnm, linknm );
#endif // __win__

#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif
}


bool File::saveCopy( const char* from, const char* to )
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


bool File::copy( const char* from, const char* to, uiString* errmsg )
{
    if ( !isSane(from) || !isSane(to) )
	return false;

    if ( SystemAccess::getProtocol(from) != SystemAccess::getProtocol(to) )
	return false;

    const SystemAccess& fsa = SystemAccess::get( from );

    if ( fsa.isDirectory(from) || fsa.isDirectory(to)  )
	return copyDir( from, to, errmsg );

    return fsa.copy( from, to, errmsg );
}


bool File::copyDir( const char* from, const char* to, uiString* errmsg )
{
    if ( !isLocal(from) || !isLocal(to) )
	return false;

    if ( !exists(from) || exists(to) )
	return false;

    uiString errmsgloc;
    if ( !checkDirectory(from,true,errmsg ? *errmsg:errmsgloc) ||
	 !checkDirectory(to,false,errmsg ? *errmsg:errmsgloc) )
	return false;

    PtrMan<Executor> copier = getRecursiveCopier( from, to );
    const bool res = copier->execute();
    if ( !res && errmsg )
	*errmsg = copier->message();
    return res;
}


bool File::resize( const char* fnm, od_int64 newsz )
{
    if ( !isLocal(fnm) )
	return false;
    else if ( newsz < 0 )
	return remove( fnm );

    return QFile::resize( fnm, newsz );
}


bool File::remove( const char* fnm )
{
    if ( !isSane(fnm) )
	return true;

    const SystemAccess& fsa = SystemAccess::get( fnm );
    return fsa.remove( fnm );
}


bool File::removeDir( const char* dirnm )
{
    if ( !isSane(dirnm) )
	return true;

    const SystemAccess& fsa = SystemAccess::get( dirnm );
    return fsa.remove( dirnm, true );
}


bool File::changeDir( const char* dir )
{
    if ( !isLocal(dir) )
	return false;
#ifdef __win__
    return _chdir( dir )==0;
#else
    return chdir( dir )==0;
#endif
}


bool File::checkDirectory( const char* fnm, bool forread, uiString& errmsg )
{
    if ( !isSane(fnm) )
    {
	errmsg = od_static_tr( "FilecheckDirectory",
			       "Please specify a folder name" );
	return false;
    }

    const SystemAccess& fsa = SystemAccess::get( fnm );

    Path fp( fnm );
    BufferString dirnm( fp.pathOnly() );

    const bool success = forread ? fsa.isReadable( dirnm )
				 : fsa.isWritable( dirnm );

    uiString postfix = od_static_tr( "FilecheckDirectory", "in folder: %1" )
				   .arg( dirnm );
    errmsg = forread ? od_static_tr( "FilecheckDirectory",
						" Cannot read in folder: %1")
		     : od_static_tr( "FilecheckDirectory",
						" Cannot write in folder: %1");
    errmsg.arg(dirnm);
    errmsg.appendPhrase( uiStrings::phrCheckPermissions() );

    return success;
}


bool File::makeWritable( const char* fnm, bool yn, bool recursive )
{
    if ( !isSane(fnm) || !exists(fnm) )
	return false;
    else if ( fnmIsURI(fnm) )
	return true;

#ifdef OD_NO_QT
    return false;
#else
    const BufferString filenm( "\"", fnm, "\"" );
    BufferStringSet args;
# ifdef __win__
    const QString qprog( "attrib" );
    args.add( yn ? "-R" : "+R" ).add( filenm );
    if ( recursive && isDirectory(fnm) )
	args.add( "\\*.*" ).add( "/S" );
# else
    const QString qprog( "chmod" );
    if ( recursive && isDirectory(fnm) )
	args.add( "-R" );
    args.add( yn ? "ug+w" : "a-w" ).add( filenm );
# endif

    QStringList qargs;
    args.fill( qargs );
    return QProcess::execute( qprog, qargs ) >= 0;
#endif
}


bool File::makeReadOnly( const char* fnm, bool recursive )
{
    return File::makeWritable( fnm, false, recursive );
}


bool File::makeExecutable( const char* fnm, bool yn )
{
    if ( !isSane(fnm) )
	return false;
    else if ( fnmIsURI(fnm) )
	return !yn;

#if ((defined __win__) || (defined OD_NO_QT) )
    return true;
#else
    const QString qprog( "chmod" );
    BufferStringSet args;
    args.add( yn ? "+r+x" : "-x" )
	.add( BufferString("\"",fnm,"\"") );
    QStringList qargs;
    args.fill( qargs );
    return QProcess::execute( qprog, qargs ) >= 0;
#endif
}


bool File::setPermissions( const char* fnm, const char* perms, bool recursive )
{
    if ( !isLocal(fnm) )
	return false;

#if ((defined __win__) || (defined OD_NO_QT) )
    return false;
#else
    const QString qprog( "chmod" );
    BufferStringSet args;
    if ( recursive && isDirectory(fnm) )
	args.add( "-R" );
    args.add( perms )
	.add( BufferString("\"",fnm,"\"") );
    QStringList qargs;
    args.fill( qargs );
    return QProcess::execute( qprog, qargs ) >= 0;
#endif
}


bool File::getContent( const char* fnm, BufferString& bs )
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


od_int64 File::getKbSize( const char* fnm )
{
    od_int64 kbsz = getFileSize( fnm ) / 1024;
    return kbsz;
}


BufferString File::getFileSizeString( od_int64 filesz ) // filesz in kB
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


BufferString File::getFileSizeString( const char* fnm )
{ return getFileSizeString( getKbSize(fnm) ); }


#define mRetUnknown { ret.set( "<unknown>" ); return ret.buf(); }


const char* File::timeCreated( const char* fnm )
{
    mDeclStaticString( ret );
    if ( !isLocal(fnm) )
	mRetUnknown

#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
    ret = qfi.birthTime().toString( Qt::ISODate );
#else
    ret = qfi.created().toString( Qt::ISODate );
#endif
    return ret.buf();
#else
    pFreeFnErrMsg(not_implemented_str);
    mRetUnknown
#endif
}


const char* File::timeLastModified( const char* fnm )
{
    mDeclStaticString( ret );
    if ( !isLocal(fnm) )
	mRetUnknown

#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    ret = qfi.lastModified().toString( Qt::ISODate );
    return ret.buf();
#else
    pFreeFnErrMsg(not_implemented_str);
    mRetUnknown
#endif
}


od_int64 File::getTimeInSeconds( const char* fnm, bool lastmodif )
{
    if ( !isLocal(fnm) || isEmpty(fnm) )
	return 0;

#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    return lastmodif ? qfi.lastModified().toTime_t()
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
		     : qfi.birthTime().toTime_t();
#else
		     : qfi.created().toTime_t();
#endif

#else
    struct stat st_buf;
    int status = stat(fnm, &st_buf);
    if (status != 0)
	return 0;

    return lastmodif ? st_buf.st_mtime : st_buf.st_ctime;
#endif
}


od_int64 File::getTimeInMilliSeconds( const char* fnm, bool lastmodif )
{
#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    const QTime qtime = lastmodif ? qfi.lastModified().time()
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				: qfi.birthTime().time();
#else
				: qfi.created().time();
#endif

    const QTime daystart( 0, 0, 0, 0 );
    return daystart.msecsTo( qtime );
#else
    struct stat st_buf;
    int status = stat( fnm, &st_buf );
    if ( status != 0 )
	return 0;

    return lastmodif ? st_buf.st_mtime * 1000 : st_buf.st_ctime * 1000;
#endif
}


bool File::waitUntilExists( const char* fnm, double maxwaittm,
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


const char* File::linkValue( const char* linknm )
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


const char* File::linkTarget( const char* linknm )
{
    if ( !isSane(linknm) )
	return "";
    else if ( fnmIsURI(linknm) )
	return linknm;

    mDeclStaticString( ret );
#ifndef OD_NO_QT
    const QFileInfo qfi( linknm );
    ret = qfi.isSymLink() ? qfi.symLinkTarget()
			  : linknm;
#else
    pFreeFnErrMsg(not_implemented_str);
    ret = linknm;
#endif
    return ret.buf();
}


const char* File::linkEnd( const char* linknm )
{
    BufferString prvfnm = linknm;
    for ( int ifollow=0; ; ifollow++ )
    {
	if ( ifollow == 100 )
	    { prvfnm = linknm; break; }

	BufferString curfnm = linkTarget( prvfnm );
	if ( curfnm == prvfnm )
	    break;
	prvfnm = curfnm;
    }

    mDeclStaticString( ret );
    ret = prvfnm;
    return ret.buf();
}


const char* File::getCurrentPath()
{
    mDeclStaticString( ret );

#ifndef OD_NO_QT
    ret = QDir::currentPath();
#else
    ret.setMinBufSize( 1024 );
# ifdef __win__
    _getcwd( ret.buf(), ret.minBufSize() );
# else
    getcwd( ret.getCStr(), ret.minBufSize() );
# endif
#endif
    return ret.buf();
}


const char* File::getHomePath()
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    ret = QDir::homePath();
#else
    pFreeFnErrMsg(not_implemented_str);
    ret = GetEnvVar( "HOME" );
#endif
    return ret.buf();
}


const char* File::getTempPath()
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    ret = QDir::tempPath();
# ifdef __win__
    ret.replace( '/', '\\' );
# endif
#else
    pFreeFnErrMsg(not_implemented_str);
# ifndef __win__
    ret = "/tmp";
# else
    ret = "C:\\TEMP";
# endif
#endif
    return ret.buf();
}


const char* File::getRootPath( const char* path )
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    const QDir qdir( path );
    ret = qdir.rootPath();
#else
    pFreeFnErrMsg(not_implemented_str);
# ifdef __win__
    ret = "/";
# else
    ret = "C:\\";
# endif
#endif
    return ret.buf();
}


bool File::launchViewer( const char* fnm, const ViewPars& vp )
{
    if ( !exists(fnm) )
	return false;

    OS::MachineCommand mc( "od_FileBrowser" );
    mc.addKeyedArg( ViewPars::sKeyFile(), fnm );
    mc.addKeyedArg( ViewPars::sKeyMaxLines(), vp.maxnrlines_ );
    mc.addKeyedArg( ViewPars::sKeyStyle(), toString(vp.style_) );
    if ( vp.editable_ )
	mc.addFlag( ViewPars::sKeyEdit() );

#ifdef __mac__
    mc.addFlag( OS::MachineCommand::sKeyFG() );
#endif

    OS::CommandLauncher cl( mc );
    OS::CommandExecPars pars;
    pars.launchtype_ = OS::RunInBG;
    return cl.execute( pars );
}
