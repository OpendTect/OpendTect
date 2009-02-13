#ifndef gendefs_h
#define gendefs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 1995
 RCS:		$Id: gendefs.h,v 1.38 2009-02-13 13:31:14 cvsbert Exp $
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
 gendefs.h

 All the above are usable from C and C++.

-*/

#ifndef undefval_h
# include "undefval.h"
#endif
#ifndef commondefs_h
#include "commondefs.h"
#endif


#endif
