#ifndef separstr_H
#define separstr_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		May 1995
 Contents:	String with a separator between the items
 RCS:		$Id: separstr.h,v 1.5 2001-02-13 17:15:46 bert Exp $
________________________________________________________________________

-*/

#include <bufstring.h>

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
			: sep(s.sep), rep(s.rep)
			{ sepstr[0] = s.sepstr[0]; sepstr[1] = s.sepstr[1]; }
    SeparString&	operator=( const char* s )
			{ rep = s; return *this; }

    const char*		operator[](unsigned int) const;
    const char*		from(unsigned int) const;
    void		add(const char*); // will also add empty strs
    SeparString&	operator +=(const char*);
    SeparString&	operator +=(int);
    SeparString&	operator +=(float);
    SeparString&	operator +=(double);
    			operator const char*() const	{ return rep; }
    			operator char*()		{ return rep; }
    unsigned int	size() const;
    char		sepChar() const			{ return sep; }

private:

    char		sep;
    char		sepstr[2];
    BufferString	rep;

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
