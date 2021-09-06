#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
________________________________________________________________________

 Some very commonly used macros.

-*/

#include "basicmod.h"
#include "odversion.h"
#include "plfdefs.h"

#include <stdexcept>
#include <new>


//--- FP rounding and equality

template <class RT,class T>
inline RT roundOff( T x )	{ return RT(x); }

template <class RT>
inline RT roundOff( double x )	{ return RT((x)>0 ? (x)+.5 : (x)-.5); }

template <class RT>
inline RT roundOff( float x )	{ return RT((x)>0 ? (x)+.5f : (x)-.5f); }

template <class fT,class eT>
inline bool isFPZero( fT v, eT eps )		{ return v < eps && v > -eps; }

template <class T1,class T2,class eT>
inline bool isFPEqual( T1 v1, T2 v2, eT eps )	{ return isFPZero(v1-v2,eps); }

template <class T>
inline T getLimited( T v, T min, T max )
{ return v<min ? min : (v>max ? max : v); }

template <class T>
inline void Swap( T& a, T& b )  { std::swap(a,b); }

template <class T>
inline T* getNonConst( const T* t )
{ return const_cast<T*>( t ); }

template <class T>
inline T& getNonConst( const T& t )
{ return const_cast<T&>( t ); }


#define mRounded(typ,x)		roundOff<typ>( x )
#define mNINT32(x)		mRounded( od_int32, x )
#define mNINT64(x)		mRounded( od_int64, x )

#define mMAX(x,y)		( (x)>(y) ? (x) : (y) )
#define mMIN(x,y)		( (x)<(y) ? (x) : (y) )
#define mMaxLimited(v,lim)	( (v)<(lim) ? (v) : (lim) )
#define mMinLimited(v,lim)	( (v)>(lim) ? (v) : (lim) )

#define mIsZero(x,eps)		isFPZero( x, eps )
#define mIsEqual(x,y,eps)	isFPEqual( x, y, eps )
#define mIsEqualWithUdf(x,y,e)	((mIsUdf(x) && mIsUdf(y)) || mIsEqual(x,y,e) )
#define mDefEpsF		(1e-10f)
#define mDefEpsD		(1e-10)
#define mDefEps			mDefEpsD


//--- Math-related constants

#ifndef M_PI
# define M_PI		3.14159265358979323846
#endif
#ifndef M_PIl
# define M_PIl          3.1415926535897932384626433832795029L
#endif
#ifndef M_2PI
# define M_2PI		6.28318530717958647692
#endif
#ifndef M_PI_2
# define M_PI_2		1.57079632679489661923
#endif
#ifndef M_PI_4
# define M_PI_4		0.78539816339744830962
#endif
#ifndef M_SQRT2
#  define M_SQRT2       1.41421356237309504880168872421
#endif
#ifndef M_SQRT1_2
# define M_SQRT1_2	0.70710678118654752440
#endif
#ifndef M_PIf
# define M_PIf		3.14159265358979323846f
#endif
#ifndef M_2PIf
# define M_2PIf		6.28318530717958647692f
#endif
#ifndef M_PI_2f
# define M_PI_2f	1.57079632679489661923f
#endif
#ifndef M_PI_4f
# define M_PI_4f	0.78539816339744830962f
#endif
#ifndef M_SQRT2f
#  define M_SQRT2f      1.41421356237309504880168872421f
#endif
#ifndef M_SQRT1_2f
# define M_SQRT1_2f	0.70710678118654752440f
#endif

#ifndef MAXFLOAT
# define MAXFLOAT	3.4028234663852886e+38F
#endif
#ifndef MAXDOUBLE
# define MAXDOUBLE	1.7976931348623157e+308
#endif

#define mFromFeetFactorF	0.3048f
#define mFromFeetFactorD	0.3048
#define mToFeetFactorF		3.2808399f
#define mToFeetFactorD		3.28083989501312336
#define mToPercent(f)		(mIsUdf(f) ? f : f*100)
#define mFromPercent(f)		(mIsUdf(f) ? f : f*0.01)
#define mDeg2RadD		0.017453292519943292
#define mRad2DegD		57.295779513082323
#define mDeg2RadF		0.017453292519943292f
#define mRad2DegF		57.295779513082323f

//--- File size-related constants

#define mDef1MB 1048576ULL
#define mDef128MB 134217728ULL
#define mDef1GB 1073741824ULL
#define mDef4GB 4294967296ULL
#define mDef32GB 34359738368ULL

//--- C++ tools

#define mCast(tp,v)		((tp)(v))
#define cCast(tp,v)		const_cast< tp >( v )
#define dCast(tp,v)		dynamic_cast< tp >( v )
#define rCast(tp,v)		reinterpret_cast< tp >( v )
#define sCast(tp,v)		static_cast< tp >( v )

#define mNonConst(x)    getNonConst( x )
#define mSelf()         mNonConst( *this )

#define mUseType(scope,typ)		typedef scope::typ typ
# define mDynamicCast(typ,out,in)	out = dynamic_cast< typ >( in );
# define mDynamicCastGet(typ,out,in)	typ mDynamicCast(typ,out,in)

