#ifndef surfaceinfo_h
#define surfaceinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Sep 2002
 RCS:		$Id$
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


#endif

