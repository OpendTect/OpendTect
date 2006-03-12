#ifndef commondefs_h
#define commondefs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: commondefs.h,v 1.1 2006-03-12 13:39:09 cvsbert Exp $
________________________________________________________________________

 Some very commonly used macros.

-*/

#define mSWAP(x,y,tmp)		{ tmp = x; x = y; y = tmp; }
#define mNINT(x)		( (int)((x)>0 ? (x)+.5 : (x)-.5) )
#define mMAX(x,y)		( (x)>(y) ? (x) : (y) )
#define mMIN(x,y)		( (x)<(y) ? (x) : (y) )

#define mIsZero(x,eps)		( (x) < (eps) && (x) > (-eps) )
#define mIsEqual(x,y,eps)	( (x-y) < (eps) && (x-y) > (-eps) )
#define mIsEqualRel(x,y,e)	( (y) ? ((x)/(y))-1<(e) && ((x)/(y)-1)>(-e) \
				      : mIsZero(x,e) )
#define mDefEps			(1e-10)

#define mUndefValue		_obsolete_mUndefValue
#define mIsUndefined(x)		_obsolete_mIsUndefined
#define sUndefValue		_obsolete_sUndefValue
#define mUndefIntVal		_obsolete_mUndefIntVal
#define mIsUndefInt(x)		_obsolete_mIsUndefInt

#define mMALLOC(sz,tp)		_include_mallocdefs_mMALLOC
#define mREALLOC(var,sz,tp)	_include_mallocdefs_mREALLOC
#define mFREE(ptr)		_include_mallocdefs_mFREE

#undef	YES
#undef	NO
#undef PATH_LENGTH
#undef mMaxUserIDLength

#define	YES 1
#define	NO 0
#define mMaxUserIDLength 127

#ifdef __win__
# include <stdio.h>
# undef small
#endif

#ifdef __msvc__
#include "msvcdefs.h"
#else

#define PATH_LENGTH		255

# define mPolyRet(base,clss)	clss
# define mTFriend(T,clss)	template <class T> friend class clss
# define mTTFriend(T,C,clss)	template <class T, class C> friend class clss
# define mProtected		protected
# define mPolyRetDownCast(clss,var)	var
# define mPolyRetDownCastRef(clss,var)	var
# define mDynamicCastGet(typ,out,in)	typ out = dynamic_cast< typ >( in );

#endif


#endif
