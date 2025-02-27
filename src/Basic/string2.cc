/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "string2.h"

#include "compoundkey.h"
#include "keystrs.h"
#include "math2.h"
#include "nrbytes2string.h"
#include "odcomplex.h"
#include "odmemory.h"
#include "perthreadrepos.h"
#include "survinfo.h"
#include "undefval.h"
#include "bufstringset.h"

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>

#ifndef OD_NO_QT
# include <QString>
#endif

#ifdef __win__
# define sDirSep        "\\"
#else
# define sDirSep        "/"
#endif

#define mToSqMileFactorD	3.58700642792e-8 // ft^2 to mile^2


static void rmSingleCharFromString( char* ptr )
{
    while ( *ptr )
    {
	*ptr = *(ptr+1);
	ptr++;
    }
}


static void cleanupMantissa( char* ptrdot, char* ptrend )
{
    if ( ptrend )
	ptrend--;
    else
	ptrend = ptrdot + StringView(ptrdot).size() - 1;

    while ( ptrend > ptrdot && *ptrend == '0' )
    {
	rmSingleCharFromString( ptrend );
	ptrend--;
    }

    if ( *(ptrdot+1) == '\0' || *(ptrdot+1) == 'e' )
	rmSingleCharFromString( ptrdot );
}


#define mSetStrTo0(str,ret) { *str = '0'; *(str+1) = '\0'; ret; }

static void finalCleanupNumberString( char* str )
{
    // We don't need any '+'s - remove them
    char* ptr = firstOcc( str, '+' );
    while ( ptr )
    {
	rmSingleCharFromString( ptr );
	ptr = firstOcc( str, '+' );
    }

    if ( !*str )
	mSetStrTo0(str,return)

    char* ptrexp = firstOcc( str, 'e' );
    if ( !ptrexp )
    {
	ptrexp = firstOcc( str, 'E' );
	if ( ptrexp )
	    *ptrexp = 'e'; // so we'll never have 'E', always 'e'
    }
    if ( ptrexp == str )
	mSetStrTo0(str,return)
    else if ( ptrexp )
    {
	// Remove leading '0's in exponent
	char* ptrexpval = ptrexp + 1;
	if ( *ptrexpval == '-' )
	    ptrexpval++;
	while ( *ptrexpval == '0' )
	    rmSingleCharFromString( ptrexpval );
	if ( !*ptrexpval ) // there were only zeros after the 'e'
	    *ptrexp = '\0';
    }

    char* ptrdot = firstOcc( str, '.' );
    if ( !ptrdot ) return;

    cleanupMantissa( ptrdot, ptrexp );
    if ( !*str )
	mSetStrTo0(str,return)

    // Remove trailing '0's in mantissa
    char* ptrend = firstOcc( str, 'e' );
    if ( !ptrend )
	ptrend = str + StringView(str).size() - 1;
    if ( ptrexp )
    {
	char* ptrlast = ptrexp-1;
	while ( *ptrlast == '0' )
	    rmSingleCharFromString( ptrlast );
    }

    if ( !*str )
	mSetStrTo0(str,return)
}


const char* getYesNoString( bool yn )
{
    const char* strs[] = { "Yes", "No" };
    return yn ? strs[0] : strs[1];
}


void removeTrailingBlanks( char* str )
{
    if ( !str || ! *str ) return;

    char* endptr = str + strlen(str) - 1;
    while( iswspace(*endptr) )
    {
	*endptr = '\0';
	if ( str == endptr ) break;
	endptr--;
    }
}


const char* getBytesString( od_uint64 sz )
{
    NrBytesToStringCreator converter;
    converter.setUnitFrom( sz );
    return converter.getString( sz, 0 );
}


const char* getDistUnitString( bool isfeet, bool wb )
{
    if ( isfeet )
	return wb ? "(ft)" : "ft";

    return wb ? "(m)" : "m";
}


const char* getTimeUnitString( bool internalstandard, bool wb )
{
    if ( internalstandard )
	return wb ? "(s)" : "s";

    return wb ? "(ms)" : "ms";
}


const char* getVelUnitString( bool isfeet, bool wb )
{
    if ( isfeet )
	return wb ? "(ft/s)" : "ft/s";

    return wb ? "(m/s)" : "m/s";
}


bool yesNoFromString( const char* str )
{
    if ( !str ) return false;
    mSkipBlanks( str );
    return *str == 'Y' || *str == 'y' || *str == 'T' || *str == 't';
}


