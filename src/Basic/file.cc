/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	File utitlities
 RCS:		$Id: file.cc,v 1.2 2010-02-17 12:38:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "file.h"
#include "bufstring.h"

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
    return qfi.isDir();
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
    BufferString lnknm( linknm );
    lnknm += ".lnk";
    return QFile::link( fnm, lnknm.buf() );
#else
    return QFile::link( fnm, linknm );
#endif

}


bool copyFile( const char* from, const char* to )
{ return QFile::copy( from, to ); }


bool copyDir( const char* from, const char* to )
{
    // TODO
    return false;
}


bool removeFile( const char* fnm )
{ return QFile::remove( fnm ); }


bool removeDir( const char* dirnm )
{
    // TODO remove directories
    return false;
}


bool makeWritable( const char* fnm, bool yn, bool recursive )
{
    // TODO
    return false;
}


bool setPermissions( const char* fnm, const char* perms, bool recursive )
{
    // TODO
    return false;
}


int getKbSize( const char* fnm )
{
    QFileInfo qfi( fnm );
    od_int64 kbsz = qfi.size() / 1024;
    return kbsz;
}


int getFreeMBytes( const char* fnm )
{
    // TODO
    return -1;
}

static const char* sKeyTimeFormat = "ddd dd MMM yyyy , hh:mm:ss";

const char* timeCreated( const char* fnm )
{
    static BufferString timestr;
    QFileInfo qfi( fnm );
    QString qstr = qfi.created().toString( sKeyTimeFormat );
    timestr = qstr.toAscii().constData();
    return timestr.buf();
}


const char* timeLastModified( const char* fnm )
{
    static BufferString timestr;
    QFileInfo qfi( fnm );
    QString qstr = qfi.lastModified().toString( sKeyTimeFormat );
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
    linkstr = qfi.symLinkTarget().toAscii().constData();
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

} // namespace File
