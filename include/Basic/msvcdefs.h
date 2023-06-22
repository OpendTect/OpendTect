#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include <stdlib.h>

#include <cmath>

#define NOMINMAX	// Otherwise Windows will define min/max

#if defined(_MSC_VER) && _MSC_VER == 1500 // && _MSC_FULL_VER >= 150030729
# define Interval StructuredQueryInterval
#  include <structuredquery.h>
# undef Interval
#endif

#define snprintf	_snprintf
#define isnan		std::isnan

#define strncasecmp	_strnicmp
#define strcasecmp	stricmp

#define strtoll		_strtoi64
#define strtoull	_strtoui64
#define strdup		_strdup
#define strtof		strtod

#define finite		_finite
#define atoll		_atoi64

# define mMaxFilePathLength		_MAX_PATH
