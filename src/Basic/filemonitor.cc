/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2016
-*/


#include "filemonitor.h"
#include "file.h"
#include "bufstringset.h"
#include "i_qfilesystemwatcher.h"


File::Monitor::Monitor()
    : watcher_(*new mQtclass(i_QFileSystemWatcher)(*this))
    , fileChanged(this)
    , dirChanged(this)
{
}


File::Monitor::~Monitor()
{
    delete &watcher_;
}


void File::Monitor::watch( const char* fnm )
{
#ifndef OD_NO_QT
    watcher_.addPath( QString(fnm) );
#endif
}


void File::Monitor::watch( const BufferStringSet& fnms )
{
#ifndef OD_NO_QT
    QStringList qstrs; fnms.fill( qstrs );
    watcher_.addPaths( qstrs );
#endif
}


void File::Monitor::forget( const char* fnm )
{
#ifndef OD_NO_QT
    watcher_.removePath( QString(fnm) );
#endif
}


void File::Monitor::updateWatcher(const char* fnm)
{
#ifndef OD_NO_QT
    if ( !watcher_.files().contains(fnm) )
	watcher_.addPath(fnm);
    else
	watcher_.fileChg(fnm);
#endif
}
