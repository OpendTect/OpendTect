#ifndef commondefs_h
#define commondefs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id$
________________________________________________________________________

 Some very commonly used macros.

-*/

#include "basicmod.h"
#include "plfdefs.h"
#include "rounding.h"

#define mRounded(typ,x)		roundOff<typ>( x )
#define mNINT32(x)		mRounded( od_int32, x )
#define mNINT64(x)		mRounded( od_int64, x )
#define mCast(tp,v)		((tp)(v))

#define mSWAP(x,y,tmp)		{ tmp = x; x = y; y = tmp; }
#define mMAX(x,y)		( (x)>(y) ? (x) : (y) )
#define mMIN(x,y)		( (x)<(y) ? (x) : (y) )

#define mIsZero(x,eps)		( (x) < (eps) && (x) > (-eps) )
#define mIsEqual(x,y,eps)	( (x-y) < (eps) && (x-y) > (-eps) )
#define mIsEqualRel(x,y,e)	( (y) ? ((x)/(y))-1<(e) && ((x)/(y)-1)>(-e) \
				      : mIsZero(x,e) )
#define mIsEqualWithUdf(x,y,e)	((mIsUdf(x) && mIsUdf(y)) || mIsEqual(x,y,e) )
#define mDefEpsF		(1e-10f)
#define mDefEpsD		(1e-10)
#define mDefEps			mDefEpsD

# define mC_True	1
# define mC_False	0

#ifndef M_PI
# define M_PI		3.14159265358979323846
#endif

#ifndef M_PI_2
# define M_PI_2		1.57079632679489661923
#endif

#ifndef M_PI_4
# define M_PI_4		0.78539816339744830962
#endif

#ifndef M_SQRT1_2
# define M_SQRT1_2	0.70710678118654752440
#endif

#ifndef MAXFLOAT
# define MAXFLOAT	3.4028234663852886e+38F
#endif

#ifndef MAXDOUBLE
# define MAXDOUBLE	1.7976931348623157e+308
#endif

#ifndef MAXSIZE
# define MAXSIZE	((size_t)-1)
#endif

#ifdef __win__
# include <stdio.h>
# undef small
#endif


#define mFromFeetFactorF	0.3048f
#define mFromFeetFactorD	0.3048
#define mToFeetFactorF		3.2808399f
#define mToFeetFactorD		3.28083989501312336
#define mToSqMileFactor		0.3861 			//km^2 to mile^2
#define mMileToFeetFactor	5280


#ifdef __msvc__
# include "msvcdefs.h"
#else
# define dll_export
# define dll_import


# define mMaxFilePathLength	255

# define mPolyRet(base,clss)	clss
# define mTFriend(T,clss)	template <class T> friend class clss
# define mTTFriend(T,C,clss)	template <class T, class C> friend class clss
# define mProtected		protected
# define mPolyRetDownCast(clss,var)	var
# define mPolyRetDownCastRef(clss,var)	var
# define mDynamicCast(typ,out,in)	out = dynamic_cast< typ >( in );
# define mDynamicCastGet(typ,out,in)	typ mDynamicCast(typ,out,in)

#endif

#define mTODOHelpID	"0.0.0"
#define mNoHelpID	"-"

#define mExp( module )		dll_export
//#define mExp( module )		Export_##module

#define mExpClass( module )		class mExp( module )
#define mExpStruct( module )		struct mExp( module )

#define mGlobal( module )		mExp( module )
#define mClass( module )		mExpClass( module )
#define mStruct( module )		mExpStruct( module )
#define mExtern( module )		extern mExp( module )
#define mExternC( module)		extern "C" mExp( module )

#define mExportTemplateInst( mod, clss, inst ) \
template <class T> class clss; \
template mExp(mod) class clss<inst>

//for Qt
#ifndef QT_NAMESPACE
# define mFDQtclass(cls) class cls;
# define mQtclass(cls) cls
# define mUseQtnamespace
#else
# define mFDQtclass(cls) namespace QT_NAMESPACE { class cls; }
# define mQtclass(cls) ::QT_NAMESPACE::cls
# define mUseQtnamespace using namespace ::QT_NAMESPACE;
#endif

#define mIfNotFirstTime(act) \
    static bool _already_visited_ = false; \
    if ( _already_visited_ ) act; \
    _already_visited_ = true

// Helps keep 4.4 compatibility
#define mDefClass( module )	mClass(module)


#endif


