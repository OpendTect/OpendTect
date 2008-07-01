#ifndef mmassetmgr_h
#define mmassetmgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: mmassetmgr.h,v 1.1 2008-07-01 14:18:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "bufstring.h"
class BufferStringSet;

namespace MMProc
{

/*!\brief Knows available nodes and decides which to use. */

class AssetMgr : public NamedObject
{
public:
    			AssetMgr( const char* nm )
			    : NamedObject(nm)			{}

    virtual void	getHostNames(BufferStringSet&) const		= 0;
    virtual void	getAvailableHosts( BufferStringSet& bs ) const
			{ return getHostNames( bs ); }

    static int		add(AssetMgr*);
};

const ObjectSet<MMProc::AssetMgr>& ASMGRS();


}; // namespace MMProc

#endif
