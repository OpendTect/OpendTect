#ifndef msvcdefs_h
#define msvcdefs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: msvcdefs.h,v 1.13 2012-08-01 11:47:26 cvsranojay Exp $
________________________________________________________________________

 For use with Microsoft Visual C++ 8.0 and 9.0

-*/

#include <stdlib.h>

#define NOMINMAX	// Otherwise Windows will define min/max

#if defined(_MSC_VER) && _MSC_VER == 1500 // && _MSC_FULL_VER >= 150030729
# define Interval StructuredQueryInterval
#  include <structuredquery.h>
# undef Interval
#endif

#ifdef _DEBUG
# define __debug__
#endif

#define snprintf	_snprintf
#define isnan		_isnan

#define strncasecmp	strnicmp
#define strcasecmp	stricmp

#define strtoll		_strtoi64
#define strtoull	_strtoui64
#define strtof		strtod

#define finite		_finite
#define atoll		_atoi64

# define mMaxFilePathLength		_MAX_PATH

# define mPolyRet(base,clss)		base
# define mTFriend(T,clss)
# define mTTFriend(T,C,clss)
# define mProtected			public
# define mPolyRetDownCast(clss,var)	dynamic_cast<clss>(var)
# define mPolyRetDownCastRef(clss,var)	*(dynamic_cast<clss*>(&var))
# define mDynamicCast(typ,out,in) \
	 out = dynamic_cast< typ >( in );
# define mDynamicCastGet(typ,out,in) \
	 typ mDynamicCast(typ,out,in)

#pragma warning( disable : 4355 4003 )

#pragma warning(2:4032)     // function arg has different type from declaration
#pragma warning(2:4092)     // 'sizeof' value too big
#pragma warning(2:4132 4268)// const object not initialized
#pragma warning(2:4152)     // pointer conversion between function and data
#pragma warning(2:4239)     // standard doesn't allow this conversion
//#pragma warning(2:4701)     // local variable used without being initialized
#pragma warning(2:4706)     // if (a=b) instead of (if a==b)
#pragma warning(2:4709)     // comma in array subscript
//#pragma warning(3:4061)     // not all enum values tested in switch statement
//#pragma warning(3:4710)     // inline function was not inlined
#pragma warning(3:4121)     // space added for structure alignment
#pragma warning(3:4505)     // unreferenced local function removed
#pragma warning(3:4019)     // empty statement at global scope
#pragma warning(3:4057)     // pointers refer to different base types
#pragma warning(3:4125)     // decimal digit terminates octal escape
#pragma warning(2:4131)     // old-style function declarator
#pragma warning(3:4211)     // extern redefined as static
#pragma warning(3:4213)     // cast on left side of = is non-standard
#pragma warning(3:4222)     // member function at file scope shouldn't be static
#pragma warning(3:4234 4235)// keyword not supported or reserved for future
#pragma warning(3:4504)     // type ambiguous; simplify code
#pragma warning(3:4507)     // explicit linkage specified after default linkage
#pragma warning(3:4515)     // namespace uses itself
#pragma warning(3:4516 4517)// access declarations are deprecated
#pragma warning(3:4670)     // base class of thrown object is inaccessible
#pragma warning(3:4671)     // copy ctor of thrown object is inaccessible
#pragma warning(3:4673)     // thrown object cannot be handled in catch block
#pragma warning(3:4674)     // dtor of thrown object is inaccessible
#pragma warning(3:4705)     // statement has no effect (example: a+1;)


#endif
