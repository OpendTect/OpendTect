#ifndef separstr_h
#define separstr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		May 1995
 Contents:	String with a separator between the items
 RCS:		$Id: separstr.h,v 1.14 2008-11-19 20:24:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "plftypes.h"


/*!\brief list encoded in a string.

SeparString is a list encoded in a string where the items are separated by
a user chosen separator. The separator in the input is escaped with a backslash.
A `\' is encoded as `\\' .
A FileMultiString has the back-quote as separator.
Elements can have any size, but if you use the [] operator they will be cut
at mMaxSepItem size.

*/

class SeparString
{
public:
			SeparString(const char* str=0,char separ=',');
			SeparString( const SeparString& s )
			: rep_(s.rep_) { sep_[0] = s.sep_[0]; sep_[1] = '\0'; }
    SeparString&	operator=(const SeparString&);
    SeparString&	operator=(const char*);
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

    void		add(const char*);
    inline SeparString&	operator +=( const char* s ) { add(s); return *this; }
    SeparString&	operator +=(int);
    SeparString&	operator +=(od_uint32);
    SeparString&	operator +=(od_int64);
    SeparString&	operator +=(od_uint64);
    SeparString&	operator +=(float);
    SeparString&	operator +=(double);
    SeparString&	operator +=(bool);

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

};


/*!\brief SeparString with backquotes as separators */

class FileMultiString : public SeparString
{
public:

			FileMultiString( const char* str=0 )
			    : SeparString(str,'`')		{}

    FileMultiString&	operator=( const FileMultiString& fms )
			{ SeparString::operator=( fms ); return *this; }
    FileMultiString&	operator=( const char* s )
			{ SeparString::operator=( s ); return *this; }

};


#endif
