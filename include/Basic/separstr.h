#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 1995
 Contents:	String with a separator between the items
________________________________________________________________________

-*/

#include "basicmod.h"
#include "stringbuilder.h"
#include "fixedstring.h"
#include "convert.h"

class BufferStringSet;
class DBKey;

/*!
\brief %List encoded in a string.

  SeparString is a list encoded in a string where the items are separated by
  a user chosen separator. The separator in the input is escaped with a
  backslash. A `\' is encoded as `\\' . Elements can have any size.  Input and
  output of elements is done unescaped. Input and output of whole (sub)strings
  is done escaped.

  The new class SeparStringSB is based on the new StringBuilder, and will
  perform much better that the legacy SeparString class.
*/

mExpClass(Basic) SeparStringSB
{
public:

			SeparStringSB( const char* escapedstr=nullptr,
				     char separ=',' )
			{ initSep( separ ); initRep( escapedstr ); }
			SeparStringSB( const SeparStringSB& ss )
			: rep_(ss.rep_) { initSep( ss.sep_[0] ); }

    SeparStringSB&	operator=(const SeparStringSB&);
    SeparStringSB&	operator=(const char* escapedstr);

    inline bool		isEmpty() const		{ return rep_.isEmpty(); }
    inline void		setEmpty()		{ rep_.setEmpty(); }

    int			size() const;
    FixedString		operator[](int) const;		//!< Output unescaped
    FixedString		from(int) const;		//!< Output escaped

    od_int16		getI16Value(int) const;
    od_uint16		getUI16Value(int) const;
    od_int32		getIValue(int) const;
    od_uint32		getUIValue(int) const;
    od_int64		getI64Value(int) const;
    od_uint64		getUI64Value(int) const;
    float		getFValue(int) const;
    double		getDValue(int) const;
    bool		getYN(int) const;

    int			indexOf(const char* unescapedstr) const;

    SeparStringSB&	add(const BufferStringSet&);	//!< Concatenation
    SeparStringSB&	add(const SeparStringSB&);	//!< Concatenation
    SeparStringSB&	add(const DBKey&,bool withsurvloc=true);
    SeparStringSB&	add(const char* unescapedstr);
    inline SeparStringSB&	add( const OD::String& ods )
			{ return add( ods.buf() ); }
    template <class T>
    inline SeparStringSB&	add( T t )
			{ return add( toString(t) ); }

    SeparStringSB&	set( const BufferStringSet& bss )
			{ setEmpty(); return add( bss ); }
    SeparStringSB&	set( const SeparStringSB& s )
			{ setEmpty(); return add( s ); }
    SeparStringSB&	set( const char* unescapedstr )
			{ setEmpty(); return add( unescapedstr ); }
    inline SeparStringSB&	set( const OD::String& ods )
			{ return set( ods.buf() ); }
    template <class T>
    inline SeparStringSB&	set( T t )
			{ setEmpty(); return add( t ); }

    template <class T>
    inline SeparStringSB&	operator +=( T t )	{ return add( t ); }
    template <class T>
    inline SeparStringSB&	operator +=( const OD::String& ods )
						{ return add( ods.buf() ); }

    inline		operator const char*() const
						{ return buf(); }

    inline char*	getCStr()		{ return rep_.getCStr(256); }
							//!< Output escaped
    inline const char*	str() const		{ return rep_.result(); }
							//!< Output escaped
    inline const char*	buf() const
    { const char* ret = str(); if ( !ret ) ret = ""; return ret; }
							//!< Output escaped
    StringBuilder&	rep()			{ return rep_; }
							//!< Output escaped

    inline const char*	unescapedStr() const	{ return getUnescaped(buf()); }
			/*!< Use with care! Distinction between separ-chars
			     and escaped separ-chars will get lost. */

    inline char		sepChar() const		{ return *sep_; }
    inline const char*	sepStr() const		{ return sep_; }
    void		setSepChar(char);

private:

    char		sep_[2];
    StringBuilder	rep_;

    void		initRep(const char*);
    inline void		initSep( char s )	{ sep_[0] = s; sep_[1] = '\0'; }

    mutable BufferString retstr_;

    const char*		getEscaped(const char* unescapedstr,char sep) const;
    const char*		getUnescaped(const char* escapedstartptr,
				     const char* nextsep=nullptr) const;

    const char*		findSeparator(const char*) const;
};