#define mDefSetupClssMemb(clss,typ,memb) \
	typ	memb##_; \
	clss&   memb( typ val )		{ memb##_ = val; return *this; }

#define mDefSetupMemb(typ,memb) mDefSetupClssMemb(Setup,typ,memb)



//--- Covering Windows problems, mainly DLL export/import stuff

#ifdef __win__
# include <stdio.h>
# undef small
# ifndef __func__
#  define __func__ __FUNCTION__
# endif
#endif

#ifdef __msvc__
# include "msvcdefs.h"
#else
# define dll_export
# define dll_import
# define mMaxFilePathLength	255
#endif

#define mExp( module )			Export_##module
#define mExpClass( module )		class mExp( module )
#define mExpStruct( module )		struct mExp( module )

#define mGlobal( module )		mExp( module )
#define mClass( module )		class
#define mStruct( module )		mExpStruct( module )
#define mExtern( module )		extern mExp( module )
#define mExternC( module)		extern "C" mExp( module )

#define mExportInst( mod, tp )		Extern_##mod tp mExp(mod)
#define mExportTemplClassInst(mod)	mExportInst(mod,template class)


//--- Local static variable initialization. This is an MT Windows problem.

#ifdef __win__
namespace Threads
{
    mGlobal(Basic) bool lockSimpleSpinWaitLock(volatile int& lock);
    mGlobal(Basic) void unlockSimpleSpinLock(volatile int& lock);
}

#define mLockStaticInitLock( nm )

#define mUnlockStaticInitLock( nm )

#else

#define mLockStaticInitLock( nm )
#define mUnlockStaticInitLock( nm )

#endif

#define mDefineStaticLocalObject( type, var, init ) \
static type var init;


//--- Single-shot initialization support

namespace Threads
{
#if mODVersion < 700
    mGlobal(Basic) bool atomicSetIfValueIs(volatile int&,int,int,int* );
    //Force developer to use three arguments
#else
    mGlobal(Basic) bool atomicSetIfValueIs(volatile int&,int,int,int* = 0 );
#endif
}

//! Macro that does something except the very first time reached

#define mIfNotFirstTime(act) \
{ \
    static volatile int _already_visited_ = 0; \
    if ( !Threads::atomicSetIfValueIs( _already_visited_, \
				       0, 1, 0 ) ) \
	act; \
}


//--- Qt class and namespace handling

#ifndef QT_NAMESPACE
# define mFDQtclass(cls) class cls;
# define mQtclass(cls) ::cls
# define mUseQtnamespace
#else
# define mFDQtclass(cls) namespace QT_NAMESPACE { class cls; }
# define mQtclass(cls) ::QT_NAMESPACE::cls
# define mUseQtnamespace using namespace ::QT_NAMESPACE;
#endif


//--- Large array allocation

//! Catches bad_alloc and sets ptr to null as normal.
#define mTryAlloc(var,stmt) \
{ try { var = new stmt; } catch ( std::bad_alloc ) { var = nullptr; } }

#define mTryAllocPtrMan(var,stmt) \
{ try { var = new stmt; } catch ( std::bad_alloc ) { var.set( nullptr ); } }

//!Creates variable, try to alloc and catch bad_alloc.
#define mDeclareAndTryAlloc(tp,var,stmt) \
    tp var; \
    mTryAlloc(var,stmt)

//!Creates new array of an integer type filled with index
#define mGetIdxArr(tp,var,sz) \
    tp* var; \
    mTryAlloc(var,tp [sz]) \
    if ( var ) \
	for  ( tp idx=0; idx<sz; idx++ ) \
	    var[idx] = idx



/*!\ingroup Basic \brief Applies an operation to all members in an array.
			 Quicker than for-loops.

  Instead of:
  \code
    for ( int idx=0; idx<arrsz; idx++ )
	arr[idx] /= 5;
  \endcode

  You can do:
  \code
    mDoArrayPtrOperation( float, arr, /= 5, arrsz, ++ );
  \endcode

  Note that the last '++' is applied to the 'current' pointer called __curptr.

*/

#define mDoArrayPtrOperation( type, arr, operation, arrsz, ptrinc ) \
{ \
    type* __curptr = arr; \
    for ( const type* __stopptr = __curptr + arrsz; \
	  __curptr!=__stopptr; \
	  __curptr ptrinc ) \
    { \
	*__curptr operation; \
    } \
}


#if defined __win32__
#define mMaxContiguousMemSize	0x20000000 // 512 MB (OS limit)
#elif defined __win64__
#define mMaxContiguousMemSize	0x800000000 // 32 GB (OS limit)
#elif defined __mac__
#define mMaxContiguousMemSize	0x800000000 // 32 GB (arbitrary)
#else
#define mMaxContiguousMemSize	0x4000000000 // 256 GB (arbitrary)
#endif


// C library functions
#ifdef __msvc__
# define od_sprintf	sprintf_s
# define od_sscanf	sscanf_s
#else
# define od_sprintf	snprintf
# define od_sscanf	sscanf
#endif
