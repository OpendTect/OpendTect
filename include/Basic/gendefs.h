#ifndef gendefs_H
#define gendefs_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		1-9-1995
 Contents:	General definitions for every module
 RCS:		$Id: gendefs.h,v 1.6 2001-05-02 13:49:56 windev Exp $
________________________________________________________________________

-*/

/*! There are only #define lines, so there is no C-language constraint. */

#include <Pmacros.h>

#define mMaxUserIDLength	127
#define mMaxUnitIDLength	60

#ifdef PATH_LENGTH
# undef PATH_LENGTH
#endif
#define PATH_LENGTH		255

#define mSWAP(x,y,tmp)		{ tmp = x; x = y; y = tmp; }
#define mNINT(x)		( (int)((x)>0 ? (x)+.5 : (x)-.5) )
#define mMAX(x,y)		( (x)>(y) ? (x) : (y) )
#define mMIN(x,y)		( (x)<(y) ? (x) : (y) )

#ifndef mEPSILON
#define mEPSILON		(1e-10)
#endif
#define mIS_ZERO(x)		( (x) < (mEPSILON) && (x) > (-mEPSILON) )

#define mUndefValue		1e30
#define mIsUndefined(x)		(((x)>9.99999e29)&&((x)<1.00001e30))
#define sUndefValue		"1e30"

#define sMinimumKey		"Minimum"
#define sMaximumKey		"Maximum"

#undef	YES
#define	YES	1
#undef	NO
#define	NO	0

#define Fail	0
#define Ok	1

#define mMALLOC(sz,tp)		(tp*)malloc((sz)*sizeof(tp))
#define mREALLOC(var,sz,tp)	(tp*)realloc(var,(sz)*sizeof(tp))
#define mFREE(ptr)		{ if (ptr) free(ptr); ptr = 0; }


#ifdef __msvc__
# define for 				if(0);else for
# define mPolyRet(base,clss)		base
# define mPolyRetDownCast(clss,var)	dynamic_cast<clss>(var)
# define mTFriend(T,clss)
# define mTProtected			public
#else
# define mPolyRet(base,clss)		clss
# define mPolyRetDownCast(clss,var)	var
# define mTFriend(T,clss)		template <class T> friend clss<T>
# define mTProtected			protected
#endif

#endif