bool isBoolString( const char* str )
{
    StringView fstr( str );
    return fstr.isEqual("True",OD::CaseInsensitive) ||
	   fstr.isEqual("False",OD::CaseInsensitive) ||
	   fstr.isEqual("Yes",OD::CaseInsensitive) ||
	   fstr.isEqual("No",OD::CaseInsensitive);
}


bool isNumberString( const char* str, bool int_only )
{
    if ( !str || !*str )
	return false;


    bool curisdigit = iswdigit(*str);
    if ( !*(str+1) )
	return curisdigit;

    bool dotseen = false;
    if ( !curisdigit )
    {
	dotseen = *str == '.';
	if ( *str != '+' && *str != '-' && (int_only || !dotseen) )
	    return false;
    }

    str++;
    bool eseen = false;
    bool previsdigit = curisdigit;
    curisdigit = iswdigit(*str);
    if ( !curisdigit )
    {
	dotseen = *str == '.';
	if ( !previsdigit )
	{
	    if ( !dotseen || (*(str-1) != '+' && *(str-1) != '-') )
		return false;
	}
	eseen = *str == 'e' || *str == 'E';
	if ( int_only || (!dotseen && !eseen) )
	    return false;
	if ( eseen && *(str+1) && (*(str+1) == '+' || *(str+1) == '-') )
	    str++;
    }

    str++;
    while ( *str )
    {
	if ( !iswdigit(*str) )
	{
	    if ( (*str == 'e' || *str == 'E') )
	    {
		if ( eseen )
		    return false;

		eseen = true;
		if ( *(str+1) && (*(str+1) == '+' || *(str+1) == '-') )
		    str++;
	    }
	    else if ( *str == '.' )
	    {
		if ( dotseen || eseen )
		    return false;
		dotseen = true;
	    }
	    else
		return false;
	}

	str++;
    }

    return !int_only || !dotseen;
}


bool isAlphaNumString( const char* str, bool allowspace )
{
    if ( !str || !*str )
	return false;

#define mCheckChar (iswalnum(*str) || (allowspace && *str==' '))
    while ( *str )
    {
	if ( !mCheckChar )
	    return false;

	str++;
    }
#undef mCheckChar

    return true;
}


bool isAlpha( char c )
{
    return std::isalpha( c );
}


bool isDigit( char c )
{
    return std::isdigit( c );
}


void cleanupString( char* str, bool spaceallow, bool fsepallow, bool dotallow )
{
    if ( !str )
	return;

    while ( *str )
    {
	if ( !iswalnum(*str) )
	{
	    bool dorepl = true;
	    switch ( *str )
	    {
	    case ' ': case '\n' : case '\t':
			if ( spaceallow )	dorepl = false;	break;
	    case '.':	if ( dotallow )		dorepl = false;	break;
	    case '+':	case '-':		dorepl = false;	break;
	    default:						break;
	    }
	    if ( fsepallow )
	    {
		if ( *str == *sDirSep )
		    dorepl = false;
#ifdef __win__
		if ( *str == ':' )
		    dorepl = false;
#endif
	    }

	    if ( dorepl )
		*str = '_';
	}
	str++;
    }
}


bool caseInsensitiveEqual( const char* str1, const char* str2, int nrchar )
{
    if ( !str1 && !str2 ) return true;
    if ( !str1 || !str2 ) return false;

    int chcount = 1;
    while ( *str1 && *str2 )
    {
	if ( tolower(*str1) != tolower(*str2) )
	    return false;
	str1++ ; str2++; chcount++;
	if ( nrchar>0 && chcount>nrchar )
	    return true;
    }

    return !*str1 && !*str2;
}


static bool getStringStartsWith( const char* str1, const char* str2, int ci )
{
    if ( !str1 && !str2 ) return true;
    if ( !str1 || !str2 ) return false;
    if ( ! *str1 )	  return true;
    if ( ! *str2 )	  return false;

    while ( *str1 )
    {
	if ( ci )
	    { if ( tolower(*str1) != tolower(*str2) ) return false; }
	else
	    { if ( *str1 != *str2 ) return false; }
	str1++ ; str2++;
    }
    return true;
}


bool stringStartsWith( const char* str1, const char* str2 )
{ return getStringStartsWith( str1, str2, false ); }
bool stringStartsWithCI( const char* str1, const char* str2 )
{ return getStringStartsWith( str1, str2, true ); }


