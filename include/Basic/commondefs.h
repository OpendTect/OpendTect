#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2006
________________________________________________________________________

 Some very commonly used macros.

-*/

#include "basicmod.h"
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
inline mDeprecated void Swap( T& a, T& b )	{ std::swap(a,b); }

template <class T>
inline T* getNonConst( const T* t )
{ return const_cast<T*>( t ); }

template <class T>
inline T& getNonConst( const T& t )
{ return const_cast<T&>( t ); }


#define mRounded(typ,x)		roundOff<typ>( x )
#define mNINT32(x)		mRounded( od_int32, x )
#define mNINT64(x)		mRounded( od_int64, x )

#define mMAX(x,y)		((x)>(y) ? (x) : (y))
#define mMIN(x,y)		((x)<(y) ? (x) : (y))
#define mLimited(v,min,max)	getLimited( v, min, max )

#define mIsZero(x,eps)		isFPZero( x, eps )
#define mIsEqual(x,y,eps)	isFPEqual( x, y, eps )
#define mIsEqualWithUdf(x,y,e)	((mIsUdf(x) && mIsUdf(y)) || mIsEqual(x,y,e) )
#define mDefEpsF		(1e-10f)
#define mDefEpsD		(1e-10)
#define mDefEps			mDefEpsD

//! Use if you do not know exactly how many, but still it's more than 1
#define mPlural 42


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
#ifndef M_EULER
#  define M_EULER	2.7182818284590452353602874713
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
#ifndef M_EULERf
#  define M_EULERf	2.7182818284590452353602874713f
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

#define mCast(tp,v)	((tp)(v))
#define cCast(tp,v)	const_cast< tp >( v )
#define dCast(tp,v)	dynamic_cast< tp >( v )
#define rCast(tp,v)	reinterpret_cast< tp >( v )
#define sCast(tp,v)	static_cast< tp >( v )

#define mNonConst(x)	getNonConst( x )
#define mSelf()		mNonConst( *this )

#ifdef __win32__
# define mUseType(scope,typ)		typedef scope::typ typ
#else
# define mUseType(scope,typ)		typedef typename scope::typ typ
#endif
//In template classes:
#define mUseTemplType(scope,typ)	typedef typename scope::typ typ

#define mGetDynamicCast(typ,in)	dynamic_cast< typ >( in );
#define mDynamicCast(typ,out,in)	out = dynamic_cast< typ >( in );
#define mDynamicCastGet(typ,out,in)	typ mDynamicCast(typ,out,in)

#define mDefSetupClssMemb(clss,typ,memb) \
	typ	memb##_; \
	clss&   memb( typ val )		{ memb##_ = val; return *this; }
#define mDefSetupMemb(typ,memb) mDefSetupClssMemb(Setup,typ,memb)

#define mDefSetupClssMember(clss,typ,memb,def) \
	typ	memb##_ = def; \
	clss&   memb( typ val )		{ memb##_ = val; return *this; }
#define mDefSetupMember(typ,memb,def) mDefSetupClssMember(Setup,typ,memb,def)

#define mDefNoAssignmentOper(clss) \
    clss& operator =( const clss& ) = delete;

#define mImplSimpleIneqOper(clss) \
    inline bool operator !=( const clss& oth ) const { return !(*this==oth); }

#define mImplSimpleEqOpers1Memb(clss,memb) \
    inline bool operator ==( const clss& oth ) const { return this->memb==oth.memb; }\
    mImplSimpleIneqOper( clss )

#define mImplSimpleEqOpers2Memb(clss,memb1,memb2) \
    inline bool operator ==( const clss& oth ) const \
    { return this->memb1 == oth.memb1 && this->memb2 == oth.memb2; } \
    mImplSimpleIneqOper( clss )

#define mImplSimpleEqOpers3Memb(clss,memb1,memb2,memb3) \
    inline bool operator ==( const clss& oth ) const \
    { return this->memb1 == oth.memb1 && this->memb2 == oth.memb2 \
	  && this->memb3 == oth.memb3; } \
    mImplSimpleIneqOper( clss )

#define mImplSimpleEqOpers4Memb(clss,memb1,memb2,memb3,memb4) \
    inline bool operator ==( const clss& oth ) const \
    { return this->memb1 == oth.memb1 && this->memb2 == oth.memb2 \
	  && this->memb3 == oth.memb3 && this->memb4 == oth.memb4; } \
    mImplSimpleIneqOper( clss )



//--- Covering Windows problems, mainly DLL export/import stuff

#ifdef __win__
# include <stdio.h>
# undef small
# ifndef __func__
#  define __func__ __FUNCTION__
# endif
#define NOMINMAX
#endif

#ifndef __msvc__

# define dll_export
# define dll_import
# define mMaxFilePathLength	255

#else

# define mMaxFilePathLength	_MAX_PATH

#ifndef __debug__
 // Debug mode is a simple switch in VS, need to support that:
# ifdef _DEBUG
#  define __debug__
# endif
#endif

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


namespace OD
{
    //!< An empty string that shows the world it's empty
    mGlobal(Basic) inline const char* EmptyString() { return ""; }
}


#define mDefineStaticLocalObject( type, var, init ) \
static type var init; \


//--- Single-shot initialization support

namespace Threads
{
    mGlobal(Basic) bool atomicSetIfValueIs(volatile int&,int,int,int* =0);
}

//! Macro that does something except the very first time reached. The header file atomic.h must be included as it defines Threads::atomicSetIfValueIs.

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
{ try { var = new stmt; } catch ( std::bad_alloc ) { var = 0; } }

#define mTryAllocPtrMan(var,stmt) \
{ try { var = new stmt; } catch ( std::bad_alloc ) { var.set( 0 ); } }

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
			 Somewhat quicker than for-loops.

  Instead of:
    for ( int idx=0; idx<arrsz; idx++ )
	arr[idx] /= 5;

  You can do:
    mDoArrayPtrOperation( float, arr, /= 5, arrsz, ++ );

*/

#define mArrayPtrOperationCurPtrVar __curptr
#define mDoArrayPtrOperation( type, arr, operation, arrsz, ptrinc ) \
{ \
    type* mArrayPtrOperationCurPtrVar = arr; \
    for ( const type* __stopptr = mArrayPtrOperationCurPtrVar + arrsz; \
	  mArrayPtrOperationCurPtrVar != __stopptr; \
	  mArrayPtrOperationCurPtrVar ptrinc ) \
    { \
	*mArrayPtrOperationCurPtrVar operation; \
    } \
}
