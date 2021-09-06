#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2013
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"


namespace OD
{


/*!\brief encapsulates the read-access-only part of strings in OD.

  buf() always returns a valid null-terminated string.
  str() is the opposite: it always returns null if the string is empty

 */


mExpClass(Basic) String
{
public:

    typedef int		size_type;
    typedef int		idx_type;

    virtual		~String()		{}

    inline bool		operator==(const String&) const;
    inline bool		operator!=(const String&) const;
    inline bool		operator==(const char*) const;
    inline bool		operator!=(const char*) const;
    template <class T>
    inline bool		operator==(const T&) const;
    template <class T>
    inline bool		operator!=( const T& t ) const	{ return !(*this==t); }

    inline		const char* buf() const		{ return gtBuf(); }
    inline		const char* str() const		{ return gtStr(); }
    inline		operator const char*() const	{ return gtBuf(); }
    size_type		size() const;
    inline bool		isEmpty() const			{ return !gtStr(); }
    inline const char&	operator []( idx_type idx ) const
						{ return gtBuf()[idx]; }
    virtual char	firstChar() const	{ return gtBuf()[0]; }
    virtual char	lastChar() const	{ return gtBuf()[size()-1]; }

#   define		mODStringDefSens CaseSensitivity cs=CaseSensitive
    bool		isEqual(const char*,mODStringDefSens) const;
    bool		isStartOf(const char*,mODStringDefSens) const;
    bool		startsWith(const char*,mODStringDefSens) const;
    bool		isEndOf(const char*,mODStringDefSens) const;
    bool		endsWith(const char*,mODStringDefSens) const;
    bool		matches(const char*,mODStringDefSens) const;

    bool		contains(char) const;
    bool		contains(const char*) const;
    const char*		find(char) const;
    const char*		findLast(char) const;
    const char*		find(const char*) const;
    const char*		findLast(const char*) const;

    unsigned int	count(char) const;
    unsigned int	getLevenshteinDist(const char*,mODStringDefSens) const;

    bool		isNumber(bool int_only=false) const;
    bool		isYesNo() const;
    int			toInt() const;
    od_uint64		toUInt64() const;
    float		toFloat() const;
    double		toDouble() const;
    bool		toBool() const;

    bool		operator >(const String&) const;
    bool		operator <(const String&) const;
    bool		operator >(const char*) const;
    bool		operator <(const char*) const;
    template <class T>
    inline bool		operator >(const T&) const;
    template <class T>
    inline bool		operator <(const T&) const;

    static const String& empty();

protected:

    virtual const char*	gtBuf() const			= 0;
			//!<\return empty even if underlying is null
    virtual const char*	gtStr() const			= 0;
			//!<\return null if empty

};



inline bool String::operator==( const String& s ) const
{ return isEqual( s.gtStr() ); }
inline bool String::operator!=( const String& s ) const
{ return !isEqual( s.gtStr() ); }
inline bool String::operator==( const char* s ) const
{ return isEqual( s ); }
inline bool String::operator!=( const char* s ) const
{ return !isEqual( s ); }

inline bool String::operator >( const String& oth ) const
{ return *this > oth.gtStr(); }
inline bool String::operator <( const String& oth ) const
{ return *this < oth.gtStr(); }

template <class T> inline bool String::operator==( const T& t ) const
{ return isEqual( toString(t) ); }
template <class T> inline bool String::operator >( const T& t ) const
{ return *this > ( toString( t ) ); }
template <class T> inline bool String::operator <( const T& t ) const
{ return *this < ( toString( t ) ); }


} // namespace OD
