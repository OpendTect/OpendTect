#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
