#ifndef matlablibmgr_h
#define matlablibmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2013
 RCS:		$Id$
________________________________________________________________________

-*/


#include "matlablinkmod.h"
#include "bufstring.h"
#include "bufstringset.h"

class SharedLibAccess;

mExpClass(MatlabLink) MatlabLibMgr
{
public:
			MatlabLibMgr();
			~MatlabLibMgr();

    const SharedLibAccess* getSharedLibAccess(const char* libfnm) const;
    bool		load(const char* libfnm);
    bool		isLoaded(const char* libfnm) const;
    const char*		errMsg() const	{ return errmsg_; }

protected:

    ObjectSet<SharedLibAccess>	slas_;
    BufferStringSet		libnms_;
    BufferString		errmsg_;
};

mGlobal(MatlabLink) MatlabLibMgr& MLM();

#endif
