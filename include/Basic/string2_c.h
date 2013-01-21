#ifndef string2_c_h
#define string2_c_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2011
 Contents:	Remainder of string2.h that has to be available for C
 RCS:		$Id$
________________________________________________________________________
-*/

#include "basicmod.h"
#include "gendefs.h"
#include <string.h>
#include <ctype.h>

/*!
\brief Advances given pointer to first non-whitespace.
*/

#define mSkipBlanks(ptr) \
    { if ( ptr ) { while ( *(ptr) && isspace(*(ptr)) ) (ptr)++; } }

/*!
\brief Advances given pointer to first whitespace.
*/

#define mSkipNonBlanks(ptr) \
    { if ( ptr ) { while ( *(ptr) && !isspace(*(ptr)) ) (ptr)++; } }

/*!
\brief Advances to first whitespace and removes trailing whitespace.
*/

#define mTrimBlanks(ptr) \
    { mSkipBlanks(ptr); C_removeTrailingBlanks(ptr); }

mGlobal(Basic) void C_removeTrailingBlanks(char*);
mGlobal(Basic) int C_caseInsensitiveEqual(const char*,const char*,
				     int nr_chars_to_match_0_is_all);
mGlobal(Basic) void C_replaceCharacter(char*,char from,char to);


#endif

