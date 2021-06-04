#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "callback.h"
#include "bufstring.h"
#include "commondefs.h"

class BufferStringSet;
class QFileSystemWatcher;
class QFileSystemWComm;

/*!
\brief Class for monitoring a file system.
*/

mExpClass(Basic) FileSystemWatcher : public CallBacker
{
public:
    friend class QFileSystemWComm;

				FileSystemWatcher();
				~FileSystemWatcher();

    void			addFile(const char*);
    void			addFiles(const BufferStringSet&);
    void			removeFile(const char*);
    void			removeFiles(const BufferStringSet&);

    const BufferString&		changedDir() const  { return chgddir_; }
    const BufferString&		changedFile() const { return chgdfile_; }

    Notifier<FileSystemWatcher>	directoryChanged;
    Notifier<FileSystemWatcher>	fileChanged;

protected:

    BufferString		chgddir_;
    BufferString		chgdfile_;

    QFileSystemWatcher*		qfswatcher_;
    QFileSystemWComm*		qfswatchercomm_;

};

mGlobal(Basic) FileSystemWatcher& FSW();
