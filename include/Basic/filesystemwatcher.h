#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bufstring.h"
#include "callback.h"
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

    bool			addPath(const char*);
				// -> Use when adding a directory
    bool			addFile(const char*);
				// -> Use when adding a file. Is essentially
				// -> the same as addPath, but makes it easier
				// -> to read.
    void			addFiles(const BufferStringSet&);
    void			removeFile(const char*);
    void			removeFiles(const BufferStringSet&);

    const BufferStringSet	files() const;
    const BufferStringSet	directories() const;

    CNotifier<FileSystemWatcher,BufferString>	directoryChanged;
    CNotifier<FileSystemWatcher,BufferString>	fileChanged;

protected:

    QFileSystemWatcher*		qfswatcher_;
    QFileSystemWComm*		qfswatchercomm_;

};

mGlobal(Basic) FileSystemWatcher& FSW();