static bool getStringEndsWith( const char* str1, const char* str2, int ci )
{
    if ( !str1 && !str2 )	return true;
    if ( !str1 || !str2 )	return false;
    if ( !*str1 )		return true;
    if ( !*str2 )		return false;
    const char* start1 = str1; const char* start2 = str2;
    while ( *str1 ) str1++; while ( *str2 ) str2++;
    str1-- ; str2--;

    while ( true )
    {
	if ( ci )
	    { if ( tolower(*str1) != tolower(*str2) ) return false; }
	else
	    { if ( *str1 != *str2 ) return false; }
	str1-- ; str2--;
	if ( str1 == start1-1 )
	    break;
	if ( str2 == start2-1 )
	    return false;
    }
    return true;
}

bool stringEndsWith( const char* str1, const char* str2 )
{ return getStringEndsWith( str1, str2, false ); }
bool stringEndsWithCI( const char* str1, const char* str2 ) \
{ return getStringEndsWith( str1, str2, true ); }


static const char* getLastOcc( const char* str, const char* tofind )
{
    if ( !str )
	return 0;
    if ( !tofind )
	tofind = "";

    const int slen = strlen( str );
    const int flen = strlen( tofind );
    if ( flen == 0 )
	return str + slen;
    else if ( flen == 1 )
	return strrchr( str, *tofind );

    BufferString srev( str ), frev( tofind );
    for ( int idx=0; idx<slen/2; idx++ )
	Swap( srev[idx], srev[slen-idx-1] );
    for ( int idx=0; idx<flen/2; idx++ )
	Swap( frev[idx], frev[flen-idx-1] );
    const char* ptr = strstr( srev.buf(), frev.buf() );
    if ( !ptr )
	return ptr;
    return str + (slen - (ptr-srev.buf())) - flen;
}

const char* firstOcc( const char* str, char tofind )
{ return str ? strchr(str,tofind) : 0; }
char* firstOcc( char* str, char tofind )
{ return str ? strchr(str,tofind) : 0; }
const char* lastOcc( const char* str, char tofind )
{ return str ? strrchr(str,tofind) : 0; }
char* lastOcc( char* str, char tofind )
{ return str ? strrchr(str,tofind) : 0; }
const char* firstOcc( const char* str, const char* tofind )
{ return str ? strstr(str,tofind?tofind:"") : 0; }
char* firstOcc( char* str, const char* tofind )
{ return str ? strstr(str,tofind?tofind:"") : 0; }
const char* lastOcc( const char* str, const char* tofind )
{ return getLastOcc( str, tofind ); }
char* lastOcc( char* str, const char* tofind )
{ return (char*)getLastOcc( str, tofind ); }


const char* getNextNonBlanks( const char* strptr, char* wordbuf )
{
    if ( !wordbuf )
	return strptr;
    *wordbuf = '\0';
    if ( !strptr || !*strptr )
	return strptr;
    mSkipBlanks( strptr );
    if ( !*strptr )
	return strptr;

    while ( *strptr && !iswspace(*strptr) )
	*wordbuf++ = *strptr++;
    *wordbuf = '\0';

    return strptr;
}


const char* getNextWord( const char* strptr, char* wordbuf )
{
    return getNextWordElem( strptr, wordbuf );
}


const char* getNextWordElem( const char* strptr, char* wordbuf )
{
    if ( !wordbuf )
	return nullptr;
    *wordbuf = '\0';
    if ( !strptr || !*strptr )
	return nullptr;
    mSkipBlanks( strptr );
    if ( !*strptr )
	return nullptr;

    bool insq = false; bool indq = false;
    while ( *strptr )
    {
	if ( *strptr == '"' )
	{
	    if ( indq )
		{ strptr++; break; }
	    if ( !insq )
		{ indq = true; strptr++; continue; }
	}
	else if ( *strptr == '\'' )
	{
	    if ( insq )
		{ strptr++; break; }
	    if ( !indq )
		{ insq = true; strptr++; continue; }
	}
	else if ( iswspace(*strptr) )
	{
	    if ( !insq && !indq )
		break;
	}

	*wordbuf++ = *strptr++;
    }

    *wordbuf = '\0';
    return strptr;
}


const char* getRankPostFix( int nr )
{
    if ( nr < 0 ) nr = -nr;

    if ( nr > 3 && nr < 21 )
	nr = 0;
    else
    {
	nr = nr % 10;
	if ( nr > 3 ) nr = 0;
    }

    const char* rets[] = { "th", "st", "nd", "rd" };
    return rets[ nr ];
}


