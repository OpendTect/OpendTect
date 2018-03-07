#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2002
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstring.h"
#include "dbkey.h"

/*!\brief Surface info name/attribname with an ID (usually the EM-ID). */

mClass(General) SurfaceInfo
{
public:
		   	 SurfaceInfo( const char* nm, DBKey mi, int vi=-1, 
				      const char* attr=0)
		    	: dbkey(mi), visid(vi), name(nm), attrnm(attr) {}

    DBKey		dbkey;
    int			visid;
    BufferString	name;
    BufferString	attrnm;

};
