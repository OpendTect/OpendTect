/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 11-4-1994
 * FUNCTION : Functions for string manipulations
-*/


#include "string2.h"

#include "keystrs.h"
#include "nrbytes2string.h"
#include "odcomplex.h"
#include "staticstring.h"
#include "survinfo.h"
#include "undefval.h"

#ifndef OD_NO_QT
# include <QString>
#else
# include <stdlib.h>
# ifdef __win__
#  define strtof strtod
#  define strtoll _strtoi64
#  define strtoull _strtoui64
# endif
#endif

#ifdef __win__
# define sDirSep        "\\"
#else
# define sDirSep        "/"
#endif

#define mToSqMileFactor	0.3861 // km^2 to mile^2


const char* getYesNoString( bool yn )
{
    const char* strs[] = { "Yes", "No" };
    return yn ? strs[0] : strs[1];
}


void removeTrailingBlanks( char* str )
{
    if ( !str || ! *str ) return;

    char* endptr = str + strLength(str) - 1;
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


const char* getVelUnitString( bool isfeet, bool wb )
{
    if ( isfeet )
	return wb ? "(ft/s)" : "ft/s";

    return wb ? "(m/s)" : "m/s";
}


bool yesNoFromString( const char* str )
{
    if ( !str )
	return false;
    mSkipBlanks( str );

    return isdigit(*str) ? *str != '0'
	 : *str=='Y' || *str=='y' || *str=='T' || *str=='t' || *str=='+';
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

    return true;
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

    const int slen = strLength( str );
    const int flen = strLength( tofind );
    if ( flen == 0 )
	return str + slen;
    else if ( flen == 1 )
	return lastOcc( str, *tofind );

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
{ return firstOcc(str,(int)tofind); }
char* firstOcc( char* str, char tofind )
{ return firstOcc(str,(int)tofind); }
const char* lastOcc( const char* str, char tofind )
{ return lastOcc(str,(int)tofind); }
char* lastOcc( char* str, char tofind )
{ return lastOcc(str,(int)tofind); }
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
    while ( *strptr && !iswspace(*strptr) )
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
    const int inplen = strLength( inp );
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


int strLength( const char* str )
{
#ifdef mMEM_DEBUGGER
    if ( !str )
	return 0;

    const char* origstr = str;
    while ( *str )
	str++;

    return str-origstr;
#else
    return strlen( str );
#endif
}


const char* firstOcc( const char* str, int character )
{
#ifdef mMEM_DEBUGGER
    if ( !str )
	return 0;

    const char c = character;
    while( *str != '\0' && *str != c )
	str++;

    return str;
#else
    return strchr(str,character);
#endif
}


char* firstOcc( char* str, int character )
{
    return const_cast<char*>( firstOcc(const_cast<const char*>(str),character));
}


const char* lastOcc( const char* str, int character )
{
#ifdef mMEM_DEBUGGER
    if ( !str )
	return 0;

    const char c = character;
    const char* res = 0;
    while( *str != '\0' )
    {
	if ( *str == c )
	    res = str;

	str++;
    }

    if ( c == '\0' )
	res++;

    return res;
#else
    return strrchr(str,character);
#endif
}


char* lastOcc( char* str, int character )
{
    return const_cast<char*>( lastOcc(const_cast<const char*>(str),character) );
}

char* truncateString( char* str, int maxlen )
{
    if ( maxlen>4 && strlen(str)>maxlen-4)
    {
	memcpy( str+maxlen-4, " ...", 5 );
    }

    return str;
}


/* Conversion stuff.

 All simple types (even integer types) are subject to the locale - sprintf,
 sscanf, etc.  they all use the locale. Because we want our data files to be
 sharable we cannot use these functions!

 So a lot of the following code is dedicated to avoiding any call that uses
 the locale. Qt has some functions we can use, and we made our own algorithms
 to get what we want/need.

*/


/* First, the integer types. */


static void mkUIntStr( char* buf, od_uint64 val, int isneg )
{
    /* Fill the string with least significant first, i.e. reversed: */
    char* pbuf = buf;
    while ( val )
    {
	const int restval = val % 10;
	val /= 10;
	*pbuf++ = '0' + (char)restval;
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


template <class IT>
static const char* getStringFromUIntType( IT val, char* str )
{
    mDeclStaticString( retstr );
    char* ret = str ? str : retstr.getCStr();
    if ( !val )
	{ ret[0] = '0'; ret[1] = '\0'; }
    else
	mkUIntStr( ret, (od_uint64)val, false );
    return ret;
}


template <class IT>
static const char* getStringFromIntType( IT val, char* str )
{
    mDeclStaticString( retstr );
    char* ret = str ? str : retstr.getCStr();
    if ( !val )
	{ ret[0] = '0'; ret[1] = '\0'; }
    else
    {
	const bool isneg = val < 0;
	if ( isneg )
	    val = -val;
	mkUIntStr( ret, (od_uint64)val, isneg );
    }
    return ret;
}


static const char* getStringFromInt( od_int32 val, char* str )
{ return getStringFromIntType( val, str ); }
static const char* getStringFromUInt( od_uint32 val, char* str )
{ return getStringFromUIntType( val, str ); }
static const char* getStringFromInt64( od_int64 val, char* str )
{ return getStringFromIntType( val, str ); }
static const char* getStringFromUInt64( od_uint64 val, char* str )
{ return getStringFromUIntType( val, str ); }


/* Second, the FP types. */

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


static bool isZeroInt( const char* start, const char* end )
{
    while ( start != end )
    {
	if ( *start != '0' && *start != '-' && *start != '+' )
	    return false;
	else
	    start++;
    }
    return true;
}


static int findUglyRoundOff( char* str, bool isdouble )
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

    char* decstartptr = ptrdot + 1;
    if ( isZeroInt(str,ptrdot) )
    {
	while ( *decstartptr && *decstartptr == '0' )
	    decstartptr++;
	if ( !*decstartptr )
	    return -1;
    }

    char* hit = firstOcc( decstartptr, isdouble ? "0000" : "000" );
    if ( !hit )
    {
	hit = firstOcc( decstartptr, isdouble ? "9999" : "999" );
	if ( !hit )
	    return -1;
    }

    if ( hit > ptrend )
	return -1;

    int nrdec = int( hit - ptrdot );
    if ( *hit == '9' ) nrdec--;
    if ( nrdec < 0 ) nrdec = 0;
    return nrdec;
}


/* currently not used but will return
static void enforceNrDecimals( char* str, int nrdec )
{
    char* ptrdot = firstOcc( str, '.' );
    if ( !ptrdot )
	return; // huh?

    char* ptrend = firstOcc( ptrdot, 'e' );
    if ( !ptrend )
    {
	ptrend = firstOcc( ptrdot, 'E' );
	if ( !ptrend )
	    ptrend = ptrdot + FixedString(ptrdot).size();
    }

    int actualnrdec = ptrend - (ptrdot + 1);
    while ( actualnrdec > nrdec )
    {
	rmSingleCharFromString( ptrdot + 1 + nrdec );
	actualnrdec--;
    }
}
*/


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
    // We don't need any ','s either - remove them
    ptr = firstOcc( str, ',' );
    while ( ptr )
    {
	rmSingleCharFromString( ptr );
	ptr = firstOcc( str, ',' );
    }

    if ( !*str )
	mSetStrTo0( str, return )

    char* ptrexp = firstOcc( str, 'e' );
    if ( !ptrexp )
    {
	ptrexp = firstOcc( str, 'E' );
	if ( ptrexp )
	    *ptrexp = 'e'; // so we'll never have 'E', always 'e'
    }
    if ( ptrexp == str )
	mSetStrTo0( str, return )
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
	mSetStrTo0( str, return )

    // Remove trailing '0's in mantissa
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
	mSetStrTo0( str, return )
}


template <class T>
static const char* getStringFromNumber( T val, char format, int precision )
{
#ifdef OD_NO_QT
    return toString( val );
#else
    mDeclStaticString( retstr );
    retstr = QString::number( val, format, precision );
    return retstr.getCStr();
#endif
}


const char* getFPStringWithDecimals( double val, int nrdec )
{
    char fmt = 'f';
    if ( val < -1e8 || val > 1e8 || (val<1e-8 && val>-1e-8) )
	fmt = 'g';
    return getStringFromNumber( val, fmt, nrdec );
}


#define mDetermineValueProps() \
    const bool scientific = (val > (T)-0.001 && val < (T)0.001) \
			|| val < (T)(-1.e8) || val >= (T)(1.e8); \
    const char fmt = scientific ? 'g' : 'f'


template <class T>
static const char* getPreciseStringFromFPNumber( T val, bool isdouble )
{
    mDeclStaticString( retstr );
    char* str = retstr.getCStr();
    if ( !val )
	mSetStrTo0( str, return str )
    else if ( mIsUdf(val) || mIsUdf(-val) )
	return sKey::FloatUdf();

    mDetermineValueProps();
    const int prec = isdouble ? 16 : 8;
    retstr = getStringFromNumber( val, fmt, prec );
    finalCleanupNumberString( str );
    return str;
}


template <class T>
static const char* getStringFromFPNumber( T val, bool isdouble )
{
    mDeclStaticString( retstr );
    retstr = getPreciseStringFromFPNumber( val, isdouble );
    char* str = retstr.getCStr();

    const int nrdec = findUglyRoundOff( str, isdouble );
    if ( nrdec >= 0 )
    {
	mDetermineValueProps();
	retstr = getStringFromNumber( val, fmt, nrdec+1 );
    }

    finalCleanupNumberString( str );
    return str;
}


static bool removeMantissaDigits( BufferString& numbstr, int maxtxtwdth )
{
    const int orgsz = numbstr.size();
    char* ptrdot = numbstr.find( '.' );
    if ( ptrdot )
    {
	char* ptrend = firstOcc( ptrdot, 'e' );
	if ( !ptrend )
	    ptrend = ptrdot + FixedString(ptrdot).size();
	const int nrcharsav = int(ptrend - ptrdot);
	if ( nrcharsav >= orgsz - maxtxtwdth )
	{
	    for ( int irm=orgsz - maxtxtwdth; irm>0; irm--)
		rmSingleCharFromString( --ptrend );
	    if ( numbstr[maxtxtwdth-1] == '.' )
		numbstr[maxtxtwdth-1] = '\0';
	    return true;
	}
    }
    return false;
}


template <class T>
static const char* getLimStringFromFPNumber( T val, int maxtxtwdth )
{
    FixedString simptostr = toString(val);
    const int simpsz = simptostr.size();
    if ( maxtxtwdth < 1 || simpsz <= maxtxtwdth )
	return simptostr;

    if ( mIsUdf(val) || mIsUdf(-val) )
	return sKey::FloatUdf();

    mDeclStaticString( ret );
    char* str = ret.getCStr();

    // First try to simply remove digits from mantissa
    BufferString numbstr( simptostr );
    if ( removeMantissaDigits(numbstr,maxtxtwdth) )
	{ ret = numbstr; return ret.buf(); }

    // OK, we'll need the 'g' format
    numbstr = getStringFromNumber( val, 'g', 10 );
    if ( removeMantissaDigits(numbstr,maxtxtwdth) )
	{ ret = numbstr; return ret.buf(); }

    // Damn. Cut off no matter how necessary it is ...
    ret = numbstr;
    char* eptr = firstOcc( str, 'e' );
    if ( !eptr ) firstOcc( str, 'E' );
    if ( !eptr )
	str[maxtxtwdth-1] = '\0';
    else
    {
	const int diff = ret.size() - maxtxtwdth;
	while ( true )
	    { *(eptr-diff) = *eptr; if ( !*eptr) break; eptr++; }
    }

    return str;
}


const char* toString( od_int32 i )
{ return getStringFromInt( i, 0 ); }

const char* toString( od_uint32 i )
{ return getStringFromUInt( i, 0 ); }

const char* toString( od_int64 i )
{ return getStringFromInt64( i, 0 ); }

const char* toString( od_uint64 i )
{ return getStringFromUInt64( i, 0 ); }

const char* toString( short i )
{ return getStringFromInt( (int)i, 0 ); }

const char* toString( unsigned short i )
{ return getStringFromUInt( (unsigned int)i, 0 ); }

const char* toString( unsigned char c )
{ return toString( (unsigned short)c ); }

const char* toString( const OD::String& ods )
{ return ods.buf(); }

const char* toString( float f )
{ return getStringFromFPNumber( f, false ); }

const char* toString( double d )
{ return getStringFromFPNumber( d, true ); }

const char* toStringPrecise( float f )
{ return getPreciseStringFromFPNumber( f, false ); }

const char* toStringPrecise( double d )
{ return getPreciseStringFromFPNumber( d, true ); }

const char* toStringLim( double d, int mw )
{ return getLimStringFromFPNumber( d, mw ); }

const char* toStringLim( float f, int mw )
{ return getLimStringFromFPNumber( f, mw ); }

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


static double gtDoubleFromString( const char* str, double defval )
{
#ifdef OD_NO_QT
    return strtod( (char*)str, endp );
#else

    if ( !str || !*str )
	return defval;

    bool isok = false;
    const QString qstr( str );
    const double ret = qstr.toDouble( &isok );
    return isok ? ret : defval;

#endif
}


static float gtFloatFromString( const char* str, float defval )
{
#ifdef OD_NO_QT
    char* endp;
    return strtof( (char*)str, &endp );
#else

    if ( !str || !*str )
	return defval;

    bool isok = false;
    const QString qstr( str );
    const float ret = qstr.toFloat( &isok );
    return isok ? ret : defval;

#endif
}


static int gtIntFromString( const char* str, int defval )
{
#ifdef OD_NO_QT
    char* endp;
    return strtoll( (char*)str, &endp, 0 );
#else

    if ( !str || !*str )
	return defval;

    bool isok = false;
    const QString qstr( str );
    const int ret = qstr.toInt( &isok, 0 );
    return isok ? ret : defval;

#endif
}

static od_uint32 gtUIntFromString( const char* str, od_uint32 defval )
{
#ifdef OD_NO_QT
    char* endp;
    return strtoul( (char*)str, &endp, 0 );
#else

    if ( !str || !*str )
	return defval;

    bool isok = false;
    const QString qstr( str );
    const od_uint32 ret = qstr.toUInt( &isok, 0 );
    return isok ? ret : defval;

#endif
}


static od_int64 gtInt64FromString( const char* str, od_int64 defval )
{
#ifdef OD_NO_QT
    char* endp;
    return strtoll( (char*)str, &endp, 0 );
#else

    if ( !str || !*str )
	return defval;

    bool isok = false;
    const QString qstr( str );
    const od_int64 ret = qstr.toLongLong( &isok, 0 );
    return isok ? ret : defval;

#endif
}


static od_uint64 gtUInt64FromString( const char* str, od_uint64 defval )
{
#ifdef OD_NO_QT
    char* endp;
    return strtoull( (char*)str, &endp, 0 );
#else

    if ( !str || !*str )
	return defval;

    bool isok = false;
    const QString qstr( str );
    const od_uint64 ret = qstr.toULongLong( &isok, 0 );
    return isok ? ret : defval;

#endif
}


#define mConvDefFromStrToSimpleType(type,function) \
\
namespace Conv { \
\
    inline void set_charp( type& _to, const char* s ) \
    { _to = (type)function( s, mUdf(type) ); } \
    template <> mGlobal(Basic) void set<type,const char*>( \
		    type& _to, const char* const& s ) \
	{ set_charp( _to, s ); } \
    template <> mGlobal(Basic) void set( type& _to, const FixedString& s ) \
	{ set_charp( _to, s.str() ); } \
    template <> mGlobal(Basic) void set( type& _to, const BufferString& s ) \
	{ set_charp( _to, s.str() ); } \
\
}

mConvDefFromStrToSimpleType( short, gtIntFromString )
mConvDefFromStrToSimpleType( unsigned short, gtUIntFromString )
mConvDefFromStrToSimpleType( int, gtIntFromString )
mConvDefFromStrToSimpleType( unsigned int, gtUIntFromString )
mConvDefFromStrToSimpleType( od_int64, gtInt64FromString )
mConvDefFromStrToSimpleType( od_uint64, gtUInt64FromString )
mConvDefFromStrToSimpleType( float, gtFloatFromString )
mConvDefFromStrToSimpleType( double, gtDoubleFromString )


int toInt( const char* str, int defval )
{ return gtIntFromString( str, defval ); }

od_int64 toInt64( const char* str, od_int64 defval )
{ return gtInt64FromString( str, defval ); }

float toFloat( const char* str, float defval )
{ return gtFloatFromString( str, defval ); }

double toDouble( const char* str, double defval )
{ return gtDoubleFromString( str, defval ); }



static float_complex float_complexFromString( const char* str,
					      float_complex defval )
{
    if ( !str || !*str || *str != '(' )
	return defval;

    Coord c; c.fromString( str );
    return float_complex( (float)c.x_, (float)c.y_ );
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

mConvDefFromStrToSimpleType( float_complex, float_complexFromString )


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

    mDeclStaticString( ret );
    ret.set( getStringFromNumber(fsz,'f',nrdecimals) );

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
