#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "namedobj.h"
#include "bufstring.h"

class BufferStringSet;

/*!\brief Distributed Processing*/

namespace MMProc
{

/*!
\brief Knows available nodes and decides which to use.
*/

mExpClass(MMProc) AssetMgr : public NamedObject
{
public:
    			AssetMgr( const char* nm )
			    : NamedObject(nm)			{}

    virtual void	getHostNames(BufferStringSet&) const		= 0;
    virtual void	getAvailableHosts( BufferStringSet& bs ) const
			{ return getHostNames( bs ); }

    static int		add(AssetMgr*);
};

mGlobal(MMProc) const ObjectSet<MMProc::AssetMgr>& ASMGRS();


}; // namespace MMProc
