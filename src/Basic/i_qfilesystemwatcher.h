#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2016
-*/


#ifdef OD_NO_QT

 class i_QFileSystemWatcher
     { i_QFileSystemWatcher(File::Monitor&) {} };

#else

# include "filemonitor.h"
# include <QString>
# include <QStringList>
# include <QFileSystemWatcher>

QT_BEGIN_NAMESPACE

class i_QFileSystemWatcher : public QFileSystemWatcher
{
    Q_OBJECT
    friend class    File::Monitor;

protected:

i_QFileSystemWatcher( File::Monitor& fm )
    : fm_(fm)
{
    connect( this, SIGNAL(directoryChanged(const QString&)),
	     this, SLOT(dirChg(const QString&)) );
    connect( this, SIGNAL(fileChanged(const QString&)),
	     this, SLOT(fileChg(const QString&)) );
}

public:

~i_QFileSystemWatcher() { deactivate(); }

void deactivate()
{
    disconnect( this, SIGNAL(directoryChanged(const QString&)),
	     this, SLOT(dirChg(const QString&)) );
    disconnect( this, SIGNAL(fileChanged(const QString&)),
	     this, SLOT(fileChg(const QString&)) );
}

private slots:

void dirChg( const QString& qstr )
{
    const BufferString dirnm( qstr );
    fm_.dirChanged.trigger( dirnm );
}

void fileChg( const QString& qstr )
{
    const BufferString filenm( qstr );
    fm_.watch( filenm );
    fm_.fileChanged.trigger( filenm );
}

private:

    File::Monitor&	fm_;

};

QT_END_NAMESPACE

#endif
