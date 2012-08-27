#ifndef msvcdefs_h
#define msvcdefs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: msvcdefs.h,v 1.17 2012-08-27 13:16:46 cvskris Exp $
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


#endif
