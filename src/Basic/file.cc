/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	File utitlities
 RCS:		$Id$
________________________________________________________________________

-*/

#include "file.h"
#include "filepath.h"
#include "bufstringset.h"
#include "dirlist.h"
#include "perthreadrepos.h"
#include "winutils.h"
#include "executor.h"
#include "ptrman.h"
#include "od_istream.h"
#include "oddirs.h"
#include "oscommand.h"

#ifdef __win__
# include <direct.h>
#else
#include "sys/stat.h"
# include <unistd.h>
#endif

#ifndef OD_NO_QT
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#else
#include <fstream>
#endif

#define mMBFactor (1024*1024)
const char* not_implemented_str = "Not implemented";


namespace File
{

class RecursiveCopier : public Executor
{
public:
			RecursiveCopier(const char* from,const char* to)
			    : Executor("Copying Directory")
			    , src_(from),dest_(to),fileidx_(0)
			    , totalnr_(0),nrdone_(0)
			    , msg_("Copying files")
			{
			    makeRecursiveFileList(src_,filelist_,false);
			    for ( int idx=0; idx<filelist_.size(); idx++ )
				totalnr_ += getFileSize( filelist_.get(idx) );
			}

    od_int64		nrDone() const		{ return nrdone_ / mMBFactor; }
    od_int64		totalNr() const		{ return totalnr_ / mMBFactor; }
    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const	{ return "MBytes copied"; }

protected:

    int			nextStep();
    void		makeFileList(const char*);

