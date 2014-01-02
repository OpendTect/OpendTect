/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 11-4-1994
 * FUNCTION : Functions for string manipulations
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "nrbytes2string.h"
#include "staticstring.h"
#include "survinfo.h"
#include "undefval.h"
#include "odcomplex.h"
#include "keystrs.h"
#include <stdio.h>
#include <string.h>
#include <iostream>

#ifdef __win__
# define sDirSep        "\\"
#else
# define sDirSep        "/"
#endif



static const char* getStringFromInt( od_int32 val, char* str )

{
    mDeclStaticString( retstr );
    char* ret = str ? str : retstr.getCStr();
    sprintf( ret, "%d", val );
    return ret;
}


static const char* getStringFromUInt( od_uint32 val, char* str )
{
    mDeclStaticString( retstr );
    char* ret = str ? str : retstr.getCStr();
    sprintf( ret, "%du", val );
    return ret;
}


/* Made the mkUIntStr function because %lld doesn't work on Windows */

static void mkUIntStr( char* buf, od_uint64 val, int isneg )
{
    if ( !val )
    {
	buf[0] = '0';
	buf[1] = 0;
	return;
    }

    /* Fill the string with least significant first, i.e. reversed: */
    char* pbuf = buf;
    while ( val )
    {
	int restval = val % 10;
	val /= 10;
	*pbuf++ = '0' + (char) restval;
    }
    if ( isneg ) *pbuf++ = '-';
    *pbuf = '\0';

    /* Reverse to normal: */
    pbuf--;
    char* pbuf2 = buf;
    while ( pbuf > pbuf2 )
    {
	char tmp = *pbuf; *pbuf = *pbuf2; *pbuf2 = tmp;
	pbuf--; pbuf2++;
    }
}


static const char* getStringFromInt64( od_int64 val, char* str )
{
    mDeclStaticString( retstr );
    char* ret = str ? str : retstr.getCStr();
    const bool isneg = val < 0 ? 1 : 0;
    if ( isneg ) val = -val;
    mkUIntStr( ret, (od_uint64)val, isneg );
    return ret;
}


static const char* getStringFromUInt64( od_uint64 val, char* str )
{
    mDeclStaticString( retstr );
    char* ret = str ? str : retstr.getCStr();
    mkUIntStr( ret, val, 0 );
    return ret;
}


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
	ptrend = ptrdot + FixedString(ptrdot).size() - 1;

    while ( ptrend > ptrdot && *ptrend == '0' )
    {
	rmSingleCharFromString( ptrend );
	ptrend--;
    }

    if ( *(ptrdot+1) == '\0' || *(ptrdot+1) == 'e' )
	rmSingleCharFromString( ptrdot );
}


static int findUglyRoundOff( char* str )
{
    char* ptrdot = firstOcc( str, '.' );
    if ( !ptrdot )
	return -1;

    char* ptrend = firstOcc( ptrdot, 'e' );
    if ( !ptrend )
    {
	ptrend = firstOcc( ptrdot, 'E' );
	if ( !ptrend )
	    ptrend = ptrdot + FixedString(ptrdot).size();
    }

    const int matchstrsz = 3;
    if ( ptrend - ptrdot < matchstrsz )
	return -1;

    char* ptr = ptrdot + 1;
    while ( *ptr == '0' )
	ptr++;
    if ( !*ptr || ptrend - ptr < matchstrsz )
	return -1;

    ptr++;
    char* hit = firstOcc( ptr, "000" );
    if ( !hit )
    {
	hit = firstOcc( ptr, "999" );
	if ( !hit )
	    return -1;
    }

    int nrdec = mCast( int, hit - ptrdot );
    if ( *hit == '9' ) nrdec--;
    if ( nrdec < 0 ) nrdec = 0;
    return nrdec;
}


#define mSetStrTo0(str,ret) { *str = '0'; *(str+1) = '\0'; ret; }

