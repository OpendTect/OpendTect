#ifndef surfaceinfo_h
#define surfaceinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		Sep 2002
 RCS:		$Id: surfaceinfo.h,v 1.3 2003-02-11 09:57:11 nanne Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "multiid.h"

/*!\brief Surface info name/attribname with an ID (usually the EM-ID). */

class SurfaceInfo
{
public:
		   	 SurfaceInfo( const char* nm, MultiID mi, int vi=-1, 
				      const char* attr=0)
		    	: multiid(mi), visid(vi), name(nm), attrnm(attr) {}

    MultiID		multiid;
    int			visid;
    BufferString	name;
    BufferString	attrnm;

};


#endif
