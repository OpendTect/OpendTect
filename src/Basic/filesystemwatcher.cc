/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "filesystemwatcher.h"
#include "qfilesystemcomm.h"
#include "bufstringset.h"


FileSystemWatcher::FileSystemWatcher()
    : directoryChanged(this)
    , fileChanged(this)
{
    qfswatcher_ = new QFileSystemWatcher;
    qfswatchercomm_ = new QFileSystemWComm( qfswatcher_, this );
}


FileSystemWatcher::~FileSystemWatcher()
{
    delete qfswatchercomm_;
    delete qfswatcher_;
}


void FileSystemWatcher::addFile( const char* fnm )
{
    qfswatcher_->addPath( fnm );
}


void FileSystemWatcher::addFiles( const BufferStringSet& fnms )
{
    for ( int idx=0; idx<fnms.size(); idx++ )
	addFile( fnms.get(idx) );
}


void FileSystemWatcher::removeFile( const char* fnm )
{
    qfswatcher_->removePath( fnm );
}


void FileSystemWatcher::removeFiles( const BufferStringSet& fnms )
{
    for ( int idx=0; idx<fnms.size(); idx++ )
	removeFile( fnms.get(idx) );
}


FileSystemWatcher& FSW()
{
    static FileSystemWatcher* fsw = nullptr;
    if ( !fsw ) fsw = new FileSystemWatcher;
    return *fsw;
}
