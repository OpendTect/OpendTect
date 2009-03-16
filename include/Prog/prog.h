#ifndef prog_h
#define prog_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		5-12-1995
 RCS:		$Id: prog.h,v 1.13 2009-03-16 10:34:00 cvsranojay Exp $
________________________________________________________________________

 Include this file in any executable program you make. The file is actually
 pretty empty ....

-*/

#include "plugins.h"
#include "debug.h"

#ifdef __cpp__
extern "C" {
#endif
    const char*		errno_message();
    			/*!< Will not return meaningful string on Windows */
#ifdef __cpp__
}
#endif


#endif
