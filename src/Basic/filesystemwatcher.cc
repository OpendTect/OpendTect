/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "filesystemwatcher.h"
#ifndef OD_NO_QT
#include "qfilesystemcomm.h"
#endif
#include "bufstringset.h"

mUseQtnamespace

FileSystemWatcher::FileSystemWatcher()
    : directoryChanged(this)
    , fileChanged(this)
{
#ifndef OD_NO_QT
    qfswatcher_ = new QFileSystemWatcher;
    qfswatchercomm_ = new QFileSystemWComm( qfswatcher_, this );
#else
    qfswatcher_ = 0;
    qfswatchercomm_ = 0;
#endif
}


FileSystemWatcher::~FileSystemWatcher()
{
#ifndef OD_NO_QT
    delete qfswatchercomm_;
    delete qfswatcher_;
#endif
}


void FileSystemWatcher::addFile( const BufferString& fnm )
{
#ifndef OD_NO_QT
    qfswatcher_->addPath( fnm.buf() );
#endif
}


void FileSystemWatcher::addFiles( const BufferStringSet& fnms )
{
    for ( int idx=0; idx<fnms.size(); idx++ )
	addFile( fnms.get(idx) );
}


void FileSystemWatcher::removeFile( const BufferString& fnm )
{
#ifndef OD_NO_QT
    qfswatcher_->removePath( fnm.buf() );
#endif
}


void FileSystemWatcher::removeFiles( const BufferStringSet& fnms )
{
    for ( int idx=0; idx<fnms.size(); idx++ )
	removeFile( fnms.get(idx) );
}


FileSystemWatcher& FSW()
{
    static FileSystemWatcher* fsw = 0;
    if ( !fsw ) fsw = new FileSystemWatcher;
    return *fsw;
}
