#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		April 2009
________________________________________________________________________

*/

#include "basicmod.h"
#include "odstring.h"



/*!\brief OD::String that holds an existing text string.

The string is assumed to be owned by someone else or be static. In any case, it
is assumed be be alive and well for the lifetime of the FixedString.

The FixedString is a light-weight, shallow adapter; it never changes the
underlying string.

*/

mExpClass(Basic) FixedString : public OD::String
{
public:

    inline		FixedString( const char* p = nullptr )
			    : str_(p)		{}
    inline FixedString&	operator=( const FixedString& fs )
						{ str_ = fs.str_; return *this;}
    inline FixedString&	operator=( const char* p )
						{ str_ = p; return *this; }
    FixedString&	operator=(const OD::String&);
				//!< Not impl - on purpose: too dangerous
    inline		operator const char*() const	{ return buf(); }

    inline bool		operator==( const FixedString& fs ) const
						{ return isEqual(fs.str_);}
    inline bool		operator!=( const FixedString& fs ) const
						{ return !isEqual(fs.str_);}
    inline bool		operator==( const char* s ) const
						{ return isEqual(s);}
    inline bool		operator!=( const char* s ) const
						{ return !isEqual(s);}
    inline bool		operator==( const OD::String& s ) const
						{ return isEqual(s.str()); }
    inline bool		operator!=( const OD::String& s ) const
						{ return !isEqual(s.str()); }
    bool		operator==(const BufferString&) const;
    bool		operator!=(const BufferString&) const;

    inline		operator bool() const	{ return !isNull(); }
    inline bool		operator !() const	{ return isNull(); }
    inline bool		isNull() const		{ return !str_; }
    inline char		firstChar() const override
						{ return str_ ? *str_ : '\0'; }

    static const FixedString& empty();


protected:

    inline const char*	gtBuf() const override
			{ return str_ ? str_ : ""; }
    inline const char*	gtStr() const override
			{ return !str_ || !*str_ ? nullptr : str_; }

    const char*	str_;

};


namespace Values
{

template<>
mClass(Basic) Undef<FixedString>
{
public:

    static FixedString	val()				{ return FixedString();}
    static bool		hasUdf()			{ return true; }
    static bool		isUdf( const FixedString& s )	{ return s.isEmpty(); }
    static void		setUdf( FixedString& s )	{ s = FixedString(); }

};

}


#ifndef __win__

// Avoid silent conversion general OD::String -> FixedString as it is dangerous.
void OD_Undef_FixedString_eq_bs_finder();
inline FixedString& FixedString::operator=(const OD::String&)
{ OD_Undef_FixedString_eq_bs_finder(); return *this; }

#endif


