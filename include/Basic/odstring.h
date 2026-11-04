#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "gendefs.h"
#include <string>

namespace Crypto { enum class Algorithm; }


/*!\brief encapsulates the read-access-only part of strings in OD.

  buf() always returns a valid null-terminated string.
  str() is the opposite: it always returns null if the string is empty

 */


mExpClass(Basic) StringBase
{
public:
    virtual		~StringBase();

    inline bool		operator==(const StringBase&) const;
    inline bool		operator!=(const StringBase&) const;
    inline bool		operator==(const char*) const;
    inline bool		operator!=(const char*) const;
    template <class T>
    inline bool		operator==(const T&) const;
    template <class T>
    inline bool		operator!=( const T& t ) const	{ return !(*this==t); }

    inline		const char* buf() const		{ return gtBuf(); }
    inline		const char* str() const		{ return gtStr(); }
    inline		operator const char*() const	{ return gtBuf(); }
    unsigned int	size() const;
    inline bool		isEmpty() const			{ return !gtStr(); }
    inline const char&	operator []( int idx ) const	{ return gtBuf()[idx]; }
    virtual char	firstChar() const	{ return gtBuf()[0]; }
    virtual char	lastChar() const	{ return gtBuf()[size()-1]; }

    bool		isEqual(const char*,
				OD::CaseSensitivity =OD::CaseSensitive) const;
    bool		isStartOf(const char*,
				OD::CaseSensitivity =OD::CaseSensitive) const;
    bool		startsWith(const char*,
				OD::CaseSensitivity =OD::CaseSensitive) const;
    bool		isEndOf(const char*,
				OD::CaseSensitivity =OD::CaseSensitive) const;
    bool		endsWith(const char*,
				OD::CaseSensitivity =OD::CaseSensitive) const;
    bool		matches(const char*,
				OD::CaseSensitivity =OD::CaseSensitive) const;

    bool		contains(char) const;
    bool		contains(const char*) const;
    const char*		find(char) const;
    const char*		findLast(char) const;
    const char*		find(const char*) const;
    const char*		findLast(const char*) const;

    unsigned int	count(char) const;
    unsigned int	getLevenshteinDist(const char*,bool casesens) const;

    bool		isNumber(bool int_only=false) const;
    bool		isYesNo() const;
    int			toInt() const;
    od_uint64		toUInt64() const;
    float		toFloat() const;
    double		toDouble() const;
    bool		toBool() const;
    std::wstring	toStdWString() const;
    const char*		getHash(Crypto::Algorithm) const;

    bool		operator >(const StringBase&) const;
    bool		operator <(const StringBase&) const;
    bool		operator >(const char*) const;
    bool		operator <(const char*) const;
    template <class T>
    inline bool		operator >(const T&) const;
    template <class T>
    inline bool		operator <(const T&) const;

    static const StringBase& empty();

protected:
			StringBase();

    virtual const char*	gtBuf() const			= 0;
			//!<\return empty even if underlying is null
    virtual const char*	gtStr() const			= 0;
			//!<\return null if empty

    inline		operator bool () const = delete;
};



inline bool StringBase::operator==( const StringBase& s ) const
{ return isEqual( s.gtStr() ); }
inline bool StringBase::operator!=( const StringBase& s ) const
{ return !isEqual( s.gtStr() ); }
inline bool StringBase::operator==( const char* s ) const
{ return isEqual( s ); }
inline bool StringBase::operator!=( const char* s ) const
{ return !isEqual( s ); }

inline bool StringBase::operator >( const StringBase& oth ) const
{ return *this > oth.gtStr(); }
inline bool StringBase::operator <( const StringBase& oth ) const
{ return *this < oth.gtStr(); }

template <class T> inline bool StringBase::operator==( const T& t ) const
{ return isEqual( toString(t) ); }
template <class T> inline bool StringBase::operator >( const T& t ) const
{ return *this > ( toString( t ) ); }
template <class T> inline bool StringBase::operator <( const T& t ) const
{ return *this < ( toString( t ) ); }

