#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "namedobj.h"
#include "bufstring.h"

class BufferStringSet;

/*!\brief Distributed Computing*/

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


} // namespace MMProc
