#ifndef cvsaccess_h
#define cvsaccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2011
 RCS:           $Id: cvsaccess.h,v 1.2 2011-12-13 09:17:26 cvsbert Exp $
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

    bool		update(const char* fnm=0);
    bool		edit(const char* fnm=0);
    bool		add(const BufferStringSet&,bool binary=false);
    bool		commit(const char* msg=0);
    bool		commit(const BufferStringSet&,const char* msg=0);

    void		checkEdited(const char* fnm,BufferStringSet& edtxts);

protected:

    const BufferString	dir_;
    const BufferString	host_;

};

#endif
