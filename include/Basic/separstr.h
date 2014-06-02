#ifndef separstr_h
#define separstr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		May 1995
 Contents:	String with a separator between the items
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "fixedstring.h"
#include "convert.h"

class BufferStringSet;

/*!
\brief %List encoded in a string.

  SeparString is a list encoded in a string where the items are separated by
  a user chosen separator. The separator in the input is escaped with a
  backslash. A `\' is encoded as `\\' . Elements can have any size.  Input and
  output of elements is done unescaped. Input and output of whole (sub)strings
  is done escaped.
*/

mExpClass(Basic) SeparString
{
public:

			SeparString( const char* escapedstr=0, char separ=',' )
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
    const BufferString&	rep() const		{ return rep_; }
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
				     const char* nextsep=0) const;

    const char*		findSeparator(const char*) const;
};


/*!
\brief SeparString with backquotes as separators, use in most ascii files.
*/

mExpClass(Basic) FileMultiString : public SeparString
{
public:

			FileMultiString(const char* escapedstr=0)
			    : SeparString(escapedstr, separator() )	{}
    template <class T>	FileMultiString( const T& t )
			    : SeparString(t,separator())		{}

    static char		separator() { return '`'; }

    // The function template overloading add(const SeparString&) in the base
    // class needs an exact match! Passing a derived object would make the
    // template function convert it to (const char*).
    inline FileMultiString& add( const FileMultiString& fms )
			{ return add( (SeparString&)fms ); }
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

#endif
