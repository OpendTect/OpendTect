#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "bufstring.h"

class QLockFile;

namespace OD
{

/*!
\brief Generates a lock file
*/

mExpClass(Basic) LockFile
{
public:
    enum LockError {
	    NoError = 0,
	    LockFailedError = 1,
	    PermissionError = 2,
	    UnknownError = 3
    };

			LockFile(const char* fileName);
			~LockFile();

    BufferString	fileName() const;
    bool		lock();
    bool		tryLock(od_int64 timeout_ms=0);
    void		unlock();
    LockError		error() const;

    void		setStaleLockTime_ms(od_int64 value);
    od_int64		staleLockTime_ms() const;

    bool		isLocked() const;
    bool		getLockInfo(od_int64& pid, BufferString& hostname,
				    BufferString& appname) const;
    bool		removeStaleLockFile();

private:
    QLockFile&		qlockfile_;
    mOD_DisableCopy(LockFile);
};

} // namespace OD
