#ifndef surfaceinfo_h
#define surfaceinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		Sep 2002
 RCS:		$Id: surfaceinfo.h,v 1.1 2002-09-19 14:34:25 kristofer Exp $
________________________________________________________________________

-*/

#include "bufstring.h"

/*!\brief Surface info name/attribname with an ID (usually the vis-ID). */

class SurfaceInfo
{
public:
		   	 SurfaceInfo( int i, const char* nm, const char* attr=0)
		    	: id(i), name(nm), attrnm(attr) {}

    int			id;
    BufferString	name;
    BufferString	attrnm;

};


#endif
