#ifndef separstr_H
#define separstr_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		May 1995
 Contents:	String with a separator between the items
 RCS:		$Id: separstr.h,v 1.1.1.2 1999-09-16 09:19:14 arend Exp $
________________________________________________________________________

SeparString is a list encoded in a string where the items are separated by
a user chosen separator. The separator in the input is escaped with a backslash.
A `\' is encoded as `\\' .
A FileMultiString has the back-quote as separator.
Elements have an upper limit of PATH_LENGTH size.

@$*/

#include <gendefs.h>
#include <fixstring.h>


class SeparString
{
public:
			SeparString(const char* str=0,char separ=',');
			SeparString( const SeparString& s )
			: sep(s.sep), rep(s.rep)
			{ sepstr[0] = s.sepstr[0]; sepstr[1] = s.sepstr[1]; }
    SeparString&	operator=( const char* s )
			{ rep = s; return *this; }

    const char*		operator[](int) const;
    const char*		from(int) const;
    void		add(const char*); // will also add empty strs
    SeparString&	operator +=(const char*);
    SeparString&	operator +=(double);
    SeparString&	operator +=(int);
    			operator const char*() const	{ return rep; }
    			operator char*()		{ return rep; }
    int			size() const;
    char		sepChar() const			{ return sep; }

private:
    char		sep;
    char		sepstr[2];
    FixedString<1023>	rep;
};


class FileMultiString : public SeparString
{
public:
	FileMultiString(const char* str=0) : SeparString(str,'`') {}
};


/*$-*/
#endif
