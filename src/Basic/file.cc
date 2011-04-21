/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	File utitlities
 RCS:		$Id: file.cc,v 1.28 2011-04-21 13:09:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "file.h"
#include "bufstring.h"
#include "staticstring.h"
#include "winutils.h"
#include "errh.h"

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
    BufferString& pathstr = StaticStringManager::STM().getString();
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
    BufferString& relpathstr = StaticStringManager::STM().getString();
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
    return 0;
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
#ifdef __win__
    if ( isDirectory(from) || getKbSize(from) > 1024 )
	return winCopy( from, to, isFile(from) );
#endif

    if ( !isFile(from) )
	return copyDir( from, to );

    if ( !exists(to) )
	return QFile::copy( from, to );

    File::remove( to );
    return QFile::copy( from, to );

#else
    pFreeFnErrMsg(not_implemented_str,"copy");
    return false;
#endif
}


bool copyDir( const char* from, const char* to )
{
    if ( !from || !exists(from) || !to || !*to || exists(to) )
	return false;

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

    BufferString cmd;
#ifdef __win__
    cmd = "rd /Q /S";
    cmd.add(" \"").add(dirnm).add("\"");
#else
    cmd = "/bin/rm -rf";
    if ( isLink(dirnm) )
    {
	// TODO
    }

    cmd.add(" '").add(dirnm).add("'");
#endif

    bool res = system( cmd ) != -1;
    if ( res ) res = !exists(dirnm);
    return res;
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


int getKbSize( const char* fnm )
{
    od_int64 kbsz = getFileSize( fnm ) / 1024;
    return kbsz;
}


const char* timeCreated( const char* fnm, const char* fmt )
{
    BufferString& timestr = StaticStringManager::STM().getString();
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
    BufferString& timestr = StaticStringManager::STM().getString();
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


const char* linkTarget( const char* linknm )
{
#ifndef OD_NO_QT
    BufferString& linkstr = StaticStringManager::STM().getString();
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
    BufferString& pathstr = StaticStringManager::STM().getString();

#ifndef OD_NO_QT
    pathstr = QDir::currentPath().toAscii().constData();
#else
    pFreeFnErrMsg(not_implemented_str,"getCurrentPath");
#endif
    return pathstr.buf();
}


const char* getHomePath()
{
    BufferString& pathstr = StaticStringManager::STM().getString();
#ifndef OD_NO_QT
    pathstr = QDir::homePath().toAscii().constData();
#else
    pFreeFnErrMsg(not_implemented_str,"getHomePath");
#endif
    return pathstr.buf();
}


const char* getTempPath()
{
    BufferString& pathstr = StaticStringManager::STM().getString();
#ifndef OD_NO_QT
    pathstr = QDir::tempPath().toAscii().constData();
#else
    pFreeFnErrMsg(not_implemented_str,"getTmpPath");
#endif
    return pathstr.buf();
}


const char* getRootPath( const char* path )
{
    BufferString& pathstr = StaticStringManager::STM().getString();
    QDir qdir( path );
    pathstr = qdir.rootPath().toAscii().constData();
    return pathstr.buf();
}

} // namespace File
