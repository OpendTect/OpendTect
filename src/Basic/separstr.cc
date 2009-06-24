/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 11-4-1994
 * FUNCTION : Functions concerning delimiter separated string lists
-*/

static const char* rcsID = "$Id: separstr.cc,v 1.19 2009-06-24 10:52:12 cvsbert Exp $";

#include <string.h>
#include <stdlib.h>
#include "separstr.h"
#include "convert.h"
#include "string2.h"
#include "bufstringset.h"

#ifdef __msvc__
# include <iostream>
#endif


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
    if ( s != rep_.buf() )
	{ rep_.setEmpty(); addStr( s ); }
    return *this;
}


int SeparString::size() const
{
    //TODO handle escaped sep_
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


const char* SeparString::operator[]( int elemnr ) const
{
    //TODO handle escaped sep_
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


SeparString& SeparString::operator +=( const BufferStringSet& bss )
{
    for ( int idx=0; idx<bss.size(); idx++ )
	*this += bss.get( idx );
    return *this;
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


int SeparString::indexOf( const char* str ) const
{
    if ( !str ) return -1;
    //TODO handle escaped sep_

    const char* startptr = rep_.buf();
    int elemnr = 0;
    while ( *startptr )
    {
	const char* endptr = strchr( startptr, sep_[0] );
	if ( !endptr )
	    return strcmp(str,startptr) ? -1 : elemnr;

	BufferString cmpstr; cmpstr.setBufSize( (int)(endptr - startptr) + 1 );
	char* ptr = cmpstr.buf();
	while ( startptr != endptr )
	    *ptr++ = *startptr++;
	*ptr = '\0';
	if ( cmpstr == str )
	    return elemnr;

	elemnr++;
	startptr = endptr+1;
    }

    return -1;
}


SeparString& SeparString::add( const char* str )
{
    if ( *rep_.buf() ) rep_ += sep_;
    if ( !str || !*str ) str = " ";
    addStr( str );
    return *this;
}


void SeparString::addStr( const char* str )
{
    //TODO escape sep_
    rep_ += str;
}


void SeparString::setSepChar( char newchar )
{
    SeparString ss( "", newchar );
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	ss.add( (*this)[idx] );
    *this = ss;
}


std::ostream& operator <<( std::ostream& s, const SeparString& ss )
{ s << ss.rep(); return s; }

std::istream& operator >>( std::istream& s, SeparString& ss )
{ s >> ss.rep(); return s; }
