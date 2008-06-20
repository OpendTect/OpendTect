/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 11-4-1994
 * FUNCTION : Functions concerning delimiter separated string lists
-*/

static const char* rcsID = "$Id: separstr.cc,v 1.12 2008-06-20 11:40:57 cvsraman Exp $";

#include <string.h>
#include <stdlib.h>
#include "separstr.h"
#include "string2.h"

SeparString::SeparString( const char* str, char separ )
{
    rep_ = str ? str : "";
    setSepChar( separ );
}


void SeparString::setSepChar( char separ )
{
    sep_ = separ;
    sepstr_[0] = separ;
    sepstr_[1] = '\0';
}


const char* SeparString::operator[]( unsigned int elemnr ) const
{
    static char buf_[mMaxSepItem+1];
    char* bufptr = buf_;
    const char* repptr = rep_;
    buf_[0] = '\0';

    while ( *repptr )
    {
	if ( !elemnr )
	    *bufptr = *repptr;

	if ( *repptr == sep_ )
	{
	    if ( !elemnr || bufptr-buf_ == mMaxSepItem )
	    {
		*bufptr = '\0';
		return buf_;
	    }
	    elemnr--;
	}
	else if ( !elemnr )
	    bufptr++;

	repptr++;
    }

    *bufptr = '\0';
    return buf_;
}


const char* SeparString::from( unsigned int idx ) const
{
    const char* ptr = rep_;
    for ( ; idx!=0; idx-- )
    {
	ptr = strchr( ptr, sep_ );
	if ( ptr ) ptr++;
    }
    return ptr;
}


void SeparString::add( const char* str )
{
    if ( str )
    {
	if ( *rep_ ) rep_ += sepstr_;
	rep_ += str;
    }
}



SeparString& SeparString::operator += ( const char* str )
{
    add( str );
    return *this;
}


unsigned int SeparString::size() const
{
    if ( !*rep_ ) return 0;

    unsigned int idx = *rep_ == sep_ ? 1 : 0;
    const char* ptr = rep_;
    while ( ptr )
    {
	idx++;
	ptr = strchr( ptr+1, sep_ );
    }

    return idx;
}


SeparString& SeparString::operator +=( int i )
{
    *this += getStringFromInt( i );
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


void SeparString::replaceSepChar( char newchar )
{
    SeparString newstr( 0, newchar );
    for ( int idx=0; idx<size(); idx++ )
	newstr.add( (*this)[idx] );

    rep_ = newstr.buf();
    setSepChar( newchar );
}


char SeparString::getPathSepChar()
{
#ifdef __win__
    return ';';
#else
    return ':';
#endif
}
