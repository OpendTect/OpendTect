#ifndef cvsaccess_h
#define cvsaccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2011
 RCS:           $Id$
________________________________________________________________________

-*/
;

#include "networkmod.h"
#include "bufstring.h"
class BufferStringSet;


mClass(Network) SVNAccess
{

public:

    			SVNAccess(const char* dir);
    virtual		~SVNAccess();
    bool		isOK() const;
    const char*		host() const		{ return host_; }

    			// info
    bool		isInSVN(const char*) const;
    void		getEntries(const char* subdir,BufferStringSet&) const;
    const char*		reposDir() const	{ return reposdir_; }
    void		diff(const char* fnm,BufferString&) const;

    			// changes locally
    bool		update(const char* fnm=0);
    bool		lock(const char*);
    bool		lock(const BufferStringSet&);

    			// sets up for repos change
    bool		add(const char*);
    bool		add(const BufferStringSet&);

    			// sets up for repos change and changes locally
    bool		rename(const char* subdir,const char* from,
				const char* to);
    bool		changeFolder(const char* fnm,const char* fromsubdir,
				     const char* tosubdir);
    bool		remove(const char*);
    bool		remove(const BufferStringSet&);

    			// changes repository
    bool		commit(const char* fnm,const char* msg=0);
    bool		commit(const BufferStringSet&,const char* msg=0);


protected:

    const bool		havesvn_;
    const BufferString	dir_;
    const BufferString	host_;
    const BufferString	reposdir_;

};

#endif

