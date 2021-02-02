#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		12-4-2000
 Contents:	Variable buffer length strings with minimum size.
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
			BufferString(size_type minlen,bool setnull);
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

    inline char*	getCStr()	{ return mNonConst(mSelf().gtBuf()); }

    inline char&	operator []( idx_type idx )
						{ return getCStr()[idx]; }
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
    BufferString&	addLim(float,size_type maxnrchars);
    BufferString&	addLim(double,size_type maxnrchars);

    BufferString&	addSpace(size_type nrspaces=1);
    BufferString&	addTab(size_type nrtabs=1);
    BufferString&	addNewLine(size_type nrnewlines=1);

    inline size_type	bufSize() const		{ return len_; }
    bool		setBufSize(size_type);
    inline size_type	minBufSize() const	{ return minlen_; }
    void		setMinBufSize(size_type);

    BufferString&	replace(char from,char to);
    BufferString&	replace(const char* from,const char* to);
    BufferString&	remove(char);
    inline BufferString& remove( const char* s )  { return replace(s,nullptr); }
    BufferString&	trimBlanks(); //!< removes front and back whitespaces

    BufferString&	insertAt(idx_type idx, const char*);
				//< If idx >= size(), pads spaces
    BufferString&	replaceAt(idx_type idx, const char*,bool cutoff=true);
				//< If idx >= size(), pads spaces
    BufferString&	toLower();
    BufferString&	toUpper(bool onlyfirstchar=false);
    BufferString&	embed(char open,char close);
    BufferString&	unEmbed(char open,char close);
    inline BufferString& quote( char q='\'' )		{ return embed(q,q); }
    inline BufferString& unQuote( char q='\'' )		{ return unEmbed(q,q); }

    enum CleanType	{ OnlyAlphaNum, AllowDots,
			  NoSpaces, NoFileSeps, NoSpecialChars };
    BufferString&	clean(CleanType ct=OnlyAlphaNum);

    static const BufferString& empty();

protected:

    char*		buf_	= nullptr;;
    size_type		len_	= 0;
    const size_type	minlen_;

    BufferString&	assignTo(const char*);

    const char*		gtBuf() const;
    inline const char*	gtStr() const;

private:

    void		init();
    inline void		destroy()	{ delete [] buf_; buf_ = nullptr; }
    BufferString&	addArr32Chars(const char*,size_type);

public:

    // use add instead
    inline BufferString& operator+=( const char* s )	{ return add( s ); }
    inline BufferString& operator+=( const OD::String& s )
							{ return add(s.str()); }
    template <class T>
    inline BufferString& operator+=( const T& t )	{ return add( t ); }

};


mGlobal(Basic) inline BufferString toString( const QString& qs )
{
    return BufferString(qs);
}


//Not implemented on purpose as it leads unwanted conversions to
//BufferStrings on the fly
inline bool operator==(const char*,const BufferString&) = delete;
inline bool operator!=(const char*,const BufferString&) = delete;

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
{ add(s1); add(t); if ( s2 ) add(s2); }

inline const char* BufferString::gtStr() const
{ return buf_ && *buf_ ? const_cast<char*>( buf_ ) : nullptr; }

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
The getCompString() function concatanates the two strings with the pipe
character ('|') in between.
*/

mExpClass(Basic) StringPair
{
public:

			StringPair(const char* str1, const char* str2)
			    : first_(str1), second_(str2) {}
			StringPair(const char* compstr);

    static char		separator() { return '|'; }

    const BufferString&	first() const	{ return first_; }
    BufferString&	first()		{ return first_; }
    const BufferString&	second() const	{ return second_; }
    BufferString&	second()	{ return second_; }

    const OD::String&	getCompString() const;

protected:

    BufferString	first_;
    BufferString	second_;

};
