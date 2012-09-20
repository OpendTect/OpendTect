#ifndef fixedstring_h
#define fixedstring_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		April 2009
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"
#include "bufstring.h"


/*! Class that holds a text string, and provides basic services around it. The
    string is assumed to be owned by someone else or be static. In any case, it
    is assumed be be alive and well for the lifetime of the FixedString. */

mClass(Basic) FixedString
{
public:
		FixedString(const char* p = 0 ) : ptr_(p) {}
    FixedString& operator=(const FixedString& f) { return *this = f.ptr_; }
    FixedString& operator=(const char* p)	{ ptr_ = p; return *this; }
    FixedString& operator=(const BufferString& b){ptr_=b.buf();return *this;}

    bool	operator==(const char*) const;
    bool	operator!=(const char* s) const		{ return !(*this==s); }
    bool	operator==(const FixedString& f) const	{ return *this==f.ptr_;}
    bool	operator!=(const FixedString& f) const	{ return *this!=f.ptr_;}
    bool	operator!() const			{ return isEmpty(); }

    bool	isNull() const			{ return !ptr_; }
    bool	isEmpty() const			{ return !ptr_ || !*ptr_; }
    int		size() const;

		operator const char*() const   	{ return str(); }

    const char*	str() const 			{ return isEmpty() ? 0 : ptr_; }

protected:

    const char*	ptr_;
};

inline bool operator==(const char* a, const FixedString& b)
{ return b==a; }
 
 
inline bool operator!=(const char* a, const FixedString& b)
{ return b!=a; }


#endif

