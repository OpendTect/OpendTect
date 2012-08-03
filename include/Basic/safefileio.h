#ifndef safefileio_h
#define safefileio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Nov 2004
 RCS:		$Id: safefileio.h,v 1.9 2012-08-03 13:00:14 cvskris Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "strmdata.h"
#include <iosfwd>


/*!\brief Protects file IO when you can't afford to have partly written things
  after write errors or have a file garbled by multiple access.
 
  Use the locking only when multiple processes can concurrently write to
  the same file. For most purposes, you won't need the locking, which is kind
  of expensive, too.

  Note that you can either read or write the file, not both at the same time.
  At the end, one of closeFail() or closeSuccess() is absolutely mandatory.
  Thus, when something goes wrong during writing, use closeFail(), otherwise
  use closeSuccess(). For writing, these do different things, and
  closeSuccess() can fail.

  When you write, you write to a new file, which will be renamed to the target
  filename on success (after the previous version is renamed to .bak).
  Note that success during writing (i.e. the integrity of the newly written
  file) is something you determine yourself.

  When you use locking, you sometimes need to re-read the original file before
  writing. In that case, you need to keep the lock that was made for the
  reading. There is where you want to use the 'ignorelock' and 'keeplock'
  flags. Otherwise, don't specify these.
 
 */

mClass(Basic) SafeFileIO
{
public:

			SafeFileIO(const char*,bool locked=false);

    bool		open(bool forread,bool ignorelock=false);
    const char*		errMsg() const		{ return errmsg_.str(); }
    std::istream&	istrm()			{ return *sd_.istrm; }
    std::ostream&	ostrm()			{ return *sd_.ostrm; }
    StreamData&		strmdata()		{ return sd_; }

    void		closeFail( bool keeplock=false )
			{ doClose( keeplock, false ); }
    bool		closeSuccess( bool keeplock=false )
			{ return doClose( keeplock, true ); }

    const char*		fileName() const	{ return filenm_.buf(); }

    // Some setup variables
    bool		usebakwhenmissing_;	//!< default=true
    bool		removebakonsuccess_;	//!< default=false
    int			lockretries_;		//!< default=10
    double		lockwaitincr_;		//!< default=0.5 (seconds)
    bool		allowlockremove_;	//!< default=true
    			//!< when true, will remove the lock after retries
    			//!< i.e. we'll assume the lock is phony then
    			//!< this is safety-- but robustness++

    bool		remove();

protected:

    const bool		locked_;
    const BufferString	filenm_;
    const BufferString	lockfnm_;
    const BufferString	bakfnm_;
    const BufferString	newfnm_;
    mutable BufferString errmsg_;
    StreamData		sd_;

    bool		openRead(bool);
    bool		openWrite(bool);
    bool		commitWrite();
    bool		doClose(bool, bool);

    bool		haveLock() const;
    bool		waitForLock() const;
    void		mkLock(bool);
    void		rmLock();
};


#endif

