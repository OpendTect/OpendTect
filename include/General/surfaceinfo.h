#ifndef surfaceinfo_h
#define surfaceinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		Sep 2002
 RCS:		$Id: surfaceinfo.h,v 1.2 2003-02-03 14:07:39 nanne Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "multiid.h"

/*!\brief Surface info name/attribname with an ID (usually the EM-ID). */

class SurfaceInfo
{
public:
		   	 SurfaceInfo( MultiID i, const char* nm, 
				      const char* attr=0)
		    	: id(i), name(nm), attrnm(attr) {}

    MultiID		id;
    BufferString	name;
    BufferString	attrnm;

};


#endif
