#ifndef cvsaccess_h
#define cvsaccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2011
 RCS:           $Id: cvsaccess.h,v 1.4 2011-12-13 12:28:35 cvsbert Exp $
________________________________________________________________________

-*/
;

#include "bufstring.h"
class BufferStringSet;


mClass CVSAccess
{

public:

    			CVSAccess(const char* dir);
    virtual		~CVSAccess();
    bool		isOK() const		{ return *host(); }
    const char*		host() const		{ return host_; }
    bool		hostOK() const;

    			// Local scan
    void		getEntries(const char* subdir,BufferStringSet&) const;

    			// On Client
    bool		update(const char* fnm=0);
    bool		edit(const char*);
    bool		edit(const BufferStringSet&);
    bool		add(const char*,bool binary=false);
    bool		add(const BufferStringSet&,bool binary=false);
    bool		remove(const char*);
    bool		remove(const BufferStringSet&);
    				//!< will also remove files if they exist
    bool		commit(const char* msg=0);
    bool		commit(const BufferStringSet&,const char* msg=0);

    			// On Server. Default serverdir is /cvsroot
    const char*		serverDir() const		{ return serverdir_; }
    void		setServerDir( const char* sd )	{ serverdir_ = sd; }
    bool		rename(const char* subdir,const char* from,
	    			const char* to);

    void		checkEdited(const char* fnm,BufferStringSet& edtxts);
    void		diff(const char* fnm,BufferString&);

protected:

    const BufferString	dir_;
    const BufferString	host_;
    BufferString	serverdir_;

};

#endif
