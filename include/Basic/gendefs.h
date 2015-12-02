#ifndef gendefs_h
#define gendefs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 1995
 RCS:		$Id$
________________________________________________________________________

 This file contains general defines that are so basic they can (read: MUST)
 be used in any source file. If you have no include anywhere in your header
 or source file, include this one.

 undefval.h includes plftypes.h which includes plfdefs.h. Thus, every single
 source file in OD is dep on:

 plfdefs.h
 plftypes.h
 undefval.h
 commondefs.h
 commontypes.h
 errmsg.h
 gendefs.h

-*/

#ifndef undefval_h
# include "undefval.h"
#endif
#ifndef commontypes_h
#include "commontypes.h"
#endif
#ifndef errmsg_h
#include "errmsg.h"
#endif

#ifdef __cpp__

namespace OD
{
    // For large memory operations: consider using odmemory.h tools.

    //! Simple function; use if your data is measured in kBs or less
    mGlobal(Basic) void	memCopy(void*,const void*,od_int64);
    //! Simple function; use if your data is measured in kBs or less
    mGlobal(Basic) void	memSet(void*,char,od_int64);
    //! Simple function; use if your data is measured in kBs or less
    mGlobal(Basic) void	memZero(void*,od_int64);

}

#endif


#endif
