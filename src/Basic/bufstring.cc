/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: bufstring.cc,v 1.11 2008-03-29 11:15:51 cvsbert Exp $";

#include "bufstring.h"

#include "general.h"
#include <iostream>
#include <stdlib.h>
#include "string2.h"
#include <string.h>


#define mDeclMinlen : minlen_(mMaxUserIDLength+1), buf_( 0 ), len_( 0 )
#define mSimpConstrImpl(pars,impl) \
    BufferString::BufferString pars mDeclMinlen { impl; }

mSimpConstrImpl( (), )
mSimpConstrImpl( (const char* s), if ( s ) *this = s )
mSimpConstrImpl( (int i), *this = i )
mSimpConstrImpl( (float f), *this = f )
mSimpConstrImpl( (double d), *this = d )

BufferString::BufferString( const BufferString& bs )
    mDeclMinlen { *this = bs; }

mSimpConstrImpl( (const char* s1, const char* s2, const char* s3),
		 *this = s1; *this += s2; *this += s3 )
mSimpConstrImpl( (const char* s1, int i, const char* s2),
		 *this = s1; *this += i; *this += s2 )
mSimpConstrImpl( (const char* s1, float f, const char* s2),
		 *this = s1; *this += f; *this += s2 )
mSimpConstrImpl( (const char* s1, double d, const char* s2),
		 *this = s1; *this += d; *this += s2 )


BufferString::~BufferString()
{
    destroy();
}


bool BufferString::operator==( const char* s ) const
{
    if ( !s || !(*s) ) return isEmpty();

    if ( isEmpty() ) return false;

    const char* ptr = buf_;
    while ( *s && *ptr )
	if ( *ptr++ != *s++ ) return false;

    return *ptr == *s;
}


char* BufferString::buf()
{
    if ( !buf_ )
	init();

    return buf_;
}


bool BufferString::isEmpty() const
{ return !buf_ || !(*buf_); }


void BufferString::setEmpty()
{
    if ( len_ != minlen_ )
	{ destroy(); init(); }
    else
	buf_[0] = 0;
}


BufferString& BufferString::operator=( const char* s )
{
    if ( buf_ == s ) return *this;

    if ( !s ) s = "";
    setBufSize( (unsigned int)(strlen(s) + 1) );
    char* ptr = buf_;
    while ( *s ) *ptr++ = *s++;
    *ptr = '\0';
    return *this;
}

BufferString& BufferString::operator=( const BufferString& bs )
{ if ( &bs != this ) *this = bs.buf_; return *this; }

BufferString& BufferString::operator=( int i )
{ if ( buf_ ) *buf_ = '\0'; *this += i; return *this; }

BufferString& BufferString::operator=( float f )
{ if ( buf_ ) *buf_ = '\0'; *this += f; return *this; }

BufferString& BufferString::operator=( double d )
{ if ( buf_ ) *buf_ = '\0'; *this += d; return *this; }


BufferString& BufferString::operator +=( const char* s )
{
    if ( s && *s )
    {
	const unsigned int newsize = strlen(s) + (buf_ ? strlen(buf_) : 0) +1;
	setBufSize( newsize );

	char* ptr = buf_;
	while ( *ptr ) ptr++;
	    while ( *s ) *ptr++ = *s++;
		*ptr = '\0';
    }
    return *this;
}

BufferString& BufferString::operator+=( int i )
{ *this += getStringFromInt(i); return *this; }

BufferString& BufferString::operator+=( float f )
{ *this += getStringFromFloat(0,f); return *this; }

BufferString& BufferString::operator+=( double d )
{ *this += getStringFromDouble(0,d); return *this; }

unsigned int BufferString::size() const	
{
    return buf_ ? strlen(buf_) : 0;
}


void BufferString::setBufSize( unsigned int newlen )
{
    if ( newlen < minlen_ )
	newlen = minlen_;
    else if ( newlen == len_ )
	return;

    if ( minlen_ > 1 )
    {
	int nrminlens = newlen / minlen_;
	if ( newlen % minlen_ ) nrminlens++;
	newlen = nrminlens * minlen_;
    }
    if ( buf_ && newlen == len_ )
	return;

    char* oldbuf = buf_;
    mTryAlloc( buf_, char [newlen] );
    if ( !buf_ )
	{ buf_ = oldbuf; return; }
    else if ( !oldbuf )
	*buf_ = '\0';
    else
    {
	int newsz = (oldbuf ? strlen( oldbuf ) : 0) + 1;
	if ( newsz > newlen )
	{
	    newsz = newlen;
	    oldbuf[newsz-1] = '\0';
	}
	memcpy( buf_, oldbuf, newsz );
    }

    delete [] oldbuf;
    len_ = newlen;
}


void BufferString::setMinBufSize( unsigned int newlen )
{
    if ( newlen < 0 ) newlen = 0;
    const_cast<unsigned int&>(minlen_) = newlen;
    setBufSize( len_ );
}


void BufferString::insertAt( int atidx, const char* str )
{
    const int cursz = size();	// Defined this to avoid weird compiler bug
    if ( atidx >= cursz )	// i.e. do not replace cursz with size()
	{ replaceAt( atidx, str ); return; }
    if ( !str || !*str )
	return;

    if ( atidx < 0 )
    {
	const int lenstr = strlen( str );
	if ( atidx <= -lenstr ) return;
	str += -atidx;
	atidx = 0;
    }

    BufferString rest( buf_ + atidx );
    *(buf_ + atidx) = '\0';
    *this += str;
    *this += rest;
}


void BufferString::replaceAt( int atidx, const char* str, bool cut )
{
    const int strsz = str ? strlen(str) : 0;
    int cursz = size();
    const int nrtopad = atidx - cursz;
    if ( nrtopad > 0 )
    {
	const int newsz = cursz + nrtopad + strsz + 1;
	setBufSize( newsz );
	char* ptr = buf_ + cursz;
	const char* stopptr = buf_ + atidx;
	while ( ptr != stopptr )
	    *ptr++ = ' ';
	buf_[atidx] = '\0';
	cursz = newsz;
    }

    if ( strsz > 0 )
    {
	if ( atidx + strsz >= cursz )
	{
	    cursz = atidx + strsz + 1;
	    setBufSize( cursz );
	    buf_[cursz-1] = '\0';
	}

	for ( int idx=0; idx<strsz; idx++ )
	{
	    const int replidx = atidx + idx;
	    if ( replidx >= 0 )
		buf_[replidx] = *(str + idx);
	}
    }

    if ( cut )
    {
	setBufSize( atidx + strsz + 1 );
	buf_[atidx + strsz] = '\0';
    }
}


bool BufferString::operator >( const char* s ) const
{ return s ? strcmp(buf_,s) > 0 : true; }


bool BufferString::operator <( const char* s ) const
{ return s ? strcmp(buf_,s) < 0 : false; }


const BufferString& BufferString::empty()
{
    static BufferString* ret = 0;
    if ( !ret )
    {
	ret = new BufferString( "0" );
	*ret->buf_ = '\0';
    }
    return *ret;
}


void BufferString::init()
{
    len_ = minlen_;
    mTryAlloc( buf_, char[len_] );
    if ( buf_ )
	*buf_ ='\0';
}


std::ostream& operator <<( std::ostream& s, const BufferString& bs )
{ s << (const char*)bs; return s; }

std::istream& operator >>( std::istream& s, BufferString& bs )
{ s >> bs.buf(); return s; }
