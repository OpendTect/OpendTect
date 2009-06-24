#ifndef prog_h
#define prog_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		5-12-1995
 RCS:		$Id: prog.h,v 1.18 2009-06-24 11:00:21 cvsbert Exp $
________________________________________________________________________

 Include this file in any executable program you make. The file is actually
 pretty empty ....

-*/

#include "plugins.h"
#include "debug.h"

#ifdef __msvc__
# ifndef _CONSOLE
#  include "winmain.h"
# endif
#endif


#ifdef __cpp__
extern "C" {
#endif
    const char*		errno_message();
    			/*!< Will not return meaningful string on Windows */
    void		forkProcess();
    			/*!< Doesn't do anything om mac and windows */
#ifdef __cpp__
}
#endif

#endif
