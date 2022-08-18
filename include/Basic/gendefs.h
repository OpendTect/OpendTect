#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 This file contains general defines that are so basic they can (read: MUST)
 be used in any source file. If you have no include anywhere in your header
 or source file, include this one.

 undefval.h includes plftypes.h which includes plfdefs.h. Thus, every single
 source file in OD is dep on:

- plfdefs.h
- plftypes.h
- undefval.h
- commondefs.h
- commontypes.h
- errmsg.h
- gendefs.h

-*/

#include "undefval.h"
#include "commontypes.h"
#include "errmsg.h"


#ifdef __cpp__

namespace OD
{
    // For guaranteed small or large operations: consider using odmemory.h

    // If you are in parallel execution, use sysMemCopy, sysMemSet, sysMemZero

    //! Function will figure out which is fastest depending on size
    mGlobal(Basic) void	memCopy(void*,const void*,od_int64);
    //! Function will figure out which is fastest depending on size
    mGlobal(Basic) void	memSet(void*,char,od_int64);
    //! Function will figure out which is fastest depending on size
    mGlobal(Basic) void	memZero(void*,od_int64);
    //! Function will figure out which is fastest depending on size
    mGlobal(Basic) void	memMove(void*,const void*,od_int64);

}

#endif