int getIndexInStringArrCI( const char* text, const char* const * namearr,
			   int startnr, int nrchar, int notfoundidx )
{
    /* some sanity */
    if ( !text || !namearr || !*namearr ) return notfoundidx;
    mSkipBlanks(text);

    /* Look for match */
    for( int idx=startnr; namearr[idx]; idx++ )
    {
        if ( caseInsensitiveEqual( text, namearr[idx], nrchar ) )
            return idx;
    }

    /* No match found */
    return notfoundidx;
}


int getIndexInStringArrCI( const char* text, const BufferStringSet& nameset,
			   int startnr, int nrchar, int notfoundidx )
{
    /* some sanity */
    if ( !text || nameset.isEmpty() ) return notfoundidx;
    mSkipBlanks(text);

    /* Look for match */
    for( int idx=startnr; idx<nameset.size(); idx++ )
    {
        if ( caseInsensitiveEqual( text, nameset.get(idx).buf(), nrchar ) )
            return idx;
    }

    /* No match found */
    return notfoundidx;
}


const char* getLimitedDisplayString( const char* inp, int nrchars,
				     bool trimright )
{
    return getLimitedDisplayString( inp, nrchars, trimright, "..." );
}


const char* getLimitedDisplayString( const char* inp, int nrchars,
				     bool trimright, const char* padbuf )
{
    const StringView inpstr( inp );
    if ( nrchars < 1 || inpstr.isEmpty() )
	return nullptr;

    const int inplen = inpstr.size();
    if ( inplen < nrchars )
	return inp;

    BufferString tmpstr( nrchars+1, false );
    char* ptr = tmpstr.getCStr();

    const StringView padstr( padbuf );
    const int padsz = padstr.size();
    if ( !trimright && padsz > 0 )
    {
	OD::sysMemCopy( ptr, padbuf, padsz );
	ptr += padsz;
    }

    inp += inplen - nrchars + padsz;
    OD::sysMemCopy( ptr, inp, nrchars-padsz );
    ptr += nrchars-padsz;

    if ( trimright && padsz > 0 )
    {
	OD::sysMemCopy( ptr, padbuf, padsz );
	ptr += padsz;
    }

    *ptr = '\0';

    mDeclStaticString( retstr );
    retstr = tmpstr.buf();
    return retstr.buf();
}


const char* getAreaString( float area, bool parensonunit, char* str, int sz )
{
    return getAreaString( area, SI().xyInFeet(), 0, parensonunit, str, sz );
}


const char* getAreaString( float area, bool xyinfeet, int nrdecimals,
			   bool parensonunit, char* str, int sz )
{
    StringView unit;
    double val = area;
    if ( area > 10. )
    {
	if ( xyinfeet )
	{
	    val *= mToSqMileFactorD;
	    unit =  "sq mi";
	}
	else
	{
	    val *= 1e-6;
	    unit = "sq km";
	}
    }
    else
    {
	if ( xyinfeet )
	    unit =  "sq ft";
	else
	{
	    unit = "sq m";
	}
    }

    BufferString valstr;
    if ( mIsUdf(nrdecimals) )
	nrdecimals = 0;

    valstr.set( val,  0, 'f', nrdecimals );
    valstr.add( " " );
    if ( parensonunit )
	valstr.add( "(" );
    valstr.add( unit );
    if ( parensonunit )
	valstr.add( ")" );

    mDeclStaticString( retstr );
    if ( !str && retstr.bufSize() < 128 )
	retstr.setMinBufSize( 128 );
    char* ret = str ? str : retstr.getCStr();
#ifdef __win__
    const int bufsz = str ? sz : retstr.bufSize();
    strcpy_s( ret, bufsz, valstr.buf() );
#else
    strcpy( ret, valstr.buf() );
#endif

    return ret;
}


template <class T>
static const char* toStringImpl( T val, BufferString& retstr,
				 const char* cformat, int minbufsz,
				 char* extstr )
{
    if ( !extstr && retstr.bufSize() < minbufsz )
	retstr.setMinBufSize( minbufsz );

    const int bufsz = extstr ? minbufsz : retstr.bufSize();
    char* ret = extstr ? extstr : retstr.getCStr();
    od_sprintf( ret, bufsz, cformat, val );
    return ret;
}


