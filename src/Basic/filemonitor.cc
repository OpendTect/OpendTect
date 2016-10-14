/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2016
-*/


#include "filemonitor.h"
#include "bufstringset.h"

#ifdef OD_NO_QT
 class od_i_FSWatcher
     { od_i_FSWatcher(File::Monitor&) {} };
#else
# include <QString>
# include <QStringList>
# include <QFileSystemWatcher>

QT_BEGIN_NAMESPACE

class od_i_FSWatcher : public QFileSystemWatcher
{
    friend class    File::Monitor;

protected:

od_i_FSWatcher( File::Monitor& fm )
    : fm_(fm)
{
    connect( this, SIGNAL(directoryChanged(const QString&)),
	     this, SLOT(dirChg(const QString&)) );
    connect( this, SIGNAL(fileChanged(const QString&)),
	     this, SLOT(fileChg(const QString&)) );
}

public:

~od_i_FSWatcher() { deactivate(); }
void deactivate() {}

private slots:

void dirChg( const QString& qstr )
{
    const BufferString dirnm( qstr );
    fm_.dirChanged.trigger( dirnm );
}

void fileChg( const QString& qstr )
{
    const BufferString filenm( qstr );
    fm_.fileChanged.trigger( filenm );
}

private:

    File::Monitor&	fm_;

};

QT_END_NAMESPACE

#endif


File::Monitor::Monitor()
    : watcher_(*new mQtclass(od_i_FSWatcher)(*this))
    , fileChanged(this)
    , dirChanged(this)
{
}


File::Monitor::~Monitor()
{
    delete &watcher_;
}


void File::Monitor::start( const char* fnm )
{
#ifndef OD_NO_QT
    watcher_.addPath( QString(fnm) );
#endif
}


void File::Monitor::start( const BufferStringSet& fnms )
{
#ifndef OD_NO_QT
    QStringList qstrs; fnms.fill( qstrs );
    watcher_.addPaths( qstrs );
#endif
}


void File::Monitor::stop( const char* fnm )
{
#ifndef OD_NO_QT
    watcher_.removePath( QString(fnm) );
#endif
}
