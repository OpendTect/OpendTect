/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/


#include "bufstring.h"
#include "globexpr.h"
#include "arrayndimpl.h"
#include "perthreadrepos.h"
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
#ifdef __debug__
    if ( s && gtStr()==s )
    {
	if ( isStaticString(this) )
	{
	    pErrMsg("Static string compared to itself");
	}
    }
#endif
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


unsigned int OD::String::getLevenshteinDist( const char* s,
					     bool casesens ) const
{
    const unsigned int len1 = size();
    const unsigned int len2 = FixedString(s).size();
    if ( len1 == 0 ) return len2;
    if ( len2 == 0 ) return len1;
    const char* s1 = str();
    const char* s2 = s;

    Array2DImpl<unsigned int> d( len1+1, len2+1 );

    for ( unsigned int idx1=0; idx1<=len1; idx1++ )
	d.set( idx1, 0, idx1 );
    for ( unsigned int idx2=0; idx2<=len2; idx2++ )
	d.set( 0, idx2, idx2 );

    for ( unsigned int idx2=1; idx2<=len2; idx2++ )
    {
	for ( unsigned int idx1=1; idx1<=len1; idx1++ )
	{
	    const bool iseq = casesens ? s1[idx1-1] == s2[idx2-1]
			: toupper(s1[idx1-1]) == toupper(s2[idx2-1]);
	    if ( iseq )
		d.set( idx1, idx2, d.get(idx1-1,idx2-1) );
	    else
	    {
		const unsigned int delval = d.get( idx1-1, idx2 );
		const unsigned int insval = d.get( idx1, idx2-1 );
		const unsigned int substval = d.get( idx1-1, idx2-1 );
		d.set( idx1, idx2, 1 + (delval < insval
			? (delval<substval ? delval : substval)
		        : (insval<substval ? insval : substval)) );
	    }
	}
    }

    return d.get( len1, len2 );
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


od_uint64 OD::String::toUInt64() const
{
    const double val = toDouble();
    return od_uint64(val);
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
