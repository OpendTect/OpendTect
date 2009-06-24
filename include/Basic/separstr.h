#ifndef separstr_h
#define separstr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		May 1995
 Contents:	String with a separator between the items
 RCS:		$Id: separstr.h,v 1.21 2009-06-24 10:52:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "convert.h"
class BufferStringSet;


/*!\brief list encoded in a string.

SeparString is a list encoded in a string where the items are separated by
a user chosen separator. The separator in the input is escaped with a backslash.
A `\' is encoded as `\\' . Elements can have any size.

*/

mClass SeparString
{
public:
			SeparString( const char* s=0, char separ=',' )
			{ initSep( separ ); addStr( s ); }
			SeparString( const SeparString& s )
			: rep_(s.rep_) { initSep( s.sep_[0] ); }

    SeparString&	operator=(const SeparString&);
    SeparString&	operator=(const char*);
    template <class T>
    inline SeparString&	operator=( T t )
    			{ return operator=( Conv::to<const char*>(t)); }

    inline bool		isEmpty() const		{ return rep_.isEmpty(); }
    inline void		setEmpty()		{ rep_.setEmpty(); }

    int			size() const;
    const char*		operator[](int) const;
    const char*		from(int) const;
    int			getIValue(int) const;
    od_uint32		getUIValue(int) const;
    od_int64		getI64Value(int) const;
    od_uint64		getUI64Value(int) const;
    float		getFValue(int) const;
    double		getDValue(int) const;
    bool		getYN(int) const;
    int			indexOf(const char*) const;

    SeparString&	add(const char*);
    inline SeparString&	operator +=( const char* s )	{ return add( s ); }
    template <class T>
    SeparString&	operator +=( T t )
    				{ return add( Conv::to<const char*>(t) ); }
    SeparString&	operator +=(const BufferStringSet&);

    inline		operator const char*() const	{ return buf(); }
    inline char*	buf()			{ return rep_.buf(); }
    inline const char*	buf() const		{ return rep_.buf(); }
    inline char		sepChar() const		{ return *sep_; }
    void		setSepChar(char);

    BufferString&	rep()			{ return rep_; }
    const BufferString&	rep() const		{ return rep_; }

private:

    char		sep_[2];
    BufferString	rep_;

    void		addStr(const char*);
    inline void		initSep( char s )	{ sep_[0] = s; sep_[1] = '\0'; }

};

mGlobal std::ostream& operator <<(std::ostream&,const SeparString&);
mGlobal std::istream& operator >>(std::istream&,SeparString&);



/*!\brief SeparString with backquotes as separators, use in most ascii files */

mClass FileMultiString : public SeparString
{
public:

			FileMultiString( const char* str=0 )
			    : SeparString(str,'`')		{}
    template <class T>	FileMultiString( T t )
			    : SeparString(t,'`')		{}

    FileMultiString&	operator=( const FileMultiString& fms )
			{ SeparString::operator=( fms ); return *this; }
    template <class T>
    FileMultiString&	operator=( T t )
    			{ SeparString::operator=( t ); return *this; }

};


#endif
