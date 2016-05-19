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
#include "uistring.h"

class BufferStringSet;
class SharedLibAccess;


mExpClass(MATLABLink) MatlabLibAccess
{ mODTextTranslationClass(MatlabLibAccess);
public:
			MatlabLibAccess(const char* libfnm);
			~MatlabLibAccess();

    bool		init();
    bool		terminate();

    void*		getFunction(const char*) const;
    bool		getParameters(int& nrin,int& nrout,
				      BufferStringSet& nms,
				      BufferStringSet& values) const;
    uiString		errMsg() const		{ return errmsg_; }

protected:

    SharedLibAccess*	sla_;
    BufferString	shlibfnm_;
    bool		inited_;
    mutable uiString	errmsg_;
};


mExpClass(MATLABLink) MatlabLibMgr
{ mODTextTranslationClass(MatlabLibMgr);
public:
			MatlabLibMgr();
			~MatlabLibMgr();

    bool		isOK() const	{ return inited_; }

    MatlabLibAccess*	getMatlabLibAccess(const char* libfnm,bool doload);
    bool		isLoaded(const char* libfnm) const;
    bool		close(const char* libfnm);
    uiString		errMsg() const	{ return errmsg_; }


protected:

    bool		initApplication();
    void		terminateApplication();
    bool		load(const char* libfnm);

    ObjectSet<MatlabLibAccess>	mlas_;
    BufferStringSet		libnms_;
    uiString			errmsg_;

    bool			inited_;
};

mGlobal(MATLABLink) MatlabLibMgr& MLM();

#endif
