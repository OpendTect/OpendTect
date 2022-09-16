#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "stringview.h"
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
    inline explicit	BufferString(const T&);
    template <class T>
    inline		BufferString(const char*,const T&,
				     const char* s=nullptr);
			BufferString(const QString&);
    virtual		~BufferString();
    inline BufferString& operator=( const BufferString& b )
						{ return assignTo(b.buf_); }
    inline BufferString& operator=( const char* s )
						{ return assignTo(s); }
    inline BufferString& operator=( const StringView& s )
						{ return assignTo(s.str()); }
    inline BufferString& operator=( const OD::String& s )
						{ return assignTo(s.str()); }
    BufferString&	operator=(const std::string&);
    template <class T>
    inline BufferString& operator=(const T&);

    inline bool		operator==(const BufferString&) const;
    inline bool		operator!=(const BufferString&) const;
    inline bool		operator==(const char*) const;
    inline bool		operator!=(const char*) const;
    inline bool		operator==(const OD::String&) const;
    inline bool		operator!=(const OD::String&) const;
    inline bool		operator==(const StringView&) const;
    inline bool		operator!=(const StringView&) const;

    inline char*	getCStr()
    { return const_cast<char*>(const_cast<BufferString*>(this)->gtBuf()); }

    inline char&	operator []( int idx )	{ return getCStr()[idx]; }
    inline char&	first()			{ return getCStr()[0]; }
    inline char&	last()			{ return getCStr()[size()-1]; }
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
    BufferString&	set(float,int nrdec);
    BufferString&	set(double,int nrdec);
    BufferString&	setLim(float,int maxnrchars);
    BufferString&	setLim(double,int maxnrchars);

    BufferString&	add(char);
    BufferString&	add(const char*);
    BufferString&	add( const OD::String& s )	{ return add(s.str()); }
    BufferString&	add(const QString&);
    template <class T>
    BufferString&	add(const T&);
    BufferString&	add(float,int nrdec);
    BufferString&	add(double,int nrdec);
    BufferString&	addLim(float,int maxnrchars);
    BufferString&	addLim(double,int maxnrchars);

    BufferString&	addSpace(int nrspaces=1);
    BufferString&	addTab(int nrtabs=1);
    BufferString&	addNewLine(int nrnewlines=1);

    inline unsigned int	bufSize() const		{ return len_; }
    bool		setBufSize(unsigned int);
    inline unsigned int	minBufSize() const	{ return minlen_; }
    void		setMinBufSize(unsigned int);

    BufferString&	replace(char from,char to);
    BufferString&	replace(const char* from,const char* to);
    BufferString&	remove(char);
    inline BufferString& remove( const char* s )  { return replace(s,nullptr); }
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

    const char*		gtBuf() const override;
    inline const char*	gtStr() const override;

private:

    void		init();
    inline void		destroy()	{ delete [] buf_; buf_ = nullptr; }
    BufferString&	addArr32Chars(const char*,int);

public:

    // use add instead
    inline BufferString& operator+=( const char* s )	{ return add( s ); }
    inline BufferString& operator+=( const OD::String& s )
							{ return add(s.str()); }
    template <class T>
    inline BufferString& operator+=( const T& t )	{ return add( t ); }
};


// Avoid silent conversion to BufferString from any type.
inline bool operator==(const char*,const BufferString&)		= delete;
inline bool operator!=(const char*,const BufferString&)		= delete;


#define mBufferStringSimpConstrInitList \
    buf_(nullptr), len_(0), minlen_(mMaxFilePathLength+1)

inline BufferString::BufferString()
    : mBufferStringSimpConstrInitList	{}
inline BufferString::BufferString( const char* s )
    : mBufferStringSimpConstrInitList	{ assignTo(s); }
inline BufferString::BufferString( const OD::String& s )
    : mBufferStringSimpConstrInitList	{ assignTo(s.str()); }

template <class T> inline
BufferString::BufferString( const T& t )
    : mBufferStringSimpConstrInitList
{ add( t ); }

template <class T> inline
BufferString::BufferString( const char* s1, const T& t, const char* s2 )
    : mBufferStringSimpConstrInitList
{ *this += s1; *this += t; *this += s2; }

inline const char* BufferString::gtStr() const
{ return buf_ && *buf_ ? buf_ : nullptr; }

inline bool BufferString::operator==( const BufferString& s ) const
{ return isEqual( s.buf_ ); }
inline bool BufferString::operator!=( const BufferString& s ) const
{ return !isEqual( s.buf_ ); }
inline bool BufferString::operator==( const char* s ) const
{ return isEqual( s ); }
inline bool BufferString::operator!=( const char* s ) const
{ return !isEqual( s ); }
inline bool BufferString::operator==( const StringView& s ) const
{ return isEqual( s.str() ); }
inline bool BufferString::operator!=( const StringView& s ) const
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

inline BufferString& BufferString::set( float f, int nrdec )
{ setEmpty(); return add( f, nrdec ); }

inline BufferString& BufferString::set( double d, int nrdec )
{ setEmpty(); return add( d, nrdec ); }

inline BufferString& BufferString::setLim( float f, int maxnrchars )
{ setEmpty(); return addLim( f, maxnrchars ); }

inline BufferString& BufferString::setLim( double d, int maxnrchars )
{ setEmpty(); return addLim( d, maxnrchars ); }


/*!
\brief A StringPair has two strings, first() and second().

The getCompString() function concatanates the two strings with
the pipe character ('|') in between.
*/

mExpClass(Basic) StringPair
{
public:

			StringPair(const char* str1, const char* str2)
			    : first_(str1), second_(str2) {}
			StringPair(const StringPair&);
			StringPair(const char* compstr);

    static char		separator() { return '|'; }

    const BufferString&	first() const	{ return first_; }
    BufferString&	first()		{ return first_; }
    const BufferString&	second() const	{ return second_; }
    BufferString&	second()	{ return second_; }
    bool		hasSecond() const	{ return !second_.isEmpty(); }

    const OD::String&	getCompString() const;
    const OD::String&	getCompString(bool withwhitespace) const;

protected:

    BufferString	first_;
    BufferString	second_;

};


//For compilation purposes, should not be used
namespace Values
{

template<>
mClass(Basic) Undef<BufferString>
{
public:

    static BufferString val() { return BufferString(); }
    static bool		hasUdf() { return true; }
    static bool		isUdf(const BufferString& s) { return s.isEmpty(); }
    static void		setUdf(BufferString& s) { s = BufferString(); }

};

}
