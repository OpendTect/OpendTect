/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: bufstring.cc,v 1.13 2008-11-21 14:58:20 cvsbert Exp $";

#include "bufstring.h"

#include "general.h"
#include <iostream>
#include <stdlib.h>
#include "string2.h"
#include <string.h>


BufferString::BufferString( int sz, bool mknull )
    : minlen_(sz)
    , len_(0)
    , buf_(0)
{
    if ( sz < 1 ) return;

    setBufSize( sz );
    if ( len_ > 0 )
	memset( buf_, 0, len_ );
}


BufferString::BufferString( const BufferString& bs )
    : minlen_(bs.minlen_)
    , len_(0)
    , buf_(0)
{
    if ( !bs.buf_ || !bs.len_ ) return;

    mTryAlloc( buf_, char [bs.len_] );
    if ( buf_ )
    {
	len_ = bs.len_;
	strcpy( buf_, bs.buf_ );
    }
}


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


BufferString& BufferString::add( const char* s )
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
    if ( len_ < 1 )
	buf_ = 0;
    else
    {
	mTryAlloc( buf_, char[len_] );
	if ( buf_ )
	    *buf_ ='\0';
    }
}


std::ostream& operator <<( std::ostream& s, const BufferString& bs )
{
    s << bs.buf();
    return s;
}

std::istream& operator >>( std::istream& s, BufferString& bs )
{
    std::string stdstr; s >> stdstr;
    bs = stdstr.c_str();
    return s;
}
