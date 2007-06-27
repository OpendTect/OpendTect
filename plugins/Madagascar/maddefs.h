#ifndef maddefs_h
#define maddefs_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
 * ID       : $Id: maddefs.h,v 1.2 2007-06-27 16:41:59 cvsbert Exp $
-*/

#include "bufstring.h"
#include "sets.h"

namespace ODMad
{

class ProgDef
{
public:

    BufferString	name_;
    BufferString	synopsis_;
    BufferString	comments_;

};


/* Scans $RSFROOT/defs directory for program definitions */

class ProgInfo
{
public:

			ProgInfo(const char* rsfroot=0);
    			//!< if Pre-Scan check is OK, err msg will be empty
			~ProgInfo();

    void		doScan();
    			//!< If scan succeeded, err msg will be empty

    const BufferString&	errMsg() const	{ return errmsg_; }

protected:

    BufferString	rsfroot_;
    BufferString	defdir_;
    BufferString	errmsg_;
    ObjectSet<ProgDef>	defs_;

    void		doPreScanCheck();
    void		addEntry(const char*);
};

} // namespace ODMad

#endif
