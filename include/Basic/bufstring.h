#ifndef bufstring_h
#define bufstring_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		12-4-2000
 Contents:	Variable buffer length strings with minimum size.
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "convert.h"
#include <iosfwd>

class FixedString;

/*!
\brief String with variable length but guaranteed minimum buffer size.

The minimum buffer size makes life easier in worlds where strcpy etc. rule.
Overhead is 4 extra bytes for variable length and 4 bytes for minimum length.

Passing a (char*) null pointer is no problem.

Don't try to add constructors with a single basic type - this leads to nasty
hidden bugs because the compiler will try to convert all kinds of things into
BufferStrings. If you just need a string from an int, float, ..., just
use str = toString( var ). If you need the BufferString later, use:
\code
BufferString istr( "", intvar );
\endcode

*/

mExpClass(Basic) BufferString
{
public:

    inline		BufferString();
    inline		BufferString(const char*);
			BufferString(const FixedString&);
			BufferString(int minlen,bool setnull);
			BufferString(const BufferString& bs);
    template <class T>
    inline		BufferString(const char*,const T&,const char* s=0);
    virtual		~BufferString();
    inline BufferString& operator=( const BufferString& b )
						{ return assignTo(b.buf_); }

    inline BufferString& operator=( const char* s ) { return assignTo(s); }
    inline BufferString& operator=( char* s ) { return assignTo(s); }
    template <class T>
    inline BufferString& operator=(const T&);

    inline bool		operator==(const BufferString&) const;
    inline bool		operator!=(const BufferString&) const;
    bool		operator==(const char*) const;
    inline bool		operator!=(const char*) const;
    template <class T>
    inline bool		operator==(const T&) const;
    template <class T>
    inline bool		operator!=( const T& t ) const	{ return !(*this==t); }

    char*		buf();		//!< Allocation of min length guaranteed
    inline const char*	buf() const	{ return buf_ ? buf_ : empty().buf_; }
    inline const char*	str() const;	//!<\returns null pointer if empty
    char*		bufEnd()	{ return buf()+size(); }
    			/*!<Use with care, allocation beyond min length is not
			    guaranteed. */
    inline		operator const char*() const	{ return buf(); }
    inline char&	operator []( int idx )		{ return buf()[idx]; }
    inline const char&	operator []( int idx ) const	{ return buf()[idx]; }
    bool		isEmpty() const;
    void		setEmpty();
    bool		isEqual(const char*,bool caseinsens=false) const;
    bool		isStartOf(const char*,bool caseinsens=false) const;
    bool		matches(const char*,bool caseinsens=false) const;
    void                fill(char*) const; //<!Caller should ensure enough space

    BufferString&	add(const char*);
    BufferString&	add( const BufferString& b )	{ return add(b.buf()); }
    inline BufferString& operator+=( const char* s )	{ return add( s ); }
    template <class T>
    BufferString&	add(const T&);
    template <class T>
    inline BufferString& operator+=( const T& t )	{ return add( t ); }

    unsigned int	size() const;
    inline unsigned int	bufSize() const		{ return len_; }
    void		setBufSize(unsigned int);
    inline unsigned int	minBufSize() const	{ return minlen_; }
    void		setMinBufSize(unsigned int);

    void		insertAt(int idx, const char*);
			//< If idx >= size(), pads spaces
    void		replaceAt(int idx, const char*,bool cutoff=true);
			//< If idx >= size(), pads spaces

    bool		operator >(const char*) const;
    bool		operator <(const char*) const;
    template <class T>
    inline bool		operator >(const T&) const;
    template <class T>
    inline bool		operator <(const T&) const;

    static const BufferString& empty();

protected:

    char*		buf_;
    unsigned int	len_;
    const unsigned int	minlen_;

    BufferString&	assignTo(const char*);

private:

    void		init();
    inline void		destroy()	{ delete [] buf_; buf_ = 0; }

};

mGlobal(Basic) std::ostream& operator <<(std::ostream&,const BufferString&);
mGlobal(Basic) std::istream& operator >>(std::istream&,BufferString&);


#define mBufferStringSimpConstrInitList \
    minlen_(mMaxFilePathLength+1), buf_(0), len_(0)

inline BufferString::BufferString()
    : mBufferStringSimpConstrInitList	{}
inline BufferString::BufferString( const char* s )
    : mBufferStringSimpConstrInitList	{ assignTo(s); }

template <class T> inline
BufferString::BufferString( const char* s1, const T& t, const char* s2 )
    : mBufferStringSimpConstrInitList
{ *this += s1; *this += t; *this += s2; }


inline
const char* BufferString::str() const	
{ return isEmpty() ? 0 : buf_; }	


inline bool BufferString::operator==( const BufferString& s ) const
{ return operator ==( s.buf() ); }

template <class T> inline bool BufferString::operator==( const T& t ) const
{ return *this == Conv::to<const char*>( t ); }

inline bool BufferString::operator!=( const BufferString& s ) const
{ return operator !=( s.buf() ); }

inline  bool BufferString::operator!=( const char* s ) const
{ return ! (*this == s); }

template <class T> inline BufferString& BufferString::operator=( const T& t )
{ char* s = buf(); strcpy(s,toString( t )); return *this; }

template <class T> inline BufferString& BufferString::add( const T& t )
{
    return add( toString( t ) );
}

template <class T> inline bool BufferString::operator >( const T& t ) const
{ return *this > ( Conv::to<const char*>( t ) ); }

template <class T> inline bool BufferString::operator <( const T& t ) const
{ return *this < ( Conv::to<const char*>( t ) ); }


#endif

