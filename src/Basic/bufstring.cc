/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: bufstring.cc,v 1.1 2006-06-01 12:23:39 cvskris Exp $";

#include "bufstring.h"

#include <iostream>
#include "string2.h"
#include <string.h>
#include <stdlib.h>



BufferString::BufferString( const char* s, unsigned int ml )
    : minlen_(ml+1)
{ init(); if ( s ) *this = s; }


BufferString::BufferString( int i, unsigned int ml )
    : minlen_(ml+1)
{ init(); *this += i; }


BufferString::BufferString( double d, unsigned int ml )
    : minlen_(ml+1)
{ init(); *this += d; }


BufferString::BufferString( float f, unsigned int ml )
    : minlen_(ml+1)
{ init(); *this += f; }


BufferString::BufferString( const BufferString& bs )
    : minlen_(bs.minlen_)
{ init(); *this = bs; }


BufferString::~BufferString()
{ delete [] buf_; }


BufferString&	BufferString::operator=( const BufferString& bs )
{ if ( &bs != this ) *this = bs.buf_; return *this; }


BufferString&	BufferString::operator=( int i )
{ *buf_ = '\0'; *this += i; return *this; }


BufferString&	BufferString::operator=( double d )
{ *buf_ = '\0'; *this += d; return *this; }


BufferString&	BufferString::operator=( float f )
{ *buf_ = '\0'; *this += f; return *this; }


BufferString&	BufferString::operator+=( int i )
{ *this += getStringFromInt("%d",i); return *this; }


BufferString&	BufferString::operator+=( double d )
{ *this += getStringFromDouble("%lg",d); return *this; }


BufferString&	BufferString::operator+=( float f )
{ *this += getStringFromFloat("%g",f); return *this; }


void BufferString::insertAt( int idx, const char* str )
{
    const int nrtopad = idx-size();
    for ( int idy=0; idy<nrtopad; idy++ )
	(*this) += " ";

    buf_[idx] = 0;
    (*this) += str;
}


BufferString::operator const char*() const
{ return buf_; }


char* BufferString::buf()
{ return buf_; }


const char* BufferString::buf() const
{ return buf_; }


char& BufferString::operator [](int idx)
{ return buf_[idx]; }


const char& BufferString::operator [](int idx) const
{ return buf_[idx]; }


unsigned int BufferString::size() const
{ return strlen(buf_); }


unsigned int BufferString::bufSize() const
{ return len_; }


bool BufferString::operator==( const BufferString& s ) const
{ return operator ==( s.buf_ ); }


bool BufferString::operator!=( const BufferString& s ) const
{ return operator !=( s.buf_ ); }


bool BufferString::operator!=( const char* s ) const
{ return ! (*this == s); }


bool BufferString::operator >( const char* s ) const
{ return s ? strcmp(buf_,s) > 0 : true; }


bool BufferString::operator <( const char* s ) const
{ return s ? strcmp(buf_,s) < 0 : false; }


void BufferString::init()
{ len_ = minlen_; buf_ = new char [len_]; *buf_ ='\0'; }


bool BufferString::operator==( const char* s ) const
{
    if ( !s ) return *buf_ == '\0';

    const char* ptr = buf_;
    while ( *s && *ptr )
	if ( *ptr++ != *s++ ) return false;

    return *ptr == *s;
}


void BufferString::setBufSize( unsigned int newlen )
{
    if ( newlen < minlen_ ) newlen = minlen_;
    if ( newlen == len_ ) return;

    char* oldbuf = buf_;
    buf_ = new char [newlen];
    memcpy( buf_, oldbuf, mMIN(newlen,strlen(oldbuf)+1) );
    delete [] oldbuf;

    len_ = newlen;
}


BufferString& BufferString::operator=( const char* s )
{
    if ( buf_ != s )
    {
	if ( !s ) s = "";
	setBufSize( (unsigned int)(strlen(s) + 1) );
	char* ptr = buf_;
	while ( *s ) *ptr++ = *s++;
	    *ptr = '\0';
    }
    return *this;
}


BufferString& BufferString::operator +=( const char* s )
{
    if ( s && *s )
    {
	setBufSize( (unsigned int)(strlen(s) + strlen(buf_)) + 1 );

	char* ptr = buf_;
	while ( *ptr ) ptr++;
	    while ( *s ) *ptr++ = *s++;
		*ptr = '\0';
    }
    return *this;
}


const BufferString& BufferString::empty()
{
    static BufferString ret;
    return ret;
}

std::ostream& operator <<( std::ostream& s, const BufferString& bs )
{ s << (const char*)bs; return s; }

std::istream& operator >>( std::istream& s, BufferString& bs )
{ s >> bs.buf(); return s; }
