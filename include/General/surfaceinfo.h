#ifndef surfaceinfo_h
#define surfaceinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Sep 2002
 RCS:		$Id: surfaceinfo.h,v 1.4 2003-11-07 12:21:51 bert Exp $
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
