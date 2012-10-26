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
#include "staticstring.h"
#include "winutils.h"
#include "errh.h"
#include "executor.h"
#include "ptrman.h"
#include "strmprov.h"
#include "strmoper.h"

#ifdef __win__
# include <direct.h>
#else
# include <unistd.h>
#endif

#ifndef OD_NO_QT
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#else
#include <sys/stat.h>
#include <fstream>
#endif

const char* not_implemented_str = "Not implemented";


namespace File
{

class RecursiveCopier : public Executor
{
public:
    			RecursiveCopier(const char* from,const char* to)
			    : Executor("Copying Directory")
			    , src_(from),dest_(to),fileidx_(0)
			    , msg_("Copying files")
			{ makeFileList(src_); }

    od_int64		nrDone() const		{ return fileidx_; }
    od_int64		totalNr() const		{ return filelist_.size(); }
    const char*		message() const		{ return msg_; }
    const char*		nrDoneText() const	{ return "Files copied"; }

protected:

    int			nextStep();
    void		makeFileList(const char*);

    int			fileidx_;
    BufferStringSet	filelist_;
    BufferString	src_;
    BufferString	dest_;
    BufferString	msg_;

};



void RecursiveCopier::makeFileList( const char* dir )
{
    DirList files( dir, DirList::FilesOnly );
    QDir srcdir( src_.buf() );
    for ( int idx=0; idx<files.size(); idx++ )
    {
	BufferString curfile( files.fullPath(idx) );
	BufferString relpath( srcdir.relativeFilePath(curfile.buf())
						.toAscii().constData() );
	filelist_.add( relpath );
    }

    DirList dirs( dir, DirList::DirsOnly );
    for ( int idx=0; idx<dirs.size(); idx++ )
    {
	BufferString curdir( dirs.fullPath(idx) );
	BufferString relpath( srcdir.relativeFilePath(curdir.buf())
						.toAscii().constData() );
	filelist_.add( relpath );
	if ( !File::isLink(curdir) )
	    makeFileList( curdir );
    }
}


#define mErrRet(s1,s2) { msg_ = s1; msg_ += s2; return ErrorOccurred(); }
int RecursiveCopier::nextStep()
{
    if ( fileidx_ >= filelist_.size() )
	return Finished();

    if ( !fileidx_ )
    {
	if ( File::exists(dest_) && !File::remove(dest_) )
	    mErrRet("Cannot overwrite ",dest_)
		
	if ( !File::createDir(dest_) )
	    mErrRet("Cannot create directory ",dest_)
    }

    FilePath srcfile( src_, filelist_.get(fileidx_) );
    FilePath destfile( dest_, filelist_.get(fileidx_) );
    if ( File::isLink(srcfile.fullPath()) )
    {
	BufferString linkval = linkValue( srcfile.fullPath() );
	if ( !createLink(linkval,destfile.fullPath()) )
	    mErrRet("Cannot create symbolic link ",destfile.fullPath())
    }
    else if ( isDirectory(srcfile.fullPath()) )
    {
	if ( !File::createDir(destfile.fullPath()) )
	    mErrRet("Cannot create directory ",destfile.fullPath())
    }
    else if ( !File::copy(srcfile.fullPath(),destfile.fullPath()) )
	mErrRet("Cannot create file ", destfile.fullPath())

    fileidx_++;
    return MoreToDo();
}


Executor* getRecursiveCopier( const char* from, const char* to )
{ return new RecursiveCopier( from, to ); }


od_int64 getFileSize( const char* fnm )
{
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
#ifndef OD_NO_QT
    return QFile::exists( fnm );
#else
    std::ifstream strm;
    strm.open( fnm );
    return strm.is_open();
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
#ifndef OD_NO_QT
    static StaticStringManager stm;
    BufferString& pathstr = stm.getString();
    QDir qdir( dir );
    pathstr = qdir.canonicalPath().toAscii().constData();
    return pathstr;
#else
    pFreeFnErrMsg(not_implemented_str,"getCanonicalPath");
    return 0;
#endif
}


const char* getRelativePath( const char* reltodir, const char* fnm )
{
#ifndef OD_NO_QT
    BufferString reltopath = getCanonicalPath( reltodir );
    BufferString path = getCanonicalPath( fnm );
    static StaticStringManager stm;
    BufferString& relpathstr = stm.getString();
    QDir qdir( reltopath.buf() );
    relpathstr = qdir.relativeFilePath( path.buf() ).toAscii().constData();
    return relpathstr.isEmpty() ? fnm : relpathstr.buf();
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
    if ( !strstr(linknm,".lnk")  )
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


bool copy( const char* from, const char* to )
{
#ifndef OD_NO_QT

    if ( isDirectory(from) || isDirectory(to)  )
	return copyDir( from, to );

    if ( exists(to) && !isDirectory(to) )
	File::remove( to );

    return QFile::copy( from, to );

#else
    pFreeFnErrMsg(not_implemented_str,"copy");
    return false;
#endif
}


bool move( const char* from, const char* to )
{
#ifdef __win__
    return winMove( from, to, isFile(from) );
#endif
    return true;
}


bool copyDir( const char* from, const char* to )
{
    if ( !from || !exists(from) || !to || !*to || exists(to) )
	return false;

#ifndef OD_NO_QT
    PtrMan<Executor> copier = getRecursiveCopier( from, to );
    const bool res = copier->execute();
#else

    BufferString cmd;
#ifdef __win__
    cmd = "xcopy /E /I /Q /H ";
    cmd.add(" \"").add(from).add("\" \"").add(to).add("\"");
#else
    cmd = "/bin/cp -r ";
    cmd.add(" '").add(from).add("' '").add(to).add("'");
#endif

    bool res = system( cmd ) != -1;
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
    {
	// TODO
    }

    cmd.add(" '").add(dirnm).add("'");
    bool res = system( cmd ) != -1;
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


bool getCurWorkDir( char* dir, int pathlen )
{
#ifdef __win__
    _getcwd( dir, pathlen );
    if ( !dir || !*dir )
	return false;
    else
	return true;
#else
    getcwd( dir, pathlen );
    if ( !dir || !*dir )
	return false;
    else
	return true;
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
    cmd.add(yn ? " ug+w '" : " a-w '").add(fnm).add("'");
#endif

    return system( cmd ) != -1;
}


bool makeExecutable( const char* fnm, bool yn )
{
#ifdef __win__
    return true;
#else
    BufferString cmd( "chmod" );
    cmd.add(yn ? " +r+x '" : " -x '").add(fnm).add("'");
    return system( cmd ) != -1;
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
    cmd.add( perms ).add( " " ).add( fnm );
    return system( cmd ) != -1;
#endif
}


bool getContent( const char* fnm, BufferString& bs )
{
    bs.setEmpty();
    if ( !fnm || !*fnm ) return false;

    StreamData sd( StreamProvider(fnm).makeIStream() );
    bool rv = true;
    if ( sd.usable() )
	StrmOper::readFile( *sd.istrm, bs );
    else
	rv = false;
    sd.close();
    return rv;
}


int getKbSize( const char* fnm )
{
    od_int64 kbsz = getFileSize( fnm ) / 1024;
    return kbsz;
}


const char* timeCreated( const char* fnm, const char* fmt )
{
    static StaticStringManager stm;
    BufferString& timestr = stm.getString();
#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    QString qstr = qfi.created().toString( fmt );
    timestr = qstr.toAscii().constData();
#else
    pFreeFnErrMsg(not_implemented_str,"timeCreated");
#endif
    return timestr.buf();
}


const char* timeLastModified( const char* fnm, const char* fmt )
{
    static StaticStringManager stm;
    BufferString& timestr = stm.getString();
#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    QString qstr = qfi.lastModified().toString( fmt );
    timestr = qstr.toAscii().constData();
#else
    pFreeFnErrMsg(not_implemented_str,"timeLastModified");
#endif
    return timestr.buf();
}


od_int64 getTimeInSeconds( const char* fnm )
{
#ifndef OD_NO_QT
    QFileInfo qfi( fnm );
    return qfi.lastModified().toTime_t();
#else
    struct stat st_buf;
    int status = stat(fnm, &st_buf);
    if (status != 0)
	return 0;

    return st_buf.st_mtime;
#endif
}


const char* linkValue( const char* linknm )
{
#ifdef __win__
    return linkTarget( linknm );
#else
    static StaticStringManager stm;
    BufferString& linkstr = stm.getString();
    const int len = readlink( linknm, linkstr.buf(), 256 );
    if ( len < 0 )
	return linknm;

    linkstr[len] = '\0';
    return linkstr.buf();
#endif
}


const char* linkTarget( const char* linknm )
{
#ifndef OD_NO_QT
    static StaticStringManager stm;
    BufferString& linkstr = stm.getString();
    QFileInfo qfi( linknm );
    linkstr = qfi.isSymLink() ? qfi.symLinkTarget().toAscii().constData()
			      : linknm;
    return linkstr.buf();
#else
    pFreeFnErrMsg(not_implemented_str,"linkTarget");
    return 0;
#endif
}


const char* getCurrentPath()
{
    static StaticStringManager stm;
    BufferString& pathstr = stm.getString();

#ifndef OD_NO_QT
    pathstr = QDir::currentPath().toAscii().constData();
#else
    pFreeFnErrMsg(not_implemented_str,"getCurrentPath");
#endif
    return pathstr.buf();
}


const char* getHomePath()
{
    static StaticStringManager stm;
    BufferString& pathstr = stm.getString();
#ifndef OD_NO_QT
    pathstr = QDir::homePath().toAscii().constData();
#else
    pFreeFnErrMsg(not_implemented_str,"getHomePath");
#endif
    return pathstr.buf();
}


const char* getTempPath()
{
    static StaticStringManager stm;
    BufferString& pathstr = stm.getString();
#ifndef OD_NO_QT
    pathstr = QDir::tempPath().toAscii().constData();
#ifdef __win__
    replaceCharacter( pathstr.buf(), '/', '\\' );
#endif
#else
    pFreeFnErrMsg(not_implemented_str,"getTmpPath");
#endif
    return pathstr.buf();
}


const char* getRootPath( const char* path )
{
    static StaticStringManager stm;
    BufferString& pathstr = stm.getString();
    QDir qdir( path );
    pathstr = qdir.rootPath().toAscii().constData();
    return pathstr.buf();
}

} // namespace File
