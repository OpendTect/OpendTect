#ifndef separstr_h
#define separstr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		May 1995
 Contents:	String with a separator between the items
 RCS:		$Id: separstr.h,v 1.12 2008-06-20 11:41:01 cvsraman Exp $
________________________________________________________________________

-*/

#include "bufstring.h"

#define mMaxSepItem 1023


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
			: sep_(s.sep_), rep_(s.rep_)
			{ sepstr_[0] = s.sepstr_[0]; sepstr_[1] = s.sepstr_[1];}
    SeparString&	operator=( const char* s )
			{ rep_ = s; return *this; }

    const char*		operator[](unsigned int) const;
    inline const char*	operator[](int x) const
			{ return (*this)[ (unsigned int)x ]; }

    const char*		from(unsigned int) const;
    void		add(const char*); // will also add empty strs
    SeparString&	operator +=(const char*);
    SeparString&	operator +=(int);
    SeparString&	operator +=(float);
    SeparString&	operator +=(double);
    			operator const char*() const	{ return rep_; }
    char*		buf()				{ return rep_.buf(); }
    unsigned int	size() const;
    bool		isEmpty() const			{ return !size(); }
    char		sepChar() const			{ return sep_; }
    void		setSepChar(char);
    void		replaceSepChar(char newsep);
    static char		getPathSepChar();

private:

    char		sep_;
    char		sepstr_[2];
    BufferString	rep_;

};


/*!\brief SeparString with backquotes as separators (for dGB ascii files) */

class FileMultiString : public SeparString
{
public:

	FileMultiString(const char* str=0) : SeparString(str,'`') {}

};


/*!\brief SeparString with dots as separators (for hierarchical constructs) */

class MultiKeyString : public SeparString
{
public:

	MultiKeyString(const char* str=0) : SeparString(str,'.') {}
	MultiKeyString( const MultiKeyString& baskey, const char* key )
		: SeparString(baskey)
	{
	    if ( key && *key ) *this += key;
	}
	MultiKeyString( const char* baskey, const char* key )
		: SeparString(baskey,'.')
	{
	    if ( key && *key ) *this += key;
	}

};


#endif
