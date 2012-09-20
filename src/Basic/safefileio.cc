/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "safefileio.h"

#include "dateinfo.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "hostdata.h"
#include "oddirs.h"
#include "strmprov.h"
#include "thread.h"

#include <iostream>


SafeFileIO::SafeFileIO( const char* fnm, bool l )
    	: filenm_(fnm)
    	, locked_(l)
    	, removebakonsuccess_(false)
    	, usebakwhenmissing_(true)
    	, lockretries_(10)
    	, lockwaitincr_(0.5)
    	, allowlockremove_(true)
{
    FilePath fp( filenm_ );
    const_cast<BufferString&>(filenm_) = fp.fullPath();
    const BufferString filenmonly( fp.fileName() );

    fp.setFileName( 0 );
    BufferString curfnm( ".lock." ); curfnm += filenmonly;
    const_cast<BufferString&>(lockfnm_) = FilePath(fp,curfnm).fullPath();
    curfnm = filenmonly; curfnm += ".bak";
    const_cast<BufferString&>(bakfnm_) = FilePath(fp,curfnm).fullPath();
    curfnm = filenmonly; curfnm += ".new";
    const_cast<BufferString&>(newfnm_) = FilePath(fp,curfnm).fullPath();
}


bool SafeFileIO::open( bool forread, bool ignorelock )
{
    return forread ? openRead( ignorelock ) : openWrite( ignorelock );
}


bool SafeFileIO::openRead( bool ignorelock )
{
    if ( locked_ && !ignorelock && !waitForLock() )
	return false;
    mkLock( true );

    const char* toopen = filenm_.buf();
    if ( File::isEmpty(toopen) )
    {
	if ( usebakwhenmissing_ )
	    toopen = bakfnm_.buf();
	if ( File::isEmpty(toopen) )
	{
	    errmsg_ = "Input file '"; errmsg_ += filenm_;
	    errmsg_ += "' is not present or empty";
	    rmLock();
	    return false;
	}
	else
	{
	    errmsg_ = "Using backup file '";
	    errmsg_ += bakfnm_; errmsg_ += "'";
	    UsrMsg( errmsg_.buf(), MsgClass::Warning );
	    errmsg_ = "";
	}
    }

    sd_ = StreamProvider( toopen ).makeIStream();
    if ( !sd_.usable() )
    {
	errmsg_ = "Cannot open '"; errmsg_ += toopen;
	errmsg_ += "' for read";
	rmLock();
	return false;
    }

    return true;
}


bool SafeFileIO::openWrite( bool ignorelock )
{
    if ( locked_ && !ignorelock && !waitForLock() )
	return false;
    mkLock( false );

    if ( File::exists(newfnm_) )
	File::remove( newfnm_ );

    sd_ = StreamProvider( newfnm_ ).makeOStream();
    if ( !sd_.usable() )
    {
	errmsg_ = "Cannot open '"; errmsg_ += newfnm_;
	errmsg_ += "' for write";
	rmLock();
	return false;
    }

    return true;
}


bool SafeFileIO::commitWrite()
{
    if ( File::isEmpty(newfnm_) )
    {
	errmsg_ = "File '"; errmsg_ += filenm_;
	errmsg_ += "' not overwritten with empty new file";
	return false;
    }

    if ( File::exists( bakfnm_ ) )
	File::remove( bakfnm_ );

    if ( File::exists(filenm_) && !File::rename( filenm_, bakfnm_ ) )
    {
	errmsg_ = "Cannot create backup file '";
	errmsg_ += bakfnm_; errmsg_ += "'";
	UsrMsg( errmsg_.buf(), MsgClass::Warning );
	errmsg_ = "";
    }
    if ( !File::rename( newfnm_, filenm_ ) )
    {
	errmsg_ = "Changes in '"; errmsg_ += filenm_;
	errmsg_ += "' could not be commited.";
	return false;
    }
    if ( removebakonsuccess_ )
	File::remove( bakfnm_ );

    return true;
}


bool SafeFileIO::remove()
{
    if ( locked_ )
	waitForLock();

    if ( File::exists(newfnm_) )
	File::remove( newfnm_ );
    if ( File::exists(bakfnm_) )
	File::remove( bakfnm_ );
    if ( File::exists(filenm_) )
	File::remove( filenm_ );

    return File::exists(filenm_);
}


bool SafeFileIO::doClose( bool keeplock, bool docommit )
{
    bool isread = sd_.istrm;
    sd_.close();
    bool res = true;
    if ( isread || !docommit )
    {
	if ( !isread && File::exists(newfnm_) )
	    File::remove( newfnm_ );
    }
    else
	res = commitWrite();

    if ( !keeplock )
	rmLock();

    return res;
}


bool SafeFileIO::haveLock() const
{
    //TODO read the lock file to see whether date and time are feasible
    return File::exists( lockfnm_ );
}


bool SafeFileIO::waitForLock() const
{
    bool havelock = haveLock();
    if ( !havelock )
	return true;

    for ( int idx=0; havelock && idx<lockretries_; idx++ )
    {
	Threads::sleep( lockwaitincr_ );
	havelock = haveLock();
    }

    if ( !havelock )
	return true;

    if ( allowlockremove_ )
    {
	File::remove( lockfnm_ );
	return true;
    }

    errmsg_ = "File '"; errmsg_ += filenm_;
    errmsg_ = "' is currently locked.\n";
    return false;
}


void SafeFileIO::mkLock( bool forread )
{
    if ( !locked_ ) return;

    StreamData sd = StreamProvider(lockfnm_).makeOStream();
    if ( sd.usable() )
    {
	DateInfo di; BufferString datestr; di.getUsrDisp( datestr, true );
	*sd.ostrm << "Type: " << (forread ? "Read\n" : "Write\n");
	*sd.ostrm << "Date: " << datestr << " (" << di.key() << ")\n";
	*sd.ostrm << "Host: " << HostData::localHostName() << '\n';
	*sd.ostrm << "Process: " << GetPID() << '\n';
	const char* ptr = GetPersonalDir();
	*sd.ostrm << "User's HOME: " << (ptr ? ptr : "<none>") << '\n';
	ptr = GetSoftwareUser();
	*sd.ostrm << "DTECT_USER: " << (ptr ? ptr : "<none>") << '\n';
	sd.ostrm->flush();
    }
    sd.close();
}


void SafeFileIO::rmLock()
{
    if ( locked_ && File::exists(lockfnm_) )
	File::remove( lockfnm_ );
}
