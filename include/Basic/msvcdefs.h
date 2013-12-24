#ifndef msvcdefs_h
#define msvcdefs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id$
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

#endif
