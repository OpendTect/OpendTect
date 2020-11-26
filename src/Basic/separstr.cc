/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 11-4-1994
 * FUNCTION : Functions concerning delimiter separated string lists
-*/


#include "separstr.h"

#include "bufstringset.h"
#include "dbkey.h"
#include "keystrs.h"
#include <string.h>


SeparStringSB& SeparStringSB::operator =( const SeparStringSB& ss )
{
    if ( this != &ss )
    {
	rep_ = ss.rep_;
	sep_[0] = ss.sep_[0];
    }
    return *this;
}


SeparStringSB& SeparStringSB::operator =( const char* s )
{
    if ( s != rep_.result() )
	initRep( s );

    return *this;
}


const char* SeparStringSB::getEscaped( const char* string, char sep ) const
{
    if ( !string )
	return 0;

    retstr_.setBufSize( 2*strlen(string) + 1 );

    char* writeptr = retstr_.getCStr();
    while ( *string )
    {
	if ( *string=='\\' || *string==sep )
	    *writeptr++ = '\\';

	*writeptr++ = *string++;
    }
    *writeptr = '\0';
    return retstr_.buf();
}


const char* SeparStringSB::getUnescaped( const char* startptr,
				       const char* nextsep ) const
{
    if ( !startptr )
	return 0;

    const int len = nextsep<startptr ? strlen(startptr) :
				       (int)(nextsep - startptr);
    retstr_.setBufSize( len + 1 );

    char* writeptr = retstr_.getCStr();
    while ( *startptr && startptr!=nextsep )
    {
	if ( *startptr == '\\' )
	    startptr++;

	*writeptr++ = *startptr++;
    }
    *writeptr = '\0';
    return retstr_.buf();
}


static bool isEscapedChar( const char* buf, const char* ptr )
{
    if ( !buf || !ptr )
	return false;

    if ( buf>ptr-1 || *(ptr-1)!='\\' )
	return false;
    if ( buf>ptr-2 || *(ptr-2)!='\\' )
	return true;

    return isEscapedChar( buf, ptr-2 );
}


const char* SeparStringSB::findSeparator( const char* startptr ) const
{
    if ( !startptr )
	return 0;

    const char* ptr = firstOcc( startptr, sep_[0] );
    if ( ptr && isEscapedChar(rep_.result(), ptr)  )
	return findSeparator( ptr+1 );

    return ptr;
}


static bool isSurelyUnescaped( const char* str, char sep )
{
    if ( !str )
	return false;

    const char* ptr = str;
    while ( true )
    {
	if ( *ptr!='\\' && *ptr!=sep && isEscapedChar(str, ptr) )
	    return true;

	if ( !*ptr++ )
	    break;
    }
    return false;
}


void SeparStringSB::initRep( const char* string )
{
    /*	Escape backslashes if str contains old-format separ-string read from
	file. Detection only fails if all backslashes in the old-format string
	are pairwise or precede a separation character (highly unlikely).
	New code should not rely on this!
     */
    if ( isSurelyUnescaped(string, sep_[0]) )
    {
	rep_ = getEscaped( string, '\0' );
	return;
    }

    rep_ = string;
}


int SeparStringSB::size() const
{
    const char* ptr = rep_.result();
    if ( !*ptr )
	return 0;

    int sz = *ptr==sep_[0] ? 1 : 0;

    while ( ptr )
    {
	sz++;
	ptr = findSeparator( ptr+1 );
    }

    return sz;
}


FixedString SeparStringSB::operator[]( int elemnr ) const
{
    if ( elemnr < 0 )
	return sKey::EmptyString();

    const char* startptr = rep_.result();
    while ( *startptr )
    {
	const char* nextsep = findSeparator( startptr );

	if ( !elemnr )
	    return getUnescaped( startptr, nextsep );

	if ( !nextsep )
	    return sKey::EmptyString();

	elemnr--;
	startptr = nextsep+1;
    }

    return sKey::EmptyString();
}


FixedString SeparStringSB::from( int idx ) const
{
    const char* ptr = rep_.result();
    for ( ; idx!=0; idx-- )
    {
	ptr = findSeparator( ptr );
	if ( ptr ) ptr++;
    }
    return ptr;
}


SeparStringSB& SeparStringSB::add( const BufferStringSet& bss )
{
    for ( int idx=0; idx<bss.size(); idx++ )
	add( bss.get(idx) );
    return *this;
}


SeparStringSB& SeparStringSB::add( const SeparStringSB& ss )
{
    for ( int idx=0; idx<ss.size(); idx++ )
	add( ss[idx] );
    return *this;
}


SeparStringSB& SeparStringSB::add( const char* string )
{
    if ( *rep_.result() )
	rep_.add( sep_ );
    if ( !string || !*string )
	string = " ";
    rep_.add( getEscaped(string ,sep_[0]) );
    return *this;
}


SeparStringSB& SeparStringSB::add( const DBKey& dbky, bool withsurvloc )
{
    return add( dbky.toString(withsurvloc) );
}


#define mDeclGetFn(typ,fn) \
typ SeparStringSB::fn( int idx ) const \
{ \
    return Conv::to<typ>( (*this)[idx] ); \
}
mDeclGetFn(od_int16,getI16Value)
mDeclGetFn(od_uint16,getUI16Value)
mDeclGetFn(od_int32,getIValue)
mDeclGetFn(od_uint32,getUIValue)
mDeclGetFn(od_int64,getI64Value)
mDeclGetFn(od_uint64,getUI64Value)
mDeclGetFn(float,getFValue)
mDeclGetFn(double,getDValue)
mDeclGetFn(bool,getYN)


