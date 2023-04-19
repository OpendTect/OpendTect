/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "filesystemwatcher.h"
#include "bufstringset.h"
#include "ptrman.h"
#include "qfilesystemcomm.h"


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


bool FileSystemWatcher::addPath( const char* path )
{
    return qfswatcher_->addPath( path );
}


bool FileSystemWatcher::addFile( const char* fnm )
{
    return qfswatcher_->addPath( fnm );
}


void FileSystemWatcher::addFiles( const BufferStringSet& fnms )
{
    for ( int idx=0; idx<fnms.size(); idx++ )
	addFile( fnms.get(idx) );
}


const BufferStringSet FileSystemWatcher::files() const
{
    BufferStringSet files;
    const QStringList filenms = qfswatcher_->files();
    for ( const auto& nm : filenms )
	files.add( BufferString(nm) );

    return files;
}


const BufferStringSet FileSystemWatcher::directories() const
{
    BufferStringSet dirs;
    const QStringList qdirs = qfswatcher_->directories();
    for ( const auto& dir : qdirs )
	dirs.add( BufferString(dir) );

    return dirs;
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
    static PtrMan<FileSystemWatcher> fsw = new FileSystemWatcher;
    return *fsw;
}
