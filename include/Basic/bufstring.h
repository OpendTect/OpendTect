#ifndef bufstring_H
#define bufstring_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-4-2000
 Contents:	Variable buffer length strings with minimum size.
 RCS:		$Id: bufstring.h,v 1.7 2001-03-19 10:17:47 bert Exp $
________________________________________________________________________

-*/

#include <string.h>
#include <string2.h>
#include <iostream.h>
#include <stdlib.h>


/*!\brief String with variable length but guaranteed minimum buffer size.

The minimum buffer size makes life easier in worlds where strcpy etc. rule.
Overhead is 8 extra bytes for length and minimum length.

*/

class BufferString
{
public:
   			BufferString( const char* s=0,
				      unsigned int ml=mMaxUserIDLength )
				: minlen(ml+1)
			{ init(); if ( s ) *this = s; }
   			BufferString( int i,
				      unsigned int ml=mMaxUserIDLength )
				: minlen(ml+1)
			{ init(); *this += i; }
   			BufferString( double d,
				      unsigned int ml=mMaxUserIDLength )
				: minlen(ml+1)
			{ init(); *this += d; }
   			BufferString( float f,
				      unsigned int ml=mMaxUserIDLength )
				: minlen(ml+1)
			{ init(); *this += f; }
			BufferString( const BufferString& bs )
				: minlen(bs.minlen)
			{ init(); *this = bs; }
			~BufferString()
			{ free(buf); }
   inline BufferString&	operator=( const BufferString& bs )
			{ if ( &bs != this ) *this = bs.buf; return *this; }
   inline BufferString&	operator=( int i )
			{ *buf = '\0'; *this += i; return *this; }
   inline BufferString&	operator=( double d )
			{ *buf = '\0'; *this += d; return *this; }
   inline BufferString&	operator=( float f )
			{ *buf = '\0'; *this += f; return *this; }
   inline BufferString&	operator+=( int i )
			{ *this += getStringFromInt("%d",i); return *this; }
   inline BufferString&	operator+=( double d )
			{ *this += getStringFromDouble("%lg",d); return *this; }
   inline BufferString&	operator+=( float f )
			{ *this += getStringFromFloat("%g",f); return *this; }
   inline		operator char*() const		{ return (char*)buf; }
   inline char&		operator [](int idx)		{ return buf[idx]; }
   inline const char&	operator [](int idx) const	{ return buf[idx]; }
   inline unsigned int	size() const			{ return strlen(buf); }
   inline unsigned int	bufSize() const
			{ return len; }
   inline bool		operator==( const BufferString& s ) const
			{ return operator ==( s.buf ); }
   inline bool		operator!=( const BufferString& s ) const
			{ return operator !=( s.buf ); }
   inline bool		operator!=( const char* s ) const
			{ return ! (*this == s); }
   inline bool		operator >( const char* s ) const
			{ return s ? strcmp(buf,s) > 0 : true; }
   inline bool		operator <( const char* s ) const
			{ return s ? strcmp(buf,s) < 0 : false; }

   inline BufferString&	operator=(const char*);
   inline BufferString&	operator+=(const char*);
   inline bool		operator==(const char*) const;

protected:

    char*		buf;
    unsigned int	len;
    const unsigned int	minlen;

private:

    inline void		init()
			{ len = minlen; buf = mMALLOC(len,char); *buf = '\0'; }

};


inline bool BufferString::operator==( const char* s ) const
{
    if ( !s ) return *buf == '\0';

    const char* ptr = buf;
    while ( *s && *ptr )
	if ( *ptr++ != *s++ ) return false;

    return *ptr == *s;
}


inline BufferString& BufferString::operator=( const char* s )
{
    if ( buf != s )
    {
	if ( !s ) s = "";
	unsigned int newlen = (unsigned int)(strlen(s) + 1);
	if ( newlen < minlen ) newlen = minlen;
	if ( newlen != len )
	    { len = newlen; buf = mREALLOC(buf,len,char); }
	char* ptr = buf;
	while ( *s ) *ptr++ = *s++;
	*ptr = '\0';
    }
    return *this;
}


inline BufferString& BufferString::operator +=( const char* s )
{
    if ( s && *s )
    {
	unsigned int newlen = (unsigned int)(strlen(s) + strlen(buf)) + 1;
	if ( newlen < minlen ) newlen = minlen;
	if ( newlen != len )
	{
	    len = newlen;
	    buf = mREALLOC(buf,len,char);
	}

	char* ptr = buf;
	while ( *ptr ) ptr++;
	while ( *s ) *ptr++ = *s++;
	*ptr = '\0';
    }
    return *this;
}



inline ostream& operator <<( ostream& stream, const BufferString& bs )
{ stream << (const char*)bs; return stream; }

inline istream& operator >>( istream& stream, BufferString& bs )
{ stream >> (char*)bs; return stream; }


#endif