template <class T>
static const char* toStringLimImpl( T val, int maxtxtwdth )
{
    mDeclStaticString( ret );
    if ( ret.bufSize() < 128 )
	ret.setMinBufSize( 128 );

    ret = toStringPrecise(val);
    const int simpsz = ret.size();
    if ( maxtxtwdth < 1 || simpsz <= maxtxtwdth )
	return ret.buf();

    if ( mIsUdf(val) || mIsUdf(-val) )
	return sKey::FloatUdf();

    char* str = ret.getCStr();
    // First try to simply remove digits from mantissa
    BufferString numbstr( ret );
    char* ptrdot = numbstr.find( '.' );
    if ( ptrdot )
    {
	char* ptrend = firstOcc( ptrdot, 'e' );
	if ( !ptrend )
	    ptrend = ptrdot + StringView(ptrdot).size();

	const int nrcharsav = int(ptrend - ptrdot);
	if ( nrcharsav >= simpsz - maxtxtwdth )
	{
	    for ( int irm=simpsz - maxtxtwdth; irm>0; irm--)
		rmSingleCharFromString( --ptrend );

	    if ( ptrend == ptrdot+1 )
		rmSingleCharFromString( --ptrend );

	    if ( numbstr[maxtxtwdth-1] == '.' )
		numbstr[maxtxtwdth-1] = '\0';

	    ret = numbstr;
	    return ret.buf();
	}
    }

    // Nope. We have no choice: use the 'g' format
    const BufferString fullfmt( "%", maxtxtwdth-4, "g" );
    od_sprintf( str, ret.bufSize(), fullfmt.buf(), val );

    const int retsz = ret.size();
    if ( retsz > maxtxtwdth )
    {
	// Damn. Cut off no matter how necessary it is ...
	char* eptr = firstOcc( str, 'e' );
	if ( !eptr ) firstOcc( str, 'E' );
	if ( !eptr )
	    str[maxtxtwdth-1] = '\0';
	else
	{
	    const int diff = retsz - maxtxtwdth;
	    while ( true )
		{ *(eptr-diff) = *eptr; if ( !*eptr) break; eptr++; }
	}
    }

    finalCleanupNumberString( str );
    return str;
}


// toString functions.
const char* toString( signed char c, od_uint16 width, int minbufsz,
		      char* extstr )
{
    mDeclStaticString( retstr );
    const BufferString format = cformat( 'c', width );
    return toStringImpl( c, retstr, format.str(), minbufsz, extstr );
}


const char* toString( unsigned char c, od_uint16 width, int minbufsz,
		      char* extstr )
{
    mDeclStaticString( retstr );
    const BufferString format = cformat( 'c', width );
    return toStringImpl( c, retstr, format.str(), minbufsz, extstr );
}


const char* toString( short i, od_uint16 width, int minbufsz, char* extstr )
{
    mDeclStaticString( retstr );
    const BufferString format = cformat( 'd', width );
    return toStringImpl( i, retstr, format.str(), minbufsz, extstr );
}


const char* toString( unsigned short i, od_uint16 width, int minbufsz,
		      char* extstr )
{
    mDeclStaticString( retstr );
    const BufferString format = cformat( 'u', width );
    return toStringImpl( i, retstr, format.str(), minbufsz, extstr );
}


const char* toString( od_int32 i, od_uint16 width, int minbufsz, char* extstr )
{
    mDeclStaticString( retstr );
    const BufferString format = cformat( 'd', width );
    return toStringImpl( i, retstr, format.str(), minbufsz, extstr );
}


const char* toString( od_uint32 i, od_uint16 width, int minbufsz, char* extstr )
{
    mDeclStaticString( retstr );
    const BufferString format = cformat( 'u', width );
    return toStringImpl( i, retstr, format.str(), minbufsz, extstr );
}


const char* toString( od_int64 i, od_uint16 width, int minbufsz, char* extstr )
{
    mDeclStaticString( retstr );
    const BufferString format = cformat( 'd', width, mUdf(od_uint16),
					 nullptr, "ll" );
    return toStringImpl( i, retstr, format.str(), minbufsz, extstr );
}


const char* toString( od_uint64 i, od_uint16 width, int minbufsz, char* extstr )
{
    mDeclStaticString( retstr );
    const BufferString format = cformat( 'u', width, mUdf(od_uint16),
					 nullptr, "ll" );
    return toStringImpl( i, retstr, format.str(), minbufsz, extstr );
}


