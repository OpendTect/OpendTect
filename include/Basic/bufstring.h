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
#include "fixedstring.h"
#include "string2.h"
class QString;

/*!
\brief OD::String with its own variable length buffer. The buffer has a
guaranteed minimum size.

The minimum buffer size makes life easier in worlds where strcpy etc. rule.
Overhead is 4 extra bytes for variable length and 4 bytes for minimum length.

Constructing/asigning/comparing etc. with a (char*) null pointer is no problem.

Don't try to add constructors with a single basic type - this leads to nasty
hidden bugs because the compiler will try to convert all kinds of things into
BufferStrings. If you just need a string from an int, float, ..., just
use str = toString( var ). If you need the BufferString later, use:
\code
BufferString istr( "", intvar );
\endcode

*/

mExpClass(Basic) BufferString : public OD::String
{
public:

    inline		BufferString();
    inline		BufferString(const char*);
			BufferString(const OD::String&);
			BufferString(int minlen,bool setnull);
			BufferString(const BufferString&);
    template <class T>
    inline		BufferString(const char*,const T&,const char* s=0);
			BufferString(const QString&);
    virtual		~BufferString();
    inline BufferString& operator=( const BufferString& b )
						{ return assignTo(b.buf_); }
    inline BufferString& operator=( const char* s )
						{ return assignTo(s); }
    inline BufferString& operator=( const FixedString& s )
						{ return assignTo(s.str()); }
    inline BufferString& operator=( const OD::String& s )
						{ return assignTo(s.str()); }
    template <class T>
    inline BufferString& operator=(const T&);

    inline		operator const char*() const	{ return buf(); }

    inline bool		operator==(const BufferString&) const;
    inline bool		operator!=(const BufferString&) const;
    inline bool		operator==(const char*) const;
    inline bool		operator!=(const char*) const;
    inline bool		operator==(const OD::String&) const;
    inline bool		operator!=(const OD::String&) const;
    inline bool		operator==(const FixedString&) const;
    inline bool		operator!=(const FixedString&) const;

    inline char*	getCStr()
    { return const_cast<char*>(const_cast<BufferString*>(this)->gtBuf()); }
    inline char&	operator []( int idx )	{ return getCStr()[idx]; }
    char*		find(char);
    char*		find(const char*);
    inline const char*	find(char) const;
    inline const char*	find(const char*) const;
    char*		findLast(char);
    char*		findLast(const char*);
    inline const char*	findLast(char) const;
    inline const char*	findLast(const char*) const;

    BufferString&	setEmpty();
    BufferString&	set(const char*);
    BufferString&	set( const OD::String& s )	{ return set(s.str()); }
    template <class T>
    BufferString&	set(const T&);

    BufferString&	add(const char*);
    BufferString&	add( const OD::String& s )	{ return add(s.str()); }
    BufferString&	add(const QString&);
    template <class T>
    BufferString&	add(const T&);

    void                fill(char*,int maxnrchar) const;
    BufferString&	addSpace();
    BufferString&	addTab();
    BufferString&	addNewLine();

    inline unsigned int	bufSize() const		{ return len_; }
    void		setBufSize(unsigned int);
    inline unsigned int	minBufSize() const	{ return minlen_; }
    void		setMinBufSize(unsigned int);

    BufferString&	replace(char from,char to);
    BufferString&	replace(const char* from,const char* to);
    BufferString&	remove(char);
    inline BufferString& remove( const char* s )	{ return replace(s,0); }
    BufferString&	trimBlanks(); //!< removes front and back whitespaces

    BufferString&	insertAt(int idx, const char*);
				//< If idx >= size(), pads spaces
    BufferString&	replaceAt(int idx, const char*,bool cutoff=true);
				//< If idx >= size(), pads spaces
    BufferString&	toLower();
    BufferString&	toUpper();
    BufferString&	embed(char open,char close);
    BufferString&	unEmbed(char open,char close);
    inline BufferString& quote( char q='\'' )		{ return embed(q,q); }
    inline BufferString& unQuote( char q='\'' )		{ return unEmbed(q,q); }

    enum CleanType	{ OnlyAlphaNum, AllowDots,
			  NoSpaces, NoFileSeps, NoSpecialChars };
    BufferString&	clean(CleanType ct=OnlyAlphaNum);

    static const BufferString& empty();

protected:

    char*		buf_;
    unsigned int	len_;
    const unsigned int	minlen_;

    BufferString&	assignTo(const char*);

    const char*		gtBuf() const;
    inline const char*	gtStr() const;

private:

    void		init();
    inline void		destroy()	{ delete [] buf_; buf_ = 0; }

public:

    // use add instead
    inline BufferString& operator+=( const char* s )	{ return add( s ); }
    inline BufferString& operator+=( const OD::String& s )
							{ return add(s.str()); }
    template <class T>
    inline BufferString& operator+=( const T& t )	{ return add( t ); }

};


#ifndef __win__

// Avoid silent conversion to BufferString from any type.

void OD_Undef_const_char_eq_bs_finder();
void OD_Undef_const_char_neq_bs_finder();
inline bool operator==(const char*,const BufferString&)
{ OD_Undef_const_char_eq_bs_finder(); return false; }
inline bool operator!=(const char*,const BufferString&)
{ OD_Undef_const_char_neq_bs_finder(); return true; }

#endif


#define mBufferStringSimpConstrInitList \
    minlen_(mMaxFilePathLength+1), buf_(0), len_(0)

inline BufferString::BufferString()
    : mBufferStringSimpConstrInitList	{}
inline BufferString::BufferString( const char* s )
    : mBufferStringSimpConstrInitList	{ assignTo(s); }
inline BufferString::BufferString( const OD::String& s )
    : mBufferStringSimpConstrInitList	{ assignTo(s.str()); }

template <class T> inline
BufferString::BufferString( const char* s1, const T& t, const char* s2 )
    : mBufferStringSimpConstrInitList
{ *this += s1; *this += t; *this += s2; }

inline const char* BufferString::gtStr() const
{ return buf_ && *buf_ ? const_cast<char*>( buf_ ) : 0; }

inline bool BufferString::operator==( const BufferString& s ) const
{ return isEqual( s.buf_ ); }
inline bool BufferString::operator!=( const BufferString& s ) const
{ return !isEqual( s.buf_ ); }
inline bool BufferString::operator==( const char* s ) const
{ return isEqual( s ); }
inline bool BufferString::operator!=( const char* s ) const
{ return !isEqual( s ); }
inline bool BufferString::operator==( const FixedString& s ) const
{ return isEqual( s.str() ); }
inline bool BufferString::operator!=( const FixedString& s ) const
{ return !isEqual( s.str() ); }
inline bool BufferString::operator==( const OD::String& s ) const
{ return isEqual( s.str() ); }
inline bool BufferString::operator!=( const OD::String& s ) const
{ return !isEqual( s.str() ); }

inline const char* BufferString::find( char c ) const
{ return OD::String::find( c ); }
inline const char* BufferString::find( const char* s ) const
{ return OD::String::find( s ); }
inline const char* BufferString::findLast( char c ) const
{ return OD::String::findLast( c ); }
inline const char* BufferString::findLast( const char* s ) const
{ return OD::String::findLast( s ); }

template <class T> inline BufferString& BufferString::operator=( const T& t )
{ return set( t ); }

template <class T> inline BufferString& BufferString::add( const T& t )
{ return add( toString(t) ); }

inline BufferString& BufferString::set( const char* s )
{ setEmpty(); return add( s ); }

template <class T> inline BufferString& BufferString::set( const T& t )
{ setEmpty(); return add( t ); }


#endif
