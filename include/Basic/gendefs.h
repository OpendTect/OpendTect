#ifndef gendefs_H
#define gendefs_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		1-9-1995
 Contents:	General definitions for every module
 RCS:		$Id: gendefs.h,v 1.24 2004-01-21 15:19:02 bert Exp $
________________________________________________________________________

-*/

/*! There are only #define lines, so there is no C-language constraint. */

#define mODMajorVersion		1
#define mODMinorVersion		1

#include <plfdefs.h>

#define mMaxUserIDLength	127

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
#define mUndefIntVal		2109876543

#undef	YES
#define	YES	1
#undef	NO
#define	NO	0

#define Fail	NO
#define Ok	YES

#define mMALLOC(sz,tp)		(tp*)malloc((sz)*sizeof(tp))
#define mREALLOC(var,sz,tp)	(tp*)realloc(var,(sz)*sizeof(tp))
#define mFREE(ptr)		{ if (ptr) free(ptr); ptr = 0; }


#ifdef PATH_LENGTH
# undef PATH_LENGTH
#endif


#ifdef __cpp__

     namespace		std {}
     using namespace	std;

#endif

#ifdef __win__
# include <stdio.h>
#endif

#ifdef __msvc__

# include <stdlib.h>
# include <windefs.h>
# define PATH_LENGTH			_MAX_PATH

# define mPolyRet(base,clss)		base
# define mTFriend(T,clss)
# define mTTFriend(T,C,clss)
# define mProtected			public
# define mPolyRetDownCast(clss,var)	dynamic_cast<clss>(var)
# define mPolyRetDownCastRef(clss,var)	*(dynamic_cast<clss*>(&var))
# define mDynamicCastGet(typ,out,in) \
	 typ out = 0; try { out = dynamic_cast< typ >( in ); } catch (...) {}

#else

#define PATH_LENGTH			255

# define mPolyRet(base,clss)		clss
# define mTFriend(T,clss)		template <class T> friend class clss
# define mTTFriend(T,C,clss)	template <class T, class C> friend class clss
# define mProtected			protected
# define mPolyRetDownCast(clss,var)	var
# define mPolyRetDownCastRef(clss,var)	var
# define mDynamicCastGet(typ,out,in)	typ out = dynamic_cast< typ >( in );

#endif

#endif
