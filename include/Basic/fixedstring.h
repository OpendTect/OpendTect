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

#include "gendefs.h"
#include <iosfwd>



/*!
\brief Class that holds a text string, and provides basic services around it.
The string is assumed to be owned by someone else or be static. In any case, it
is assumed be be alive and well for the lifetime of the FixedString.
*/

mExpClass(Basic) FixedString
{
public:

    inline		FixedString( const char* p = 0 )
			    : ptr_(p)		{}

    inline FixedString&	operator=( const char* p )
						{ ptr_ = p; return *this; }

    bool		operator==(const char*) const;
    bool		operator==(const BufferString&) const;
    inline bool		operator!=( const BufferString& bs ) const
						{ return !(*this == bs); }
    inline bool		operator!=( const char* s ) const
						{ return !(*this == s); }
    inline bool		operator==( const FixedString& fs ) const
						{ return *this == fs.ptr_;}
    inline bool		operator!=( const FixedString& fs ) const
						{ return *this != fs.ptr_;}

    inline bool		isNull() const		{ return !ptr_; }
    inline bool		isEmpty() const		{ return !ptr_ || !*ptr_; }
    int			size() const;

    inline		operator const char*() const
						{ return buf(); }
    inline		operator bool() const	{ return !isEmpty(); }
    inline bool		operator!() const	{ return isEmpty(); }

    inline const char*	buf() const		{ return ptr_ ? ptr_ : ""; }
    inline const char*	str() const		{ return isEmpty() ? 0 : ptr_; }
    inline char		firstChar() const	{ return ptr_ ? *ptr_ : '\0'; }

    FixedString&	operator=(const BufferString&);
				//!< Not impl - on purpose: too dangerous

protected:

    const char*	ptr_;

};


inline bool operator==( const char* s, const FixedString& fs )
{
    return fs == s;
}


inline bool operator!=( const char* s, const FixedString& fs )
{
    return fs != s;
}


mGlobal(Basic) const char* toString(const FixedString&);


namespace Values
{

template<>
mClass(Basic) Undef<FixedString>
{
public:

    static FixedString	val()				{ return FixedString();}
    static bool		hasUdf()			{ return true; }
    static bool		isUdf(const FixedString& s)	{return s.isEmpty();}
    static void		setUdf(FixedString& s)		{ s = FixedString(); }

};

}


mGlobal(Basic) std::ostream& operator <<(std::ostream&,const FixedString&);



void Qq_FixedString_eq_bs_qQ();

inline FixedString& FixedString::operator=(const BufferString&)
{
    Qq_FixedString_eq_bs_qQ();
    ptr_ = 0;
    return *this;
}


#endif
