/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "safefileio.h"

#include "dateinfo.h"
#include "file.h"
#include "filepath.h"
#include "hostdata.h"
#include "oddirs.h"
#include "strmprov.h"
#include "thread.h"
#include "msgh.h"


SafeFileIO::SafeFileIO( const char* fnm, bool l )
    	: filenm_(fnm)
    	, strm_(0)
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


SafeFileIO::~SafeFileIO()
{
    if ( strm_ )
	{ pErrMsg("You forgot to close"); }
}


od_istream& SafeFileIO::istrm()
{
    if ( !strm_ )
	{ pErrMsg("no open stream" ); return od_istream::nullStream(); }
    if ( !strm_->forRead() )
	{ pErrMsg("no read stream" ); return od_istream::nullStream(); }
    return static_cast<od_istream&>( *strm_ );
}


od_ostream& SafeFileIO::ostrm()
{
    if ( !strm_ )
	{ pErrMsg("no open stream" ); return od_ostream::nullStream(); }
    if ( !strm_->forWrite() )
	{ pErrMsg("no write stream" ); return od_ostream::nullStream(); }
    return static_cast<od_ostream&>( *strm_ );
}


bool SafeFileIO::open( bool forread, bool ignorelock )
{
    return forread ? openRead( ignorelock ) : openWrite( ignorelock );
}


bool SafeFileIO::openRead( bool ignorelock )
{
    if ( strm_ )
	{ pErrMsg("Stream open before openRead"); closeFail(); }

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
	    errmsg_.set( "Input file '" ).add( filenm_ )
		   .add( "' is not present or empty" );
	    rmLock();
	    return false;
	}
	else
	{
	    const BufferString msg( "Using backup file '", bakfnm_, "'" );
	    UsrMsg( errmsg_.buf(), MsgClass::Warning );
	}
    }

    strm_ = new od_istream( toopen );
    if ( !strm_ || !strm_->isOK() )
    {
	errmsg_.set( "Cannot open '" ).add( toopen ).add( "' for read" );
	if ( strm_ )
	{
	    errmsg_.add( ".\n" ).add( strm_->errMsg() );
	    delete strm_; strm_ = 0;
	}
	rmLock();
	return false;
    }

    return true;
}


bool SafeFileIO::openWrite( bool ignorelock )
{
    if ( strm_ )
	{ pErrMsg("Stream open before openWrite"); closeFail(); }

    if ( locked_ && !ignorelock && !waitForLock() )
	return false;
    mkLock( false );

    if ( File::exists(newfnm_) )
	File::remove( newfnm_ );

    strm_ = new od_ostream( newfnm_ );
    if ( !strm_ || !strm_->isOK() )
    {
	errmsg_.set( "Cannot open '" ).add( newfnm_ ).add( "' for write" );
	if ( strm_ )
	{
	    errmsg_.add( ".\n" ).add( strm_->errMsg() );
	    delete strm_; strm_ = 0;
	}
	rmLock();
	return false;
    }

    return true;
}


bool SafeFileIO::commitWrite()
{
    if ( File::isEmpty(newfnm_) )
    {
	errmsg_.set( "File '" ).add( filenm_ )
		.add( "' not overwritten with empty new file" );
	return false;
    }

    if ( File::exists( bakfnm_ ) )
	File::remove( bakfnm_ );

    if ( File::exists(filenm_) && !File::rename( filenm_, bakfnm_ ) )
    {
	const BufferString msg( "Cannot create backup file '", bakfnm_, "'" );
	UsrMsg( msg.buf(), MsgClass::Warning );
    }
    if ( !File::rename( newfnm_, filenm_ ) )
    {
	errmsg_.set( "Changes in '" ).add( filenm_ )
	       .add( "' could not be commited" );
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
    bool isread = false; bool iswrite = false;
    if ( strm_ )
    {
	isread = strm_->forRead();
	iswrite = strm_->forWrite();
	delete strm_; strm_ = 0;
    }
    bool res = true;
    if ( isread || !docommit )
    {
	if ( !isread && File::exists(newfnm_) )
	    File::remove( newfnm_ );
    }
    else if ( iswrite )
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

    od_ostream strm( lockfnm_ );
    if ( strm.isOK() )
    {
	DateInfo di; BufferString datestr; di.getUsrDisp( datestr, true );
	strm << "Type: " << (forread ? "Read\n" : "Write\n");
	strm << "Date: " << datestr << " (" << di.key() << ")\n";
	strm << "Host: " << HostData::localHostName() << od_newline;
	strm << "Process: " << GetPID() << '\n';
	const char* ptr = GetPersonalDir();
	strm << "User's HOME: " << (ptr ? ptr : "<none>") << od_newline;
	ptr = GetSoftwareUser();
	strm << "DTECT_USER: " << (ptr ? ptr : "<none>") << od_endl;
    }
}


void SafeFileIO::rmLock()
{
    if ( locked_ && File::exists(lockfnm_) )
	File::remove( lockfnm_ );
}
