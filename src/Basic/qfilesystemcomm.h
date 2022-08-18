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
    connect( qfswatcher, SIGNAL(directoryChanged(const QString&)),
	     this, SLOT(directoryChanged(const QString&)) );
    connect( qfswatcher, SIGNAL(fileChanged(const QString&)),
	     this, SLOT(fileChanged(const QString&)) );
}

private slots:

void directoryChanged( const QString& path )
{
    fswatcher_->chgddir_ = path.toLatin1().constData();
    fswatcher_->directoryChanged.trigger( *fswatcher_ );
}

void fileChanged( const QString& fnm )
{
    fswatcher_->chgdfile_ = fnm.toLatin1().constData();
    fswatcher_->fileChanged.trigger( *fswatcher_ );
}

private:

    QFileSystemWatcher*		qfswatcher_;
    FileSystemWatcher*		fswatcher_;

};
