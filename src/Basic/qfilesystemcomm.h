#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include <QFileSystemWatcher>
#include "filesystemwatcher.h"

/*!
\brief QFileSystemWatcher communication class

  Internal object, to hide Qt's signal/slot mechanism.
*/

class QFileSystemWComm : public QObject
{
    Q_OBJECT
    friend class	::FileSystemWatcher;

protected:

QFileSystemWComm( QFileSystemWatcher* qfswatcher, FileSystemWatcher* fswatcher )
    : qfswatcher_(qfswatcher)
    , fswatcher_(fswatcher)
{
    connect( qfswatcher, &QFileSystemWatcher::directoryChanged,
	     this, &QFileSystemWComm::directoryChanged );
    connect( qfswatcher, &QFileSystemWatcher::fileChanged,
	     this, &QFileSystemWComm::fileChanged );
}

private slots:

void directoryChanged( const QString& path )
{
    const BufferString chgddir( path.toLatin1().constData() );
    fswatcher_->directoryChanged.trigger( chgddir );
}

void fileChanged( const QString& fnm )
{
    const BufferString chgdfile( fnm.toLatin1().constData() );
    fswatcher_->fileChanged.trigger( chgdfile );
}

private:

    QFileSystemWatcher*		qfswatcher_;
    FileSystemWatcher*		fswatcher_;

};
