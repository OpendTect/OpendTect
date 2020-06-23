#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2011
________________________________________________________________________

-*/
;

#include "networkmod.h"
#include "bufstring.h"
class BufferStringSet;


mExpClass(Network) CVSAccess
{

public:

			CVSAccess(const char* dir);
    virtual		~CVSAccess();
    bool		isOK() const		{ return *host(); }
    const char*		host() const		{ return host_; }
    bool		hostOK() const;

			// info
    bool		isInCVS(const char*) const;
    void		getEntries(const char* subdir,BufferStringSet&) const;
    const char*		baseDir() const		{ return dir_; }
    const char*		reposDir() const	{ return reposdir_; }
    void		getEditTxts(const char* fnm,BufferStringSet&) const;
    void		diff(const char* fnm,BufferString&) const;

			// changes locally
    bool		update(const char* fnm=0);
    bool		edit(const char*);
    bool		edit(const BufferStringSet&);

			// sets up for repos change
    bool		add(const char*,bool binary=false);
    bool		add(const BufferStringSet&,bool binary=false);
			// sets up for repos change and changes locally
    bool		remove(const char*);
    bool		remove(const BufferStringSet&);
    bool		rename(const char* subdir,const char* from,
				const char* to);
    bool		changeFolder(const char* fnm,const char* fromsubdir,
				     const char* tosubdir);

			// changes repository
    bool		commit(const char* fnm,const char* msg=0);
    bool		commit(const BufferStringSet&,const char* msg=0);


protected:

    const BufferString	dir_;
    const BufferString	host_;
    BufferString	serverdir_;
    BufferString	reposdir_;

    bool		doRename(const char*,const char*,
				 const char*, const char*);

};
