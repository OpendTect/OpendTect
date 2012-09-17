#ifndef mallocdefs_h
#define mallocdefs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: mallocdefs.h,v 1.2 2009/07/22 16:01:14 cvsbert Exp $
________________________________________________________________________


-*/

#ifdef __mac__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif
#include <stdlib.h>

#define mMALLOC(sz,tp)		(tp*)malloc((sz)*sizeof(tp))
#define mREALLOC(var,sz,tp)	(tp*)realloc(var,(sz)*sizeof(tp))
#define mFREE(ptr)		{ if (ptr) free(ptr); ptr = 0; }


#endif
