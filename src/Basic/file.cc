/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	File utitlities
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
#include "perthreadrepos.h"
#include "ptrman.h"
#include "pythonaccess.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "oscommand.h"
#include "uistrings.h"

#ifdef __win__
#include "winutils.h"
# include <direct.h>
#else
# include "sys/stat.h"
# include <unistd.h>
# include <utime.h>
#endif

#ifndef OD_NO_QT
# include <QDateTime>
# include <QDir>
# include <QFile>
# include <QFileInfo>
# include <QStandardPaths>
#else
# include <fstream>
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


namespace File
{

class RecursiveCopier : public Executor
{ mODTextTranslationClass(RecursiveCopier);
public:
			RecursiveCopier(const char* from,const char* to)
			    : Executor("Copying Folder")
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
    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const	{ return tr("MBytes copied"); }

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


#define mErrRet(s1) { msg_ = s1; return ErrorOccurred(); }
int RecursiveCopier::nextStep()
{
#ifdef OD_NO_QT
    return ErrorOccurred();
#else
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
#endif
}


class RecursiveDeleter : public Executor
{
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
			    if ( !externallist )
			    {
				filelist_.add( dirname_ );
				makeRecursiveFileList( dirname_, filelist_  );
			    }
			    else
				filelist_ = *externallist;