int SeparStringSB::indexOf( const char* string ) const
{
    if ( !string ) return -1;

    const char* startptr = rep_.result();
    int elemnr = 0;
    while ( *startptr )
    {
	const char* nextsep = findSeparator( startptr );
	FixedString elemstr = getUnescaped( startptr, nextsep );

	if ( elemstr == string )
	    return elemnr;

	if ( !nextsep )
	    return -1;

	elemnr++;
	startptr = nextsep + 1;
    }

    return -1;
}


void SeparStringSB::setSepChar( char newchar )
{
    SeparStringSB ss( "", newchar );
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	ss.add( (*this)[idx] );
    *this = ss;
}


//Former SeparString class

SeparString& SeparString::operator =( const SeparString& ss )
{
    if ( this != &ss )
    {
	rep_ = ss.rep_;
	sep_[0] = ss.sep_[0];
    }
    return *this;
}


SeparString& SeparString::operator =( const char* s )
{
    if ( s != rep_.buf() )
	initRep( s );

    return *this;
}


const char* SeparString::getEscaped( const char* str, char sep ) const
{
    if ( !str )
	return 0;

    retstr_.setBufSize( 2*strlen(str) + 1 );

    char* writeptr = retstr_.getCStr();
    while ( *str )
    {
	if ( *str=='\\' || *str==sep )
	    *writeptr++ = '\\';

	*writeptr++ = *str++;
    }
    *writeptr = '\0';
    return retstr_.buf();
}


const char* SeparString::getUnescaped( const char* startptr,
				       const char* nextsep ) const
{
    if ( !startptr )
	return 0;

    const int len = nextsep<startptr ? strlen(startptr) :
				       (int)(nextsep - startptr);
    retstr_.setBufSize( len + 1 );

    char* writeptr = retstr_.getCStr();
    while ( *startptr && startptr!=nextsep )
    {
	if ( *startptr == '\\' )
	    startptr++;

	*writeptr++ = *startptr++;
    }
    *writeptr = '\0';
    return retstr_.buf();
}


const char* SeparString::findSeparator( const char* startptr ) const
{
    if ( !startptr )
	return 0;

    const char* ptr = firstOcc( startptr, sep_[0] );
    if ( ptr && isEscapedChar(rep_.buf(), ptr)	)
	return findSeparator( ptr+1 );

    return ptr;
}


void SeparString::initRep( const char* str )
{
    /*  Escape backslashes if str contains old-format separ-string read from
	file. Detection only fails if all backslashes in the old-format string
	are pairwise or precede a separation character (highly unlikely).
	New code should not rely on this!
     */
    if ( isSurelyUnescaped(str, sep_[0]) )
    {
	rep_ = getEscaped( str, '\0' );
	return;
    }

    rep_ = str;
}


int SeparString::size() const
{
    const char* ptr = rep_.buf();
    if ( !*ptr )
	return 0;

    int sz = *ptr==sep_[0] ? 1 : 0;

    while ( ptr )
    {
	sz++;
	ptr = findSeparator( ptr+1 );
    }

    return sz;
}


FixedString SeparString::operator[]( int elemnr ) const
{
    if ( elemnr < 0 )
	return sKey::EmptyString();

    const char* startptr = rep_.buf();
    while ( *startptr )
    {
	const char* nextsep = findSeparator( startptr );

	if ( !elemnr )
	    return getUnescaped( startptr, nextsep );

	if ( !nextsep )
	    return sKey::EmptyString();

	elemnr--;
	startptr = nextsep+1;
    }

    return sKey::EmptyString();
}


FixedString SeparString::from( int idx ) const
{
    const char* ptr = rep_.buf();
    for ( ; idx!=0; idx-- )
    {
	ptr = findSeparator( ptr );
	if ( ptr ) ptr++;
    }
    return ptr;
}


SeparString& SeparString::add( const BufferStringSet& bss )
{
    for ( int idx=0; idx<bss.size(); idx++ )
	add( bss.get(idx) );
    return *this;
}


SeparString& SeparString::add( const SeparString& ss )
{
    for ( int idx=0; idx<ss.size(); idx++ )
	add( ss[idx] );
    return *this;
}


SeparString& SeparString::add( const char* str )
{
    if ( *rep_.buf() ) rep_ += sep_;
    if ( !str || !*str ) str = " ";
    rep_ += getEscaped( str , sep_[0] );
    return *this;
}

#undef mDeclGetFn
#define mDeclGetFn(typ,fn) \
typ SeparString::fn( int idx ) const \
{ \
    return Conv::to<typ>( (*this)[idx] ); \
}
mDeclGetFn(int,getIValue)
mDeclGetFn(od_uint32,getUIValue)
mDeclGetFn(od_int64,getI64Value)
mDeclGetFn(od_uint64,getUI64Value)
mDeclGetFn(float,getFValue)
mDeclGetFn(double,getDValue)
mDeclGetFn(bool,getYN)


int SeparString::indexOf( const char* str ) const
{
    if ( !str ) return -1;

    const char* startptr = rep_.buf();
    int elemnr = 0;
    while ( *startptr )
    {
	const char* nextsep = findSeparator( startptr );
	FixedString elemstr = getUnescaped( startptr, nextsep );

	if ( elemstr == str )
	    return elemnr;

	if ( !nextsep )
	    return -1;

	elemnr++;
	startptr = nextsep + 1;
    }

    return -1;
}


void SeparString::setSepChar( char newchar )
{
    SeparString ss( "", newchar );
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	ss.add( (*this)[idx] );
    *this = ss;
}
