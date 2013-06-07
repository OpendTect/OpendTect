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

class BufferStringSet;
class SharedLibAccess;


mExpClass(MatlabLink) MatlabLibAccess
{
public:
			MatlabLibAccess(const char* libfnm);
			~MatlabLibAccess();

    bool		init();
    bool		terminate();

    void*		getFunction(const char*) const;
    bool		getParameters(BufferStringSet& nms,
				      BufferStringSet& values) const;
    const char*		errMsg() const		{ return errmsg_; }

protected:

    mutable BufferString errmsg_;
    BufferString	shlibfnm_;
    bool		inited_;
};


mExpClass(MatlabLink) MatlabLibMgr
{
public:
			MatlabLibMgr();
			~MatlabLibMgr();

    const SharedLibAccess* getSharedLibAccess(const char* libfnm) const;
    bool		load(const char* libfnm);
    bool		isLoaded(const char* libfnm) const;
    const char*		errMsg() const	{ return errmsg_; }

    bool		initApplication();
    void		terminateApplication();

protected:

    ObjectSet<SharedLibAccess>	slas_;
    BufferStringSet		libnms_;
    BufferString		errmsg_;

    bool			inited_;
};

mGlobal(MatlabLink) MatlabLibMgr& MLM();

#endif
