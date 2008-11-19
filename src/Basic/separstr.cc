/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 11-4-1994
 * FUNCTION : Functions concerning delimiter separated string lists
-*/

static const char* rcsID = "$Id: separstr.cc,v 1.14 2008-11-19 20:24:24 cvsbert Exp $";

#include <string.h>
#include <stdlib.h>
#include "separstr.h"
#include "convert.h"
#include "string2.h"

SeparString::SeparString( const char* str, char separ )
    : rep_(str)
{
    sep_[0] = separ; sep_[1] = '\0';
}


SeparString& SeparString::operator =( const SeparString& ss )
{
    if ( this != &ss )
    {
	rep_ = ss.rep_;
	sep_[0] = ss.sep_[0];
    }
    return *this;
}


SeparString& SeparString::operator =( const char* s )
{
    if ( s == rep_.buf() ) return *this;

    rep_.setEmpty();
    add( s );
    return *this;
}


void SeparString::setSepChar( char newchar )
{
    SeparString ss( 0, newchar );
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	ss.add( (*this)[idx] );
    *this = ss;
}


const char* SeparString::operator[]( int elemnr ) const
{
    static const char* emptystr = "";
    if ( elemnr < 0 ) return emptystr;

    const char* startptr = rep_.buf();
    while ( *startptr )
    {
	const char* endptr = strchr( startptr, sep_[0] );
	if ( !elemnr || !endptr )
	{
	    if ( elemnr ) return emptystr;
	    if ( !endptr )
		return startptr;

	    const int retlen = (int)(endptr - startptr);
	    if ( retlen < 1 ) return emptystr;
	    static BufferString ret;
	    ret.setBufSize( retlen + 1 );
	    char* ptr = ret.buf();
	    while ( startptr != endptr )
		*ptr++ = *startptr++;
	    *ptr = '\0';
	    return ret.buf();
	}

	elemnr--;
	startptr = endptr+1;
    }

    return emptystr;
}


#define mDeclGetFn(typ,fn) \
typ SeparString::fn( int idx ) const \
{ \
    return Conv::to<typ>( (*this)[idx] ); \
}
mDeclGetFn(int,getIValue)
mDeclGetFn(od_uint32,getUIValue)
mDeclGetFn(od_int64,getI64Value)
mDeclGetFn(od_uint64,getUI64Value)
mDeclGetFn(float,getFValue)
mDeclGetFn(double,getDValue)
mDeclGetFn(bool,getYN)


const char* SeparString::from( int idx ) const
{
    const char* ptr = rep_.buf();
    for ( ; idx!=0; idx-- )
    {
	ptr = strchr( ptr, sep_[0] );
	if ( ptr ) ptr++;
    }
    return ptr;
}


void SeparString::add( const char* str )
{
    if ( !str || !*str ) str = " ";

    if ( *rep_.buf() ) rep_ += sep_;
    rep_ += str;
}


int SeparString::size() const
{
    if ( !*rep_.buf() ) return 0;

    int sz = *rep_.buf() == sep_[0] ? 1 : 0;
    const char* ptr = rep_.buf();
    while ( ptr )
    {
	sz++;
	ptr = strchr( ptr+1, sep_[0] );
    }

    return sz;
}


#define mDeclAddFn(typ) \
SeparString& SeparString::operator +=( typ v ) \
{ \
    *this += Conv::to<const char*>( v ); \
    return *this; \
}

mDeclAddFn(int)
mDeclAddFn(od_uint32)
mDeclAddFn(od_int64)
mDeclAddFn(od_uint64)
mDeclAddFn(float)
mDeclAddFn(double)
mDeclAddFn(bool)
