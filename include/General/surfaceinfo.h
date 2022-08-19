#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstring.h"
#include "multiid.h"

/*!\brief Surface info name/attribname with an ID (usually the EM-ID). */

mClass(General) SurfaceInfo
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