static void finalCleanupNumberString( char* str )
{
    // We don't need any '+'
    char* ptr = firstOcc( str, '+' );
    while ( ptr )
    {
	rmSingleCharFromString( ptr );
	ptr = firstOcc( str, '+' );
    }

    if ( !*str )
	mSetStrTo0(str,return)

    char* ptrdot = firstOcc( str, '.' );
    if ( !ptrdot ) return;

    char* ptrexp = firstOcc( str, 'e' );
    if ( !ptrexp )
    {
	ptrexp = firstOcc( str, 'E' );
	if ( ptrexp )
	    *ptrexp = 'e';
    }
    if ( ptrexp == str )
	mSetStrTo0(str,return)

    cleanupMantissa( ptrdot, ptrexp );
    if ( !*str )
	mSetStrTo0(str,return)

    char* ptrend = firstOcc( str, 'e' );
    if ( !ptrend )
	ptrend = str + FixedString(str).size() - 1;
    if ( ptrexp )
    {
	char* ptrlast = ptrexp-1;
	while ( *ptrlast == '0' )
	    rmSingleCharFromString( ptrlast );
    }

    if ( !*str )
	mSetStrTo0(str,return)
}


template <class T>
static const char* getStringFromFPNumber( T inpval )
{
    mDeclStaticString( retstr );
    char* str = retstr.getCStr();

    if ( !inpval )
	mSetStrTo0(str,return str)

    const bool isneg = inpval < 0;
    const float val = isneg ? -inpval : inpval;
    if ( mIsUdf(val) )
	return sKey::FloatUdf();


    const char* fmtend = val < (T)0.001 || val >= (T)1e8 ? "g" : "f";
    const BufferString fmt( "%.8", fmtend );

    if ( isneg ) *str = '-';
    sprintf( isneg ? str+1 : str, fmt.buf(), val );

    const int nrdec = findUglyRoundOff( str );
    if ( nrdec >= 0 )
    {
	const BufferString newfmt( "%.", nrdec, fmtend );
	sprintf( isneg ? str+1 : str, newfmt.buf(), val );
    }

    finalCleanupNumberString( str );

    return str;
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
    while( isspace(*endptr) )
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


bool yesNoFromString( const char* str )
{
    if ( !str ) return false;
    mSkipBlanks( str );
    return *str == 'Y' || *str == 'y' || *str == 'T' || *str == 't';
}


bool isNumberString( const char* str, bool int_only )
{
    if ( !str || !*str )
	return false;
    bool curisdigit = isdigit(*str);
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
    curisdigit = isdigit(*str);
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
	if ( !isdigit(*str) )
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

    return true;
}


bool isAlphaNumString( const char* str, bool allowspace )
{
    if ( !str || !*str )
	return false;

#define mCheckChar (isalnum(*str) || (allowspace && *str==' '))
    while ( *str )
    {
	if ( !mCheckChar )
	    return false;

	str++;
    }
#undef mCheckChar

    return true;
}


void cleanupString( char* str, bool spaceallow, bool fsepallow, bool dotallow )
{
    if ( !str )
	return;

    while ( *str )
    {
	if ( !isalnum(*str) )
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

    while ( true )
    {
	if ( ci )
	    { if ( tolower(*str1) != tolower(*str2) ) return false; }
	else
	    { if ( *str1 != *str2 ) return false; }
	str1-- ; str2--;
	if ( str1 == start1 )
	    break;
	if ( str2 == start2 )
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


const char* getNextWord( const char* strptr, char* wordbuf )
{
    if ( !wordbuf ) return 0;
    *wordbuf = '\0';
    if ( !strptr || !*strptr ) return strptr;

    mSkipBlanks( strptr );
    while ( *strptr && !isspace(*strptr) )
	*wordbuf++ = *strptr++;
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


const char* getLimitedDisplayString( const char* inp, int nrchars,
				     bool trimright )
{
    if ( nrchars < 1 || !inp || !*inp ) return "";
    const int inplen = strlen( inp );
    if ( inplen < nrchars )
	return inp;

    mDefineStaticLocalObject( Threads::Lock, lock, (true) );
    Threads::Locker locker ( lock );

    mDefineStaticLocalObject( char*, ret, = 0 );
    delete [] ret; ret = new char [nrchars+1];
    char* ptr = ret;

    const char* dots = "...";
    if ( !trimright )
    {
	inp += inplen - nrchars + 3;
	strcpy( ret,  dots );
	ptr = ret + 3;
    }

    for( int idx=0; idx<nrchars-3; idx++ )
	*ptr++ = *inp++;
    *ptr = '\0';

    if ( trimright )
	strcat( ret,  dots );

    return ret;
}


const char* getAreaString( float m2, bool parensonunit, char* str )
{
    BufferString val;

    const float km2 = m2* float(1e-6);

    FixedString unit;

    if ( km2>0.01 )
    {
	if ( SI().xyInFeet() )
	{
	    val = km2*mToSqMileFactor;
	    unit =  "sq mi";
	}
	else
	{
	    val = km2;
	    unit = "sq km";
	}
    }
    else
    {
	if ( SI().xyInFeet() )
	{
	    val = m2*mToFeetFactorF*mToFeetFactorF;
	    unit =  "sq ft";
	}
	else
	{
	    val = m2;
	    unit = "sq m";
	}
    }

    val += " ";
    if ( parensonunit )
	val += "(";
    val += unit;
    if ( parensonunit )
	val += ")";

    mDeclStaticString( retstr );
    char* ret = str ? str : retstr.getCStr();
    strcpy( ret, val.buf() );

    return ret;
}


// toString functions.
const char* toString( od_int32 i )
{ return getStringFromInt( i, 0 ); }

const char* toString( od_uint32 i )
{ return getStringFromUInt( i, 0 ); }

const char* toString( od_int64 i )
{ return getStringFromInt64( i, 0 ); }

const char* toString( od_uint64 i )
{ return getStringFromUInt64(i, 0); }

const char* toString( float f )
{ return getStringFromFPNumber( f ); }

const char* toString( double d )
{ return getStringFromFPNumber( d ); }

const char* toString( short i )
{ return getStringFromInt((int)i, 0); }

const char* toString( unsigned short i )
{ return getStringFromUInt( (unsigned int)i, 0 ); }

const char* toString( unsigned char c )
{ return toString( ((unsigned short)c) ); }

const char* toString( const OD::String& ods )
{ return ods.buf(); }

template <class T>
static const char* toStringLimImpl( T val, int maxtxtwdth )
{
    FixedString simptostr = toString(val);
    if ( maxtxtwdth < 1 || simptostr.size() <= maxtxtwdth )
	return simptostr;

    mDeclStaticString( ret );
    char* str = ret.getCStr();
    if ( mIsUdf(val) )
	ret.set( "1e30" );
    else
    {
	const BufferString fullfmt( "%", maxtxtwdth-5, "g" );
	sprintf( str, fullfmt.buf(), val );
    }

    const int retsz = ret.size();
    if ( retsz > maxtxtwdth )
    {
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

const char* toString( double d, int mw )
{ return toStringLimImpl( d, mw ); }

const char* toString( float f, int mw )
{ return toStringLimImpl( f, mw ); }


const char* toString( const char* str )
{
    return str ? str : "";
}


const char* toString( signed char c )
{
    mDeclStaticString( retstr );
    char* buf = retstr.getCStr();
    buf[0] = (char)c; buf[1] = '\0';
    return buf;
}


const char* toString( bool b )
{
    const char* res = getYesNoString(b);
    return res;
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
    while ( **pendptr && !isspace(**pendptr) )
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
    ret = float_complex( (float)c.x, (float)c.y );

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
mImplGetFromStrFunc(float, strtod(s,&e) )
mImplGetFromStrFunc(double, strtod(s,&e) )

#undef mImplGetFromStrFunc



NrBytesToStringCreator::NrBytesToStringCreator()
    : unit_( Bytes )
{
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


FixedString NrBytesToStringCreator::getString( od_uint64 sz, int nrdecimals,
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
    sprintf( ret.getCStr(), formatstr, fsz );

    if ( withunit )
	ret.add( " " ).add( getUnitString() );

    return FixedString( ret.str() );
}



FixedString NrBytesToStringCreator::getUnitString() const
{
    return toString( unit_ );
}


FixedString NrBytesToStringCreator::toString(NrBytesToStringCreator::Unit unit)
{
    const char* units[] = { "bytes", "kB", "MB", "GB", "TB", "PB", 0 };
    return units[(int) unit];
}