    int			fileidx_;
    od_int64		totalnr_;
    od_int64		nrdone_;
    BufferStringSet	filelist_;
    BufferString	src_;
    BufferString	dest_;
    BufferString	msg_;

};


#define mErrRet(s1,s2) { msg_ = s1; msg_ += s2; return ErrorOccurred(); }
int RecursiveCopier::nextStep()
{
    if ( fileidx_ >= filelist_.size() )
	return Finished();

    if ( !fileidx_ )
    {
	if ( File::exists(dest_) && !File::remove(dest_) )
	    mErrRet("Cannot overwrite ",dest_)
	if( !File::createDir(dest_) )
	    mErrRet("Cannot create directory ",dest_)
    }

    const BufferString& srcfile = *filelist_[fileidx_];
    QDir srcdir( src_.buf() );
    BufferString relpath( srcdir.relativeFilePath(srcfile.buf()) );
    const BufferString destfile = FilePath(dest_,relpath).fullPath();
    if ( File::isLink(srcfile) )
    {
	BufferString linkval = linkValue( srcfile );
	if ( !createLink(linkval,destfile) )
	    mErrRet("Cannot create symbolic link ",destfile)
    }
    else if ( isDirectory(srcfile) )
    {
	if ( !File::createDir(destfile) )
	    mErrRet("Cannot create directory ",destfile)
    }
    else if ( !File::copy(srcfile,destfile) )
	    mErrRet("Cannot create file ", destfile)

    fileidx_++;
    nrdone_ += getFileSize( srcfile );
    return MoreToDo();
}


Executor* getRecursiveCopier( const char* from, const char* to )
{ return new RecursiveCopier( from, to ); }


void makeRecursiveFileList( const char* dir, BufferStringSet& filelist,
			    bool followlinks )
{
    DirList files( dir, DirList::FilesOnly );
    for ( int idx=0; idx<files.size(); idx++ )
    {
	if ( !followlinks )
	    filelist.add( files.fullPath(idx) );
	else
	    filelist.addIfNew( files.fullPath(idx) );
    }

    DirList dirs( dir, DirList::DirsOnly );
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
    if ( !fnm )
	return false;

#ifndef OD_NO_QT
    return (*fnm == '@' && *(fnm+1)) || QFile::exists( fnm );
    // support, like od_istream, commands. These start with '@'.
#else
    return od_istream(fnm).isOK();
#endif
}


bool isEmpty( const char* fnm )
{
    return getFileSize( fnm ) < 1;
}


bool isFile( const char* fnm )
{
#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
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


const char* getCanonicalPath( const char* dir )
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    const QDir qdir( dir );
    ret = qdir.canonicalPath();
#else
    pFreeFnErrMsg(not_implemented_str,"getCanonicalPath");
    ret = dir;
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
    pFreeFnErrMsg(not_implemented_str,"getRelativePath");
    return fnm;
#endif
}


bool isLink( const char* fnm )
{
#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    return qfi.isSymLink();
#else
    pFreeFnErrMsg(not_implemented_str,"isLink");
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
    pFreeFnErrMsg(not_implemented_str,"isHidden");
    return false;
#endif
}


bool isWritable( const char* fnm )
{
#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    return qfi.isWritable();
#else
    struct stat st_buf;
    int status = stat(fnm, &st_buf);
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
    if (status != 0)
	return false;

    return st_buf.st_mode & S_IXUSR;
#endif
}



bool isFileInUse( const char* fnm )
{
#ifdef __win__
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


bool createDir( const char* fnm )
{
#ifndef OD_NO_QT
    QDir qdir; return qdir.mkpath( fnm );
#else
    pFreeFnErrMsg(not_implemented_str,"createDir");
    return false;
#endif
}


bool rename( const char* oldname, const char* newname )
{
#ifndef OD_NO_QT
    return QFile::rename( oldname, newname );
#else
    pFreeFnErrMsg(not_implemented_str,"rename");
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
    pFreeFnErrMsg(not_implemented_str,"createLink");
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

    return ret;

#else
    pFreeFnErrMsg(not_implemented_str,"copy");
    return false;
#endif
}


bool copyDir( const char* from, const char* to, BufferString* errmsg )
{
    if ( !from || !exists(from) || !to || !*to || exists(to) )
	return false;

#ifndef OD_NO_QT
    PtrMan<Executor> copier = getRecursiveCopier( from, to );
    const bool res = copier->execute();
    if ( !res && errmsg )
	errmsg->add( copier->uiMessage().getFullString() );
#else

    BufferString cmd;
#ifdef __win__
    cmd = "xcopy /E /I /Q /H ";
    cmd.add(" \"").add(from).add("\" \"").add(to).add("\"");
#else
    cmd = "/bin/cp -r ";
    cmd.add(" '").add(from).add("' '").add(to).add("'");
#endif

    bool res = !system( cmd );
    if ( res ) res = exists( to );
#endif
    return res;
}


bool remove( const char* fnm )
{
#ifndef OD_NO_QT
    return isFile(fnm) ? QFile::remove( fnm ) : removeDir( fnm );
#else
    pFreeFnErrMsg(not_implemented_str,"remove");
    return false;
#endif
}


bool removeDir( const char* dirnm )
{
    if ( !exists(dirnm) )
	return true;

#ifdef __win__
    return winRemoveDir( dirnm );
#else
    BufferString cmd;
    cmd = "/bin/rm -rf";
    if ( isLink(dirnm) )
	return QFile::remove( dirnm );	

    cmd.add(" \"").add(dirnm).add("\"");
    bool res = QProcess::execute( QString(cmd.buf()) ) >= 0;
    if ( res ) res = !exists(dirnm);
    return res;
#endif
}


bool changeDir( const char* dir )
{
#ifdef __win__
    return _chdir( dir );
#else
    return chdir( dir );
#endif
}


bool makeWritable( const char* fnm, bool yn, bool recursive )
{
    BufferString cmd;
#ifdef __win__
    cmd = "attrib"; cmd += yn ? " -R " : " +R ";
    cmd.add("\"").add(fnm).add("\"");
    if ( recursive && isDirectory(fnm) )
	cmd += "\\*.* /S ";
#else
    cmd = "chmod";
    if ( recursive && isDirectory(fnm) )
	cmd += " -R ";
    cmd.add(yn ? " ug+w \"" : " a-w \"").add(fnm).add("\"");
#endif

    return QProcess::execute( QString(cmd.buf()) ) >= 0;
}


bool makeExecutable( const char* fnm, bool yn )
{
#ifdef __win__
    return true;
#else
    BufferString cmd( "chmod" );
    cmd.add(yn ? " +r+x \"" : " -x \"").add(fnm).add("\"");
    return QProcess::execute( QString(cmd.buf()) ) >= 0;
#endif
}


bool setPermissions( const char* fnm, const char* perms, bool recursive )
{
#ifdef __win__
    return false;
#else
    BufferString cmd( "chmod " );
    if ( recursive && isDirectory(fnm) )
	cmd += " -R ";
    cmd.add( perms ).add( " \"" ).add( fnm ).add( "\"" );
    return QProcess::execute( QString(cmd.buf()) ) >= 0;
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


int getKbSize( const char* fnm )
{
    od_int64 kbsz = getFileSize( fnm ) / 1024;
    return kbsz;
}


const char* timeCreated( const char* fnm, const char* fmt )
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    ret = qfi.created().toString( fmt );
#else
    pFreeFnErrMsg(not_implemented_str,"timeCreated");
    ret = "<unknown>";
#endif
    return ret.buf();
}


const char* timeLastModified( const char* fnm, const char* fmt )
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    ret = qfi.lastModified().toString( fmt );
#else
    pFreeFnErrMsg(not_implemented_str,"timeLastModified");
    ret = "<unknown>";
#endif
    return ret.buf();
}


od_int64 getTimeInSeconds( const char* fnm, bool lastmodif )
{
#ifndef OD_NO_QT
    const QFileInfo qfi( fnm );
    return lastmodif ? qfi.lastModified().toTime_t() : qfi.created().toTime_t();
#else
    struct stat st_buf;
    int status = stat(fnm, &st_buf);
    if (status != 0)
	return 0;

    return lastmodif ? st_buf.st_mtime : st_buf.st_ctim;
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
    pFreeFnErrMsg(not_implemented_str,"linkTarget");
    ret = linknm;
#endif
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
    getcwd( ret.buf(), ret.minBufSize() );
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
    pFreeFnErrMsg(not_implemented_str,"getHomePath");
    ret = GetEnvVar( "HOME" );
#endif
    return ret.buf();
}


const char* getTempPath()
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    ret = QDir::tempPath();
# ifdef __win__
    ret.replace( '/', '\\' );
# endif
#else
    pFreeFnErrMsg(not_implemented_str,"getTmpPath");
# ifdef __win__
    ret = "/tmp";
# else
    ret = "C:\\TEMP";
# endif
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
    pFreeFnErrMsg(not_implemented_str,"getRootPath");
# ifdef __win__
    ret = "/";
# else
    ret = "C:\\";
# endif
#endif
    return ret.buf();
}


bool launchViewer( const char* fnm, const ViewPars& vp )
{
    if ( !exists(fnm) )
	return false;

    BufferString cmd( FilePath(GetBinPlfDir(),"od_FileBrowser").fullPath() );
    if ( vp.style_ != Text )
	cmd.add( " --style ")
	   .add( vp.style_ == File::Table	? "table"
	      : (vp.style_ == File::Log		? "log" : "bin") );
    if ( vp.editable_ )
	cmd.add( " --edit" );
    cmd.add( " --maxlines " ).add( vp.maxnrlines_ );
    cmd.add( " " ).add(" \" ").add( fnm ).add(" \" ");

    OS::CommandLauncher cl = OS::MachineCommand( cmd );
    return cl.execute();
}

} // namespace File