const char* toString( float f, od_uint16 width, char specifier,
		      od_uint16 precision,
		      const char* length, const char* flags,
		      int minbufsz, char* extstr )
{
    mDeclStaticString( retstr );
    const BufferString format = cformat( specifier, width, precision,
					 length, flags );
    return toStringImpl( f, retstr, format.buf(), minbufsz, extstr );
}


const char* toString( double d, od_uint16 width, char specifier,
		      od_uint16 precision,
		      const char* length, const char* flags,
		      int minbufsz, char* extstr )
{
    mDeclStaticString( retstr );
    const BufferString format = cformat( specifier, width, precision,
					 length, flags );
    return toStringImpl( d, retstr, format.buf(), minbufsz, extstr );
}


const char* toStringPrecise( float f )
{ return toString( f, 0, 'g', 6 ); }

const char* toStringDec( float f, int nrdec )
{
    const float absval = Math::Abs( f );
    const bool needgeneric = absval < 1e-4f || absval >= 1e6f;
    const char specifier = needgeneric ? 'g' : 'f';
    const int precision = needgeneric ? nrdec <=0 ? 1 : nrdec+1 : nrdec;
    return toString( f, 0, specifier, precision );
}

const char* toStringSpec( float f, char specifier, int precision )
{ return toString( f, 0, specifier, precision ); }

const char* toStringCFmt( float f, const char* cformat, int minbufsz,
			  char* extstr )
{
    mDeclStaticString( retstr );
    return toStringImpl( f, retstr, cformat, minbufsz, extstr );
}

const char* toStringLim( float f, int maxtxtwidth )
{ return toStringLimImpl( f, maxtxtwidth ); }


const char* toStringPrecise( double d )
{ return toString( d, 0, 'g', 15 ); }

const char* toStringDec( double d, int nrdec )
{
    const float absval = Math::Abs( d );
    const bool needgeneric = absval < 1e-4f || absval >= 1e6f;
    const char specifier = needgeneric ? 'g' : 'f';
    const int precision = needgeneric ? nrdec <=0 ? 1 : nrdec+1 : nrdec;
    return toString( d, 0, specifier, precision );
}

const char* toStringSpec( double d, char specifier, int precision )
{ return toString( d, 0, specifier, precision ); }

const char* toStringCFmt( double d, const char* cformat, int minbufsz,
		      char* extstr )
{
    mDeclStaticString( retstr );
    return toStringImpl( d, retstr, cformat, minbufsz, extstr );
}

const char* toStringLim( double d, int maxtxtwidth )
{ return toStringLimImpl( d, maxtxtwidth ); }


const char* toHexString( od_uint32 i, bool padded )
{
    mDeclStaticString( retstr );
    if ( padded )
	retstr.set( "0x" );
    else
	retstr.setEmpty();

    std::stringstream stream;
    stream << std::hex << i;
    const std::string hexstr( stream.str() );
    if ( padded )
    {
	const int len = hexstr.size();
	for ( int idx=0; idx<8-len; idx++ )
	    retstr.add( "0" );
    }

    retstr.add( hexstr.c_str() );
    return retstr.buf();
}


const char* toString( bool b )
{
    const char* res = getYesNoString(b);
    return res;
}


const char* toString( const char* str )
{ return str ? str : ""; }

const char* toString( const OD::String& ods )
{ return ods.buf(); }

const char* toString( const CompoundKey& key )
{ return key.buf(); }

const char* toString( const MultiID& key )
{
    mDeclStaticString( retstr );
    retstr = key.toString();
    return retstr.buf();
}


static float_complex float_complexFromString( const char* str,
					char** pendptr )
{
    float_complex ret = float_complex(0.f,0.f);
    char* workendptr; if ( !pendptr ) pendptr = &workendptr;
    if ( !str || !*str )
	{ if ( str ) pendptr = (char**)(&str); return ret; }

    mSkipBlanks( str );
    pendptr = (char**)(&str);
    BufferString fcstr;
    char buf[1024+1]; int bufidx = 0;
    while ( **pendptr && !iswspace(**pendptr) )
    {
	buf[bufidx] = **pendptr;
	(*pendptr)++;
	if ( ++bufidx == 1024 )
	    { buf[bufidx] = '\0'; fcstr.add( buf ); bufidx = 0; }
    }
    if ( bufidx )
	{ buf[bufidx] = '\0'; fcstr.add( buf ); }

    const char* ptrfcstr = fcstr.buf();
    if ( !*ptrfcstr )
	return ret;

    Coord c; c.fromString( ptrfcstr );
    ret = float_complex( (float)c.x_, (float)c.y_ );

    return ret;
}


