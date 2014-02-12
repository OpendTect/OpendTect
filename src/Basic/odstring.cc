/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "bufstring.h"
#include "globexpr.h"
#include <string.h>


static FixedString emptyfixedstring( "" );

const OD::String& OD::String::empty()
{ return emptyfixedstring; }
const FixedString& FixedString::empty()
{ return emptyfixedstring; }


bool FixedString::operator==( const BufferString& s ) const
{ return isEqual(s.str()); }
bool FixedString::operator!=( const BufferString& s ) const
{ return !isEqual(s.str()); }


unsigned int OD::String::size() const
{
    const char* my_str = gtStr();
    return my_str ? (unsigned int)strlen( my_str ) : 0;
}


bool OD::String::operator >( const char* s ) const
{
    const char* my_str = gtStr();
    return s && my_str ? strcmp(my_str,s) > 0 : (bool)my_str;
}


bool OD::String::operator <( const char* s ) const
{
    const char* my_str = gtStr();
    return s && my_str ? strcmp(my_str,s) < 0 : (bool)s;
}


#define mIsEmpty(str) (!str || !*str)

#define mRetEmptyEquality(s1,s2) \
    if ( s1 == s2 || ( mIsEmpty(s1) && mIsEmpty(s2) ) )\
	return true; \
    else if ( mIsEmpty(s1) || mIsEmpty(s2) ) \
	return false;\

#define mGetMeForEquality() \
    const char* me = gtStr(); \
    mRetEmptyEquality( me, s )

#define mIsInsens() (sens == CaseInsensitive)

bool OD::String::isEqual( const char* s, CaseSensitivity sens ) const
{
    mGetMeForEquality();
    return mIsInsens() ? caseInsensitiveEqual(me,s,0) : !strcmp( me, s );
}


bool OD::String::isStartOf( const char* s, CaseSensitivity sens ) const
{
    mGetMeForEquality();
    return mIsInsens() ? stringStartsWithCI(me,s) : stringStartsWith(me,s);
}


bool OD::String::startsWith( const char* s, CaseSensitivity sens ) const
{
    mGetMeForEquality();
    return mIsInsens() ? stringStartsWithCI(s,me) : stringStartsWith(s,me);
}


bool OD::String::isEndOf( const char* s, CaseSensitivity sens ) const
{
    mGetMeForEquality();
    return mIsInsens() ? stringEndsWithCI(me,s) : stringEndsWith(me,s);
}


bool OD::String::endsWith( const char* s, CaseSensitivity sens ) const
{
    mGetMeForEquality();
    return mIsInsens() ? stringEndsWithCI(s,me) : stringEndsWith(s,me);
}


bool OD::String::matches( const char* s, CaseSensitivity sens ) const
{
    mGetMeForEquality();
    return GlobExpr(s,!mIsInsens()).matches( me );
}


bool OD::String::contains( char tofind ) const
{
    const char* me = gtStr();
    return me ? (bool) firstOcc( me, tofind ) : false;
}


bool OD::String::contains( const char* tofind ) const
{
    return (bool) firstOcc( gtStr(), tofind );
}


const char* OD::String::find( char c ) const
{ return firstOcc( str(), c ); }
const char* OD::String::findLast( char c ) const
{ return lastOcc( str(), c ); }
const char* OD::String::find( const char* s ) const
{ return firstOcc( str(), s ); }
const char* OD::String::findLast( const char* s ) const
{ return lastOcc( str(), s ); }


unsigned int OD::String::count( char tocount ) const
{
    const char* ptr = gtStr();
    int ret = 0;
    if ( !ptr )
	return ret;

    while ( *ptr )
    {
	if ( *ptr == tocount )
	    ret++;
	ptr++;
    }

    return ret;
}


bool OD::String::isNumber( bool int_only ) const
{
    return isNumberString( str(), int_only );
}


bool OD::String::isYesNo() const
{
    return isEqual("yes",CaseInsensitive) || isEqual("no",CaseInsensitive);
}


int OD::String::toInt() const
{
    return ::toInt( str() );
}


float OD::String::toFloat() const
{
    return ::toFloat( str() );
}


double OD::String::toDouble() const
{
    return ::toDouble( str() );
}


bool OD::String::toBool() const
{
    return ::toBool( str() );
}
