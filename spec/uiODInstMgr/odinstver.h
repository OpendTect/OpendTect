#ifndef odinstver_h
#define odinstver_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2010
 RCS:           $Id: odinstver.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "bufstring.h"


namespace ODInst
{

mDefClass(uiODInstMgr) Version
{
public:
    			Version( const char* s=0 )	{ set(s); }
    			Version( int s, int ma, int mi, const char* p )
			    : super_(s), major_(ma), minor_(mi)
			    , patch_(p)			{}
    bool		operator ==(const Version&) const;
    bool		operator >(const Version&) const;

    BufferString	dirName(bool local) const;
			// false will give dirname on server
    BufferString	fullName() const;
    bool		isCompatibleWith(const Version&) const;
    bool		isEmpty() const { return !super_ && !major_ && !minor_
					      && patch_.isEmpty(); }
    inline BufferString	userName() const { return fullName(); }

    void		set(const char*);
    void		setEmpty();

    int			super_;
    int			major_;
    int			minor_;
    BufferString	patch_;

};

} // namespace

#endif