const char* toString( float_complex c )
{
    mDeclStaticString( ret );
    if ( mIsUdf(c) )
	ret.set( "<undef>" );
    else
	ret.set( "(" ).add( c.real() ).add( "," ).add( c.imag() ).add( ")" );
    return ret.buf();
}


mConvDefFromStrToSimpleType( float_complex, float_complexFromString(s,&endptr) )

namespace Conv
{
template <> void set( const char*& _to, const float_complex& c )
{
    _to = toString(c);
}

template <> void set( bool& _to, const float_complex& c )
{
    _to = !(mIsZero(c.real(),mDefEpsD) && mIsZero(c.imag(),mDefEpsD));
}
} // namespace Conv


bool getFromString( bool& b, const char* s )
{
    if ( s )
    {
	b = ( yesNoFromString( s ) ? true : false );
	return true;
    }

    b = false;
    return false;
}


#define mImplGetFromStrFunc( type, func ) \
bool getFromString( type& i, const char* s, type undef ) \
{ \
    if ( s && *s ) \
    { \
	char* e; \
	i = (type)func; \
	if ( e==s ) \
	{ \
	    i = undef; \
	    return false;\
	}\
	return true; \
    } \
 \
    i = undef; \
    return false; \
}


mImplGetFromStrFunc(int, strtol(s,&e,10) )
mImplGetFromStrFunc(od_int64, strtoll(s,&e,10) )
mImplGetFromStrFunc(float, strtod(s,&e) )
mImplGetFromStrFunc(double, strtod(s,&e) )

#undef mImplGetFromStrFunc

static const od_int64 kbtobytes = 1024;
static const od_int64 mbtobytes = 1024*kbtobytes;
static const od_int64 gbtobytes = 1024*mbtobytes;
static const od_int64 tbtobytes = 1024*gbtobytes;
static const od_int64 pbtobytes = 1024*tbtobytes;

NrBytesToStringCreator::NrBytesToStringCreator()
    : unit_( Bytes )
{
}


NrBytesToStringCreator::NrBytesToStringCreator( od_uint64 nrbytes )
    : unit_( Bytes )
{
    setUnitFrom( nrbytes );
}


void NrBytesToStringCreator::setUnitFrom( od_uint64 number, bool max )
{
    int nrshifts = 0;
    for ( ; nrshifts<4 && number>=1024; nrshifts++ )
	number >>= 10;

    const Unit newunit = (Unit) nrshifts;
    if ( max )
	unit_ = mMAX( newunit, unit_ );
    else
	unit_ = newunit;
}


StringView NrBytesToStringCreator::getString( od_uint64 sz, int nrdecimals,
					     bool withunit ) const
{
    if ( nrdecimals>5 ) nrdecimals = 5;
    if ( nrdecimals<0 ) nrdecimals = 0;

    //Deliberatily make 10 times larger, so that rounding off will work
    od_uint64 nrdecfactor = 10;
    for ( int idx=0; idx<nrdecimals; idx++ )
	nrdecfactor *= 10;

    sz *= nrdecfactor;
    unsigned char nrshifts = (unsigned char) unit_;
    for ( int idx=0; idx<nrshifts; idx++ )
	sz >>= 10;

    float fsz = (float) sz;
    fsz /= nrdecfactor;

    BufferString formatstr = "%.";
    formatstr.add( nrdecimals );
    formatstr.add( "f");

    mDeclStaticString( ret );
    if ( ret.isEmpty() )
	ret.setMinBufSize( 16 );
    od_sprintf( ret.getCStr(), ret.bufSize(), formatstr, fsz );

    if ( withunit )
	ret.add( " " ).add( getUnitString() );

    return StringView( ret.str() );
}



StringView NrBytesToStringCreator::getUnitString() const
{
    return toString( unit_ );
}


StringView NrBytesToStringCreator::toString(NrBytesToStringCreator::Unit unit )
{
    const char* units[] = { "bytes", "kB", "MB", "GB", "TB", "PB", nullptr };
    return units[ int(unit) ];
}