mExpClass(Basic) SeparString
{
public:

			SeparString( const char* escapedstr=nullptr,
				     char separ=',' )
			{ initSep( separ ); initRep( escapedstr ); }
			SeparString( const SeparString& ss )
			: rep_(ss.rep_) { initSep( ss.sep_[0] ); }

    SeparString&	operator=(const SeparString&);
    SeparString&	operator=(const char* escapedstr);

    inline bool		isEmpty() const		{ return rep_.isEmpty(); }
    inline void		setEmpty()		{ rep_.setEmpty(); }

    int			size() const;
    FixedString		operator[](int) const;		//!< Output unescaped
    FixedString		from(int) const;		//!< Output escaped

    int			getIValue(int) const;
    od_uint32		getUIValue(int) const;
    od_int64		getI64Value(int) const;
    od_uint64		getUI64Value(int) const;
    float		getFValue(int) const;
    double		getDValue(int) const;
    bool		getYN(int) const;

    int			indexOf(const char* unescapedstr) const;

    SeparString&	add(const BufferStringSet&);	//!< Concatenation
    SeparString&	add(const SeparString&);	//!< Concatenation
    SeparString&	add(const char* unescapedstr);
    inline SeparString&	add( const OD::String& ods )
			{ return add( ods.buf() ); }
    template <class T>
    inline SeparString&	add( T t )
			{ return add( toString(t) ); }

    template <class T>
    inline SeparString&	operator +=( T t )	{ return add( t ); }
    template <class T>
    inline SeparString&	operator +=( const OD::String& ods )
						{ return add( ods.buf() ); }

    inline		operator const char*() const
						{ return buf(); }

    inline char*	getCStr()		{ return rep_.getCStr(); }
							//!< Output escaped
    inline const char*	buf() const		{ return rep_.buf(); }
							//!< Output escaped
    BufferString&	rep()			{ return rep_; }
							//!< Output escaped
    const OD::String&	rep() const		{ return rep_; }
							//!< Output escaped

    inline const char*	unescapedStr() const	{ return getUnescaped(buf()); }
			/*!< Use with care! Distinction between separ-chars
			     and escaped separ-chars will get lost. */

    inline char		sepChar() const		{ return *sep_; }
    inline const char*	sepStr() const		{ return sep_; }
    void		setSepChar(char);

private:

    char		sep_[2];
    BufferString	rep_;

    void		initRep(const char*);
    inline void		initSep( char s )	{ sep_[0] = s; sep_[1] = '\0'; }

    mutable BufferString retstr_;

    const char*		getEscaped(const char* unescapedstr,char sep) const;
    const char*		getUnescaped(const char* escapedstartptr,
				     const char* nextsep=nullptr) const;

    const char*		findSeparator(const char*) const;
};


/*!
\brief SeparString with backquotes as separators, use in most ascii files.
*/

mExpClass(Basic) FileMultiString : public SeparString
{
public:

			FileMultiString(const char* escapedstr=nullptr)
			    : SeparString(escapedstr, separator() )	{}
    template <class T>	FileMultiString( const T& t )
			    : SeparString(t,separator())		{}

    static char		separator() { return '`'; }

    // The function template overloading add(const SeparString&) in the base
    // class needs an exact match! Passing a derived object would make the
    // template function convert it to (const char*).
    inline FileMultiString& add( const FileMultiString& fms )
			{ return add( sCast(const SeparString&,fms) ); }
    template <class T> inline
    FileMultiString&	operator +=( T t )		{ return add( t ); }
    inline FileMultiString& operator +=( const OD::String& ods )
						{ return add( ods.buf() ); }

    template <class T> inline
    FileMultiString&	add( T t )
			{ SeparString::add( t ); return *this; }
    inline FileMultiString& add( const OD::String& ods )
			{ return add( ods.buf() ); }

};

