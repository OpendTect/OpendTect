#ifndef qfilesystemcomm_h
#define qfilesystemcomm_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/

#include <QFileSystemWatcher>
#include "filesystemwatcher.h"

QT_BEGIN_NAMESPACE

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

QT_END_NAMESPACE

#endif
