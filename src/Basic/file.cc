/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	File utitlities
 RCS:		$Id: file.cc,v 1.21 2010-11-19 03:59:09 cvsnanne Exp $
________________________________________________________________________

-*/

#include "file.h"
#include "bufstring.h"
#include "winutils.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>


namespace File
{

bool exists( const char* fnm )
{ return QFile::exists( fnm ); }


bool isEmpty( const char* fnm )
{
    QFileInfo qfi( fnm );
    return qfi.size() < 1;
}


bool isFile( const char* fnm )
{
    QFileInfo qfi( fnm );
    return qfi.isFile();
}


bool isDirectory( const char* fnm )
{
    QFileInfo qfi( fnm );
    if ( qfi.isDir() )
	return true;

    BufferString lnkfnm( fnm, ".lnk" );
    qfi.setFile( lnkfnm.buf() );
    return qfi.isDir();
}


const char* getCanonicalPath( const char* dir )
{
    static BufferString pathstr;
    QDir qdir( dir );
    pathstr = qdir.canonicalPath().toAscii().constData();
    return pathstr;
}


const char* getRelativePath( const char* reltodir, const char* fnm )
{
    BufferString reltopath = getCanonicalPath( reltodir );
    BufferString path = getCanonicalPath( fnm );
    static BufferString relpathstr;
    QDir qdir( reltopath.buf() );
    relpathstr = qdir.relativeFilePath( path.buf() ).toAscii().constData();
    return relpathstr;
}


bool isLink( const char* fnm )
{
    QFileInfo qfi( fnm );
    return qfi.isSymLink();
}


bool isWritable( const char* fnm )
{
    QFileInfo qfi( fnm );
    return qfi.isWritable();
}


bool createDir( const char* fnm )
{ QDir qdir; return qdir.mkpath( fnm ); }


bool rename( const char* oldname, const char* newname )
{ return QFile::rename( oldname, newname ); }


bool createLink( const char* fnm, const char* linknm )
{ 
#ifdef __win__
    BufferString winlinknm( linknm );
    if ( !strstr(linknm,".lnk")  )
	winlinknm += ".lnk";
    return QFile::link( fnm, winlinknm.buf() );
#else
    return QFile::link( fnm, linknm );
#endif

}


bool saveCopy( const char* from, const char* to )
{
    if ( isDirectory(from) ) return false;
    if ( !exists(to) )
	return QFile::copy( from, to );
    
    const BufferString tmpfnm( to, ".tmp" );
    if ( !File::rename(to,tmpfnm) )
	return false;

    const bool res = QFile::copy( from, to );
    res ? File::remove( tmpfnm ) : File::rename( tmpfnm, to );
    return res;
}


bool copy( const char* from, const char* to )
{
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
{ return isFile(fnm) ? QFile::remove( fnm ) : removeDir( fnm ); }


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
    QFileInfo qfi( fnm );
    od_int64 kbsz = qfi.size() / 1024;
    return kbsz;
}


const char* timeCreated( const char* fnm, const char* fmt )
{
    static BufferString timestr;
    QFileInfo qfi( fnm );
    QString qstr = qfi.created().toString( fmt );
    timestr = qstr.toAscii().constData();
    return timestr.buf();
}


const char* timeLastModified( const char* fnm, const char* fmt )
{
    static BufferString timestr;
    QFileInfo qfi( fnm );
    QString qstr = qfi.lastModified().toString( fmt );
    timestr = qstr.toAscii().constData();
    return timestr.buf();
}


od_int64 getTimeInSeconds( const char* fnm )
{
    QFileInfo qfi( fnm );
    return qfi.lastModified().toTime_t();
}


const char* linkTarget( const char* linknm )
{
    static BufferString linkstr;
    QFileInfo qfi( linknm );
    linkstr = qfi.isSymLink() ? qfi.symLinkTarget().toAscii().constData()
			      : linknm;
    return linkstr.buf();
}


const char* getCurrentPath()
{
    static BufferString pathstr;
    pathstr = QDir::currentPath().toAscii().constData();
    return pathstr.buf();
}


const char* getHomePath()
{
    static BufferString pathstr;
    pathstr = QDir::homePath().toAscii().constData();
    return pathstr.buf();
}


const char* getTempPath()
{
    static BufferString pathstr;
    pathstr = QDir::tempPath().toAscii().constData();
    return pathstr.buf();
}

} // namespace File
