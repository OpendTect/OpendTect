#ifndef surfaceinfo_h
#define surfaceinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Sep 2002
 RCS:		$Id: surfaceinfo.h,v 1.9 2012-09-13 18:36:27 cvsnanne Exp $
________________________________________________________________________

-*/

#include "generalmod.h"
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

