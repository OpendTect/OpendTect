/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odlockfile.h"

#include <QLockFile>

namespace OD
{

LockFile::LockFile( const char* fileName )
    : qlockfile_(*new QLockFile(fileName))
{
}


LockFile::~LockFile()
{
    delete &qlockfile_;
}


BufferString LockFile::fileName() const
{
    return qlockfile_.fileName();
}


bool LockFile::lock()
{
    return qlockfile_.lock();
}


bool LockFile::tryLock( od_int64 timeout_ms )
{
    return qlockfile_.tryLock( timeout_ms );
}


void LockFile::unlock()
{
    qlockfile_.unlock();
}


LockFile::LockError LockFile::error() const
{
    return (LockError) qlockfile_.error();
}


void LockFile::setStaleLockTime_ms( od_int64 value )
{
    qlockfile_.setStaleLockTime( value );
}


od_int64 LockFile::staleLockTime_ms() const
{
    return qlockfile_.staleLockTime();
}


bool LockFile::isLocked() const
{
    return qlockfile_.isLocked();
}


bool LockFile::getLockInfo( od_int64& pid, BufferString& hostname,
			    BufferString& appname ) const
{
    QString qhostnm, qappnm;
    const bool res = qlockfile_.getLockInfo( (qint64*)&pid, &qhostnm, &qappnm );
    hostname = qhostnm;
    appname = qappnm;
    return res;
}


bool LockFile::removeStaleLockFile()
{
    return qlockfile_.removeStaleLockFile();
}

} // namespace OD
