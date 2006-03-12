#ifndef mallocdefs_h
#define mallocdefs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: mallocdefs.h,v 1.1 2006-03-12 13:39:09 cvsbert Exp $
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