od_int64 convertToBytes( double fsz, File::SizeUnit inpunit )
{
    od_int64 ret = static_cast<od_int64>(fsz);
    if ( inpunit == File::SizeUnit::KB )
       return ret * kbtobytes;
    else if ( inpunit == File::SizeUnit::MB )
       return ret * mbtobytes;
    else if ( inpunit == File::SizeUnit::GB )
       return ret * gbtobytes;
    else if ( inpunit == File::SizeUnit::TB )
       return ret * tbtobytes;
    else if ( inpunit == File::SizeUnit::PB )
       return ret * pbtobytes;

    return ret;
}


double convertFromBytes( od_int64 fsz, File::SizeUnit opunit )
{
    double ret = static_cast<double>(fsz);
    if ( opunit == File::SizeUnit::KB )
	return ret/kbtobytes;
    else if ( opunit == File::SizeUnit::MB )
	return ret/mbtobytes;
    else if ( opunit == File::SizeUnit::GB )
	return ret/gbtobytes;
    else if ( opunit == File::SizeUnit::TB )
	return ret/tbtobytes;
    else if ( opunit == File::SizeUnit::PB )
	return ret/pbtobytes;

    return ret;
}


char* truncateString( char* str, int maxlen )
{
    if ( maxlen>4 && strlen(str)>maxlen-4)
    {
	memcpy( str+maxlen-4, " ...", 5 );
    }

    return str;
}


const char* cformat( char specifier, od_uint16 width, od_uint16 precision,
		     const char* length, const char* flags )
{
// %[flags][width][.precision][length]specifier
    BufferString tmpstr;
    tmpstr.set( '%' );
    if ( flags )	tmpstr.add( flags );
    if ( width>0 )	tmpstr.add( width );
    if ( !mIsUdf(precision) )
    {
	tmpstr.add( "." );
	if ( precision>0 )
	    tmpstr.add( precision );
    }

    if ( length )
	tmpstr.add( length );

    tmpstr.add( specifier );

    mDeclStaticString( ret );
    ret = tmpstr;
    return ret.buf();
}


BufferString toUserString( const Interval<int>& intv )
{
    BufferString ret;
    ret = intv.start_;
    ret.add( " - " ).add( intv.stop_ );
    if ( intv.hasStep() )
    {
	mDynamicCastGet(const StepInterval<int>*,sintv,&intv);
	if ( sintv )
	    ret.add( " [" ).add( sintv->step_ ).add("]");
    }

    return ret;
}


static BufferString toUserString( const Interval<float>& intv, char specifier,
				  int precision )
{
    BufferString ret;
    ret = toStringSpec( intv.start_, specifier, precision );
    ret.add( " - " ).add( toStringSpec( intv.stop_, specifier, precision ) );
    if ( intv.hasStep() )
    {
	mDynamicCastGet(const StepInterval<float>*,sintv,&intv);
	if ( sintv )
	    ret.add( " [" )
	       .add( toStringSpec( sintv->step_, specifier, precision ) )
	       .add( "]" );
    }

    return ret;
}

BufferString toUserString( const Interval<float>& intv, int precision )
{
    return toUserString( intv, 'g', precision );
}


BufferString toUserStringF( const Interval<float>& intv, int nrdec )
{
    return toUserString( intv, 'f', nrdec );
}


BufferString toUserString( const Interval<double>& intv, int precision )
{
    BufferString ret;
    ret = toStringSpec( intv.start_, 'g', precision );
    ret.add( " - " ).add( toStringSpec( intv.stop_, 'g', precision ) );
    if ( intv.hasStep() )
    {
	mDynamicCastGet(const StepInterval<double>*,sintv,&intv);
	if ( sintv )
	    ret.add( " [" )
	       .add( toStringSpec( sintv->step_, 'g', precision ) )
	       .add( "]" );
    }

    return ret;
}


const char* getDimensionString( int sz1, int sz2, int sz3 )
{
    mDeclStaticString( ret );
    ret = sz1;
    ret.add( " x " ).add( sz2 );
    if ( sz3 > 0 )
	ret.add( " x " ).add( sz3 );

    return ret;
}


// Deprecated implementations:

const char* toString( float f, int nrdec )
{
    return toString( f, 0, 'f', nrdec );
}


const char* toString( double d, int nrdec )
{
    return toString( d, 0, 'f', nrdec );
}


const char* toString( float f, char format, int precision )
{
    return toStringSpec( f, format, precision );
}


const char* toString( double d, char format, int precision )
{
    return toStringSpec( d, format, precision );
}
