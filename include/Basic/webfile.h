#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"

class BufferString;


namespace File
{

			// These functions want reasonably common URI's,
			// no nulls, empties or crap stuff

typedef bool		(*ExistsFn)(const char*);
typedef od_int64	(*GetSizeFn)(const char*);
typedef bool		(*GetContentFn)(const char*,BufferString&);

mGlobal(Basic) void	setWebHandlers(ExistsFn,GetSizeFn,GetContentFn);


} // namespace File