			    totalnr_ = filelist_.size();
			}

    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }
    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const
			{ return mToUiStringTodo( "Files removed" ); }

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
{ return new RecursiveCopier( from, to ); }

Executor* getRecursiveDeleter( const char* dirname,
			       const BufferStringSet* externallist,
			       bool filesonly )
{ return new RecursiveDeleter( dirname, externallist, filesonly ); }


void makeRecursiveFileList( const char* dir, BufferStringSet& filelist,
			    bool followlinks )
{
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


od_int64 getFileSize( const char* fnm, bool followlink )
{
    if ( !followlink && isLink(fnm) )
    {
	od_int64 filesize = 0;
#ifdef __win__
	HANDLE file = CreateFile ( fnm, GENERIC_READ, 0, NULL, OPEN_EXISTING,
				   FILE_ATTRIBUTE_NORMAL, NULL );
	filesize = GetFileSize( file, NULL );
	CloseHandle( file );
#else
	struct stat filestat;
	filesize = lstat( fnm, &filestat )>=0 ? filestat.st_size : 0;
#endif

	return filesize;
    }

#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    return qfi.size();
#else
    struct stat st_buf;
    int status = stat(fnm, &st_buf);
    if (status != 0)
	return 0;

    return st_buf.st_size;
#endif
}

bool exists( const char* fnm )
{
    if ( !fnm || !*fnm )
	return false;



#ifndef OD_NO_QT
    return QFile::exists( fnm );
#else
    return isReadable(fnm);
#endif
}


bool isEmpty( const char* fnm )
{
    return getFileSize( fnm ) < 1;
}


bool isDirEmpty( const char* dirnm )
{
#ifndef OD_NO_QT
    const QDir qdir( dirnm );
    return qdir.entryInfoList(QDir::NoDotAndDotDot|
			      QDir::AllEntries).count() == 0;
#else
    return false;
#endif
}


bool isFile( const char* fnm )
{
#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    return qfi.isFile();
#else
    struct stat st_buf;
    int status = stat(fnm, &st_buf);
    if (status != 0)
	return false;

    return S_ISREG (st_buf.st_mode);
#endif
}


bool isDirectory( const char* fnm )
{
#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    if ( qfi.isDir() )
	return true;

    BufferString lnkfnm( fnm, ".lnk" );
    qfi.setFile( lnkfnm.buf() );
    return qfi.isDir();
#else
    struct stat st_buf;
    int status = stat(fnm, &st_buf);
    if (status != 0)
	return false;

    if ( S_ISDIR (st_buf.st_mode) )
	return true;

    BufferString lnkfnm( fnm, ".lnk" );
    status = stat ( lnkfnm.buf(), &st_buf);
    if (status != 0)
	return false;

    return S_ISDIR (st_buf.st_mode);
#endif
}


BufferString findExecutable( const char* exenm, const BufferStringSet& paths,
			     bool includesyspath )
{
#ifndef OD_NO_QT
    BufferString ret;
    if ( includesyspath )
	ret = QStandardPaths::findExecutable( exenm, QStringList() );

    if ( !paths.isEmpty() )
    {
	QStringList qpaths;
	for ( int idx=0; idx<paths.size(); idx++ )
	    qpaths.append( (const char*) paths.get( idx ) );

	const BufferString tmp = QStandardPaths::findExecutable( exenm,
								 qpaths );
	if ( !tmp.isEmpty() )
	    ret = tmp;
    }
    return ret;
#else
    return BufferString::empty();
#endif
}

const char* getCanonicalPath( const char* dir )
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


const char* getAbsolutePath( const char* dir, const char* relfnm )
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    const QDir qdir( dir );
    ret = qdir.absoluteFilePath( relfnm );
#else
    pFreeFnErrMsg(not_implemented_str);
    ret = relfnm;
#endif
    return ret.buf();
}


const char* getRelativePath( const char* reltodir, const char* fnm )
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


bool isLink( const char* fnm )
{
#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    return qfi.isSymLink();
#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif
}


void hide( const char* fnm, bool yn )
{
    if ( !exists(fnm) ) return;

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


bool isHidden( const char* fnm )
{
#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    return qfi.isHidden();
#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif
}


bool isReadable( const char* fnm )
{
#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    return qfi.isReadable();
#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif
}


bool isWritable( const char* fnm )
{
#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    const bool iswritable = qfi.isWritable();
# ifdef __unix__
    return iswritable;
# else
    if ( !iswritable || WinUtils::IsUserAnAdmin() )
	return iswritable;

    return WinUtils::pathContainsTrustedInstaller(fnm)
	    ? false : iswritable;

# endif
#else
    struct stat st_buf;
    int status = stat( fnm, &st_buf );
    if (status != 0)
	return false;

    return st_buf.st_mode & S_IWUSR;
#endif
}


bool isExecutable( const char* fnm )
{
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
    return WinUtils::isFileInUse( fnm );
#else
    return false;
#endif
}


bool createDir( const char* fnm )
{
#ifndef OD_NO_QT
    QDir qdir; return qdir.mkpath( fnm );
#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif
}


bool listDir( const char* dirnm, DirListType dlt, BufferStringSet& fnames,
	      const char* mask )
{
    if ( !isDirectory(dirnm) )
	return false;

    QDir qdir( dirnm );
    if ( mask && *mask )
    {
	QStringList filters;
	filters << mask;
	qdir.setNameFilters( filters );
    }

    QDir::Filters dirfilters;
    if ( dlt == FilesInDir )
	dirfilters = QDir::Files | QDir::Hidden;
    else if ( dlt == DirsInDir )
	dirfilters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden;
    else
	dirfilters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files
				| QDir::Hidden;

    const QStringList qlist = qdir.entryList( dirfilters );
    for ( int idx=0; idx<qlist.size(); idx++ )
	fnames.add( qlist[idx] );

    fnames.sort();
    return true;
}


bool rename( const char* oldname, const char* newname, uiString* errmsg )
{
#ifndef OD_NO_QT
    if ( !File::exists(oldname) )
    {
	if ( errmsg )
	    errmsg->append( uiStrings::phrDoesntExist(::toUiString(oldname)) );
	return false;
    }

    const FilePath destpath( newname );
    if ( destpath.exists() )
    {
	if ( errmsg )
	{
	    BufferString errstr( "Destination '" );
	    errstr.add( destpath.fullPath() )
		.add( "' already exists." )
		.add( "Please remove or rename manually." );
	    errmsg->append( errstr );
	}
	return false;
    }

    const FilePath destdir( destpath.pathOnly() );
    const BufferString targetbasedir( destdir.fullPath() );
    if ( !File::exists(targetbasedir) )
    {
	if ( !File::createDir(targetbasedir) )
	{
	    if ( errmsg )
		errmsg->append( uiStrings::phrCannotCreateDirectory(
						toUiString(targetbasedir)) );
	    return false;
	}
    }
#ifdef __unix__
    else if ( !File::isWritable(targetbasedir) )
    {
	if ( errmsg )
	    errmsg->append( uiStrings::phrCannotWrite(
						toUiString(targetbasedir)) );
	return false;
    }
#endif

    int itatr=0;
    int nritatr = 10;
    bool res=false;
    while ( ++itatr < nritatr )
    {
	res = QFile::rename( oldname, newname );
	if ( res )
	    return true;
	else if ( !File::exists(oldname) && File::exists(newname) )
	{// False negative detection.
	    return true;
	}

	if ( !res && File::isDirectory(oldname) )
	{
	    QDir dir;
	    res = dir.rename( oldname, newname );
	    if ( res )
		return true;
	    else if ( !File::exists(oldname) && File::exists(newname) )
		return true;
	    else
	    {
		const FilePath sourcefp( oldname );
		dir.setCurrent( QString(sourcefp.pathOnly()) );
		const QString newnm = dir.relativeFilePath( newname );
		res = dir.rename( QString(sourcefp.fileName()), newnm );
		if ( res )
		    return true;
		else if ( !File::exists(oldname) && File::exists(newname) )
		    return true;
	    }
	}

	Threads::sleep( 0.1 );
    }

    if ( !res )
    { //Trying c rename function
	rename( oldname, newname );
	if ( !File::exists(oldname) && File::exists(newname) )
	    return true;
    }

    OS::MachineCommand mc;
#ifdef __win__
    const BufferString destdrive = destdir.rootPath();
    const FilePath sourcefp( oldname );
    const BufferString sourcedrive = sourcefp.rootPath();
    if ( destdrive != sourcedrive )
    {
	BufferString msgstr;
	res = File::copyDir( oldname, newname, &msgstr );
	if ( !res )
	{
	    if ( errmsg && !msgstr.isEmpty() )
		errmsg->append( msgstr );
	    return false;
	}

	res = File::removeDir( oldname );
	return res;
    }

    mc.setProgram( "move" );
#else
    mc.setProgram( "mv" );
#endif
    mc.addArg( oldname ).addArg( newname );
    BufferString stdoutput, stderror;
    res = mc.execute( stdoutput, &stderror );
    if ( !res && errmsg )
    {
	if ( !stderror.isEmpty() )
	    errmsg->append( stderror, true );

	if ( !stdoutput.isEmpty() )
	    errmsg->append( stdoutput, true );

	errmsg->append( "Failed to rename using system command." );
    }

    return res;
#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif

}


bool createLink( const char* fnm, const char* linknm )
{
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


bool saveCopy( const char* from, const char* to )
{
    if ( isDirectory(from) ) return false;
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
#ifndef OD_NO_QT

    if ( isDirectory(from) || isDirectory(to)  )
	return copyDir( from, to, errmsg );

    if ( exists(to) && !isDirectory(to) )
	File::remove( to );

    QFile qfile( from );
    bool ret = qfile.copy( to );
    if ( !ret && errmsg )
	errmsg->add( qfile.errorString() );

#ifdef __unix__
    const QFileInfo qfi( qfile );
    utimbuf timestamp;
    timestamp.actime = qfi.lastRead().toSecsSinceEpoch();
    timestamp.modtime = qfi.lastModified().toSecsSinceEpoch();
    utime( to, &timestamp );
#endif

    return ret;

#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif
}


bool copyDir( const char* from, const char* to, BufferString* errmsg )
{
    if ( !from || !exists(from) || !to || !*to || exists(to) )
	return false;

    PtrMan<Executor> copier = getRecursiveCopier( from, to );
    const bool res = copier->execute();
    if ( !res && errmsg )
	errmsg->add( copier->uiMessage().getFullString() );

    return res;
}


bool resize( const char* fnm, od_int64 newsz )
{
    if ( !fnm || !*fnm )
	return true;
    else if ( newsz < 0 )
	return remove( fnm );
#ifndef OD_NO_QT
    return QFile::resize( fnm, newsz );
#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif
}


bool remove( const char* fnm )
{
    if ( !fnm || !*fnm )
	return true;
#ifndef OD_NO_QT
    return isFile(fnm) ? QFile::remove( fnm ) : removeDir( fnm );
#else
    pFreeFnErrMsg(not_implemented_str);
    return false;
#endif
}


bool removeDir( const char* dirnm )
{
#ifdef OD_NO_QT
    return false;
#else
    if ( !exists(dirnm) )
	return true;

    if ( isLink(dirnm) )
	return QFile::remove( dirnm );

    QDir qdir( dirnm );
    return qdir.removeRecursively();
#endif
}


bool changeDir( const char* dir )
{
#ifdef __win__
    return _chdir( dir )==0;
#else
    return chdir( dir )==0;
#endif
}


bool makeWritable( const char* fnm, bool yn, bool recursive )
{
#ifdef OD_NO_QT
    return false;
#else
    BufferStringSet args;
# ifdef __win__
    OS::MachineCommand mc( "ATTRIB" );
    mc.addArg( yn ? "-R" : "+R" ).addArg( fnm );
    if ( recursive && isDirectory(fnm) )
	mc.addArg( "/S" ).addArg( "/D" );
# else
    OS::MachineCommand mc( "chmod" );
    if ( recursive && isDirectory(fnm) )
	mc.addArg( "-R" );
    mc.addArg( yn ? "ug+w" : "a-w" ).addArg( fnm );
# endif

    return mc.execute();
#endif
}


bool makeReadOnly(const char* fnm, bool recursive)
{
    return File::makeWritable(fnm, false, recursive);
}


bool makeExecutable( const char* fnm, bool yn )
{
#if ((defined __win__) || (defined OD_NO_QT) )
    return true;
#else
    OS::MachineCommand mc( "chmod" );
    mc.addArg( yn ? "+r+x" : "-x" ).addArg( fnm );
    return mc.execute();
#endif
}


bool setPermissions( const char* fnm, const char* perms, bool recursive )
{
#if ((defined __win__) || (defined OD_NO_QT) )
    return false;
#else
    OS::MachineCommand mc( "chmod" );
    if ( recursive && isDirectory(fnm) )
	mc.addArg( "-R" );
    mc.addArg( perms ).addArg( fnm );
    return mc.execute();
#endif
}


bool getContent( const char* fnm, BufferString& bs )
{
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
#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    if ( !fmt || !*fmt )
# if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
	ret = qfi.birthTime().toString( Qt::ISODate );
# else
	ret = qfi.created().toString( Qt::ISODate );
# endif
    else
# if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
	ret = qfi.birthTime().toString( fmt );
# else
	ret = qfi.created().toString( fmt );
# endif
#else
    pFreeFnErrMsg(not_implemented_str);
    ret = "<unknown>";
#endif
    return ret.buf();
}


const char* timeLastModified( const char* fnm, const char* fmt )
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    if ( !fmt || !*fmt )
	ret = qfi.lastModified().toString( Qt::ISODate );
    else
	ret = qfi.lastModified().toString( fmt );
#else
    pFreeFnErrMsg(not_implemented_str);
    ret = "<unknown>";
#endif
    return ret.buf();
}


od_int64 getTimeInSeconds( const char* fnm, bool lastmodif )
{
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


od_int64 getTimeInMilliSeconds( const char* fnm, bool lastmodif )
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


const char* linkValue( const char* linknm )
{
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


const char* getHomePath()
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
#ifndef OD_NO_QT
    ret = QDir::tempPath();
# ifdef __win__
    ret.replace( '/', '\\' );
# endif
#else
    pFreeFnErrMsg(not_implemented_str);
# ifdef __win__
    ret = "C:\\TEMP";
# else
    ret = "/tmp";
# endif
#endif
    return ret.buf();
}


const char* getUserAppDataPath()
{
    mDeclStaticString( ret );

#ifndef OD_NO_QT
    ret = QStandardPaths::locate( QStandardPaths::GenericDataLocation, "",
				      QStandardPaths::LocateDirectory );

#endif
    return ret.buf();
}


const char* getRootPath( const char* path )
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


const char* asciiFilesFilter()
{ return "ASCII Files (*.dat)"; }

const char* textFilesFilter()
{ return "Text Files (*.txt)"; }

const char* allFilesFilter()
{
#ifdef __win__
    return "All Files (*.*)";
#else
    return "All Files (*)";
#endif
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


#ifdef __win__
static bool canApplyScript( const char* scriptfnm )
{
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
#endif

bool initTempDir()
{
#ifdef __win__
    if ( !hasAppLocker() ||
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
                targetpath,
                "' to store temporary files."), 1);
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
                targetpath,
                "' to store temporary files."), 1);
        errmsgs.insertAt(
            new BufferString("Cannot create that directory, "
                "please check permissions"), 2);
        OD::DisplayErrorMessage( errmsgs.cat() );
        return false;
    }

    tempfnm = FilePath( targetpath,
        FilePath::getTempFileName("applocker_test", "bat") ).fullPath();
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
        errmsgs.insertAt( new BufferString(
            "AppLocker prevents executing the script '",
                   mc.program(), "'" ), 1 );
        OD::DisplayErrorMessage( errmsgs.cat() );
        return false;
    }

    return res;
#else
    return true;
#endif
}

} // namespace File
