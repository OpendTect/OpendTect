#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "odstring.h"



/*!\brief OD::String that holds an existing text string.

The string is assumed to be owned by someone else or be static. In any case, it
is assumed be be alive and well for the lifetime of the StringView.

The StringView is a light-weight, shallow adapter; it never changes the
underlying string. It is comparable to std::string_view (C++17).

*/

mExpClass(Basic) StringView : public OD::String
{
public:
			StringView(const char* =nullptr);
			~StringView();

    inline StringView&	operator=( const StringView& fs )
						{ str_ = fs.str_; return *this;}
    inline StringView&	operator=( const char* p )
						{ str_ = p; return *this; }
    inline bool		operator==( const StringView& fs ) const
						{ return isEqual(fs.str_);}
    inline bool		operator!=( const StringView& fs ) const
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

    inline		operator bool() const			= delete;
    inline bool		operator !() const			= delete;

    inline bool		isNull() const		{ return !str_; }
    inline char		firstChar() const override
						{ return str_ ? *str_ : '\0'; }

    static const StringView& empty();


protected:

    inline const char*	gtBuf() const override
			{ return str_ ? str_ : ""; }
    inline const char*	gtStr() const override
			{ return !str_ || !*str_ ? nullptr : str_; }

    const char* str_;

    StringView(const OD::String&)		= delete;
    StringView & operator=(const OD::String&)	= delete;

};


namespace Values
{

template<>
mClass(Basic) Undef<StringView>
{
public:

    static StringView	val()				{ return StringView();}
    static bool		hasUdf()			{ return true; }
    static bool		isUdf( const StringView& s )	{ return s.isEmpty(); }
    static void		setUdf( StringView& s ) { s = StringView(); }

};

}

