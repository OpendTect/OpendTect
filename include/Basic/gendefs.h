#ifndef gendefs_H
#define gendefs_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		1-9-1995
 Contents:	General definitions for every module
 RCS:		$Id: gendefs.h,v 1.37 2006-03-12 13:39:09 cvsbert Exp $
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

#include "undefval.h"
#include "commondefs.h"


#endif
