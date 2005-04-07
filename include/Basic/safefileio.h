#ifndef safefileio_h
#define safefileio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2004
 RCS:		$Id: safefileio.h,v 1.1 2005-04-07 16:28:44 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "strmdata.h"
#include <iosfwd>


/*!\brief Protects file IO when you can't afford to lose the file or have it
  garbled by multiple access.
 
  Note that you can either read or write the file, not both at the same time.

  At the end, one closeFail() or closeSuccess() is absolutely mandatory.
  For writing, these are different, and closeSuccess() can fail.

  When you write, you write to a new file, which will be renamed to the target
  filename on success (after the previous version is renamed to .bak).
  Note that success during writing (i.e. the integrity of the newly written
  file) is something you determine yourself.
 
 */

class SafeFileIO
{
public:

			SafeFileIO(const char*,bool locked);

    bool		open(bool forread,bool ignorelock=false);
    const char*		errMsg() const		{ return errmsg_; }
    std::ostream&	ostrm()			{ return *sd_.ostrm; }
    std::istream&	istrm()			{ return *sd_.istrm; }
    void		closeFail( bool keeplock=false )
			{ doClose( keeplock, false ); }
    bool		closeSuccess( bool keeplock=false )
			{ return doClose( keeplock, true ); }

    int			lockretries_;		//!< default=10
    double		lockwaitincr_;		//!< default=0.5 (seconds)
    bool		allowlockremove_;	//!< default=true

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
