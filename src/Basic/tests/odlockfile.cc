/*
 * ________________________________________________________________________
 *
 * Copyright:	(C) 1995-2026 dGB Beheer B.V.
 * License:	https://dgbes.com/licensing
 * ________________________________________________________________________
 *
 *
 */

#include "odlockfile.h"
#include "testprog.h"
#include "genc.h"


static bool testLockUnlock( const BufferString& lockfile )
{
    BufferString hostnm, appnm;
    od_int64 pid;
    OD::LockFile lck( lockfile.buf() );
    mRunStandardTest( !lck.isLocked(), "Not locked before create" );
    mRunStandardTest( lck.lock(), "Create a lock file" );
    mRunStandardTest( lck.isLocked(), "Is locked after create" );
    mRunStandardTest( !lck.tryLock(), "Cannot lock if already locked" );
    lck.getLockInfo( pid, hostnm, appnm );
    mRunStandardTest( pid==GetPID(), "Lock info - PID" );
    mRunStandardTest( appnm==GetExecutableName(),
		      "Lock info - application name" );
    lck.unlock();
    mRunStandardTest( !lck.isLocked(), "Not locked after remove" );
    return true;
}




int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    FilePath fp( __FILE__ );
    fp.setExtension( "lck" );

    const BufferString lockfile( fp.fullPath() );
    if (
	!testLockUnlock(lockfile) )
	return 1;

    return 0;
}


