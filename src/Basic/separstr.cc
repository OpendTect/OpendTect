/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 11-4-1994
 * FUNCTION : Functions concerning comma separated string lists
-*/

static const char* rcsID = "$Id: separstr.cc,v 1.3 2000-09-27 16:04:49 bert Exp $";

#include <string.h>
#include <stdlib.h>
#include "separstr.h"
#include "string2.h"

static char buf[mMaxSepItem+1];

/*$-*/

SeparString::SeparString( const char* str, char separ )
{
    rep = str ? str : "";
    sep = separ;
    sepstr[0] = separ;
    sepstr[1] = '\0';
}


const char* SeparString::operator[]( unsigned int elemnr ) const
{
    char* bufptr = buf;
    char* repptr = rep;
    buf[0] = '\0';

    while ( *repptr )
    {
	if ( !elemnr )
	    *bufptr = *repptr;

	if ( *repptr == sep )
	{
	    if ( --elemnr == -1 || bufptr-buf == mMaxSepItem )
	    {
		*bufptr = '\0';
		return buf;
	    }
	}
	else if ( !elemnr )
	    bufptr++;

	repptr++;
    }

    *bufptr = '\0';
    return buf;
}


const char* SeparString::from( unsigned int idx ) const
{
    const char* ptr = rep;
    for ( ; idx!=0; idx-- )
    {
	ptr = strchr( ptr, sep );
	if ( ptr ) ptr++;
    }
    return ptr;
}


void SeparString::add( const char* str )
{
    rep += sepstr;
    rep += str;
}



SeparString& SeparString::operator += ( const char* str )
{
    if ( str )
    {
	if ( *rep ) rep += sepstr;
	rep += str;
    }
    return *this;
}


unsigned int SeparString::size() const
{
    if ( !*rep ) return 0;

    unsigned int idx = *rep == sep ? 1 : 0;
    char* ptr = rep;
    while ( ptr )
    {
	idx++;
	ptr = strchr( ptr+1, sep );
    }

    return idx;
}


SeparString& SeparString::operator +=( int i )
{
    *this += getStringFromInt( 0, i );
    return *this;
}


SeparString& SeparString::operator +=( float f )
{
    *this += getStringFromFloat( 0, f );
    return *this;
}


SeparString& SeparString::operator +=( double d )
{
    *this += getStringFromDouble( 0, d );
    return *this;
}
