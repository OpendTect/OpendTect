/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 11-4-1994
 * FUNCTION : Functions for string manipulations
-*/

static const char* rcsID mUnusedVar = "$Id: string2.cc,v 1.19 2012-07-10 08:05:30 cvskris Exp $";

#include "string2.h"
#include "staticstring.h"
#include "fixedstring.h"
#include "survinfo.h"
#include "undefval.h"
#include <stdio.h>

#ifdef __win__
# define sDirSep        "\\"
#else
# define sDirSep        "/"
#endif


extern "C" void C_removeTrailingBlanks(char*);
extern "C" int C_caseInsensitiveEqual(const char*,const char*,int);
extern "C" void C_replaceCharacter(char*,char,char);

extern "C" void C_removeTrailingBlanks( char* str )
{ removeTrailingBlanks( str ); }
extern "C" int C_caseInsensitiveEqual( const char* s1, const char* s2, int n )
{ return caseInsensitiveEqual( s1, s2, n ); }
extern "C" void C_replaceCharacter( char* s, char from, char to )
{ replaceCharacter( s, from, to ); }


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


const char* getStringFromInt( od_int32 val, char* str )

{
    static StaticStringManager stm;
    char* ret = str ? str : stm.getString().buf();
    sprintf( ret, "%d", val );
    return ret;
}


const char* quoteString( const char* initial, char quote )
{
    static StaticStringManager stm;
    BufferString& str = stm.getString();

    str = initial;

    if ( !str.size() )
	return str.str();

    char insertstr[] = { quote, 0 };

    if ( str[0]!=quote )
	str.insertAt( 0, insertstr );

    if ( str[str.size()-1]!='`' )
	str += insertstr;

    return str.str();
}


const char* getStringFromUInt( od_uint32 val, char* str )
{
    static StaticStringManager stm;
    char* ret = str ? str : stm.getString().buf();
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
	*pbuf++ = '0' + restval;
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


const char* getStringFromInt64( od_int64 val, char* str )
{
    static StaticStringManager stm;
    char* ret = str ? str : stm.getString().buf();
    const bool isneg = val < 0 ? 1 : 0;
    if ( isneg ) val = -val;
    mkUIntStr( ret, (od_uint64)val, isneg );
    return ret;
}


const char* getStringFromUInt64( od_uint64 val, char* str )
{
    static StaticStringManager stm;
    char* ret = str ? str : stm.getString().buf();
    mkUIntStr( ret, val, 0 );
    return ret;
}


const char* getStringFromDouble( const char* fmt, double actualval, char* str )
{
    static StaticStringManager stm;
    char* ret = str ? str : stm.getString().buf();
    const bool isneg = actualval < 0;
    const double val = isneg ? -actualval : actualval;
    char* bufptr;

    if ( mIsUdf(val) )
	strcpy( ret, "1e30" );
    else
    {
	bufptr = ret;
	if ( isneg ) *bufptr++ = '-';
	if ( !fmt ) fmt = val > 1e-3 && val < 1e8 ? "%lf" : "%lg";
	sprintf( bufptr, fmt, val );
	prettyNumber( ret, 1 );
    }
    return ret;
}


const char* getBytesString( od_uint64 sz )
{
    const char* postfix[] = { " bytes", " KB", " MB", " GB", " TB", "PB" };

    unsigned char nrshifts;
    for ( nrshifts=0; nrshifts<4 && sz>=1024; nrshifts++ )
	sz >>= 10;

    static StaticStringManager stm;
    char* res = stm.getString().buf();
    getStringFromUInt64( sz, res );
    strcat( res, postfix[nrshifts] );

    return res;
}


static void truncFloatStr( float val, char* str )
{
    const int len = strlen( str );
    int pos = val < 0 ? 10 : 9;

    if ( len > pos )
    {
	char c = str[pos];
	str[pos--] = '\0';
	while ( 1 )
	{
	    if ( pos < 0 )
	    {
		/* We ran into a number like 9.999 */
		sprintf( str, "%d", mNINT32(val) );
		return;
	    }
	    if ( !isdigit(c) ) { pos--; continue; }
	    if ( c != '9' ) break;

	    c = str[pos];
	    str[pos] = c == '9' ? 0 : c + 1;

	    pos--;
	}
    }
}


const char* getStringFromFloat( const char* fmt, float actualval, char* str )
{
    static StaticStringManager stm;
    char* ret = str ? str : stm.getString().buf();
    static const char* normalfmt = "%f";
    const bool isneg = actualval < 0;
    const float val = isneg ? -actualval : actualval;

    if ( mIsUdf(val) )
	strcpy( ret, "1e30" ); /* Also for -1e30 therefore */
    else
    {
	char* bufptr = ret;
	if ( isneg ) *bufptr++ = '-';
	if ( !fmt ) fmt = val > 1e-3 && val < 1e8 ? normalfmt : "%g";
	sprintf( bufptr, fmt, val );
	if ( fmt == normalfmt )
	    truncFloatStr( actualval, ret );
	prettyNumber( ret, 0 );
    }
    return ret;
}


const char* getYesNoString( bool yn )
{
    static const char* strs[] = { "Yes", "No" };
    return yn ? strs[0] : strs[1];
}


const char* getDistUnitString( bool isfeet, bool wb )
{
    if ( isfeet )
	return wb ? "(ft)" : "ft";

    return wb ? "(m)" : "m";
}


int yesNoFromString( const char* str )
{
    if ( !str ) return false;
    mSkipBlanks( str );
    return *str == 'Y' || *str == 'y' || *str == 'T' || *str == 't';
}


int countCharacter( const char* str, char ch )
{
    int nr = 0;
    if ( !str || ! *str ) return nr;

    while ( *str )
    {
	if ( *str == ch ) nr++;
	str++;
    }

    return nr;
}


void replaceCharacter( char* str, char chorg, char chnew )
{
    if ( !str )
	return;

    while ( *str )
    {
	if ( *str == chorg ) *str = chnew;
	str++;
    }
}


void replaceString( char* str, const char* tok, const char* repl )
{
    const int toksz = tok ? strlen( tok ) : 0;
    const int replsz = repl ? strlen( repl ) : 0;

    if ( !str || !*str || toksz == 0 ) return;
    if ( !repl ) repl = "";

    char* tokptr = strstr( str, tok );
    if ( !tokptr ) return;

    char* restbuf = new char [ strlen(str) ];
    while ( tokptr )
    {
	strcpy( restbuf, tokptr + toksz );
	strcpy( tokptr, repl );
	tokptr += replsz;
	strcpy( tokptr, restbuf );
	tokptr = strstr( tokptr, tok );
    }
    delete [] restbuf;
}


void removeCharacter( char* str, char torem )
{
    char* curptr = str;
    if ( !str || ! *str ) return;

    while ( *curptr )
    {
	if ( *curptr != torem )
	{
	    if ( curptr != str )
		*str = *curptr;
	    str++;
	}
	curptr++;
    }
    *str = '\0';
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


void removeStartAndEndSpaces( char* str )
{
    if ( !str ) return;

    char* firstnonblank = str;
    while ( *firstnonblank && isspace( *firstnonblank ) )
	firstnonblank++;

    if ( *firstnonblank )
	memmove( str, firstnonblank, strlen( firstnonblank ) );

    char* lastnonblank = str + strlen(str)-1;

    while ( lastnonblank!=str && isspace( *lastnonblank ) )
    {
	*lastnonblank = 0;
	lastnonblank--;
    }
}



void cleanupString( char* str, bool spaceallow, bool slashallow, bool dotallow )
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
	    if ( slashallow )
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


static int getStringMatch( const char* str1, const char* str2, int ci )
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


bool matchString( const char* str1, const char* str2 )
{ return getStringMatch( str1, str2, false ); }
bool matchStringCI( const char* str1, const char* str2 )
{ return getStringMatch( str1, str2, true ); }


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


void prettyNumber( char* str, bool is_double )
{
    if ( !str ) return;
    if ( !*str ) { *str = '0'; *(str+1) = '\0'; return; }

    char ret[255]; char* ptrret = ret;

    /* find '.' and copy to end or 'E' */ 
    char* ptr = strrchr( str, '.' );
    if ( !ptr ) return;
    char* ptre = ptr; char* ptrb = ptrret;
    while ( *ptre && *ptre != 'e' && *ptre != 'E' ) *ptrb++ = *ptre++;
    *ptrb-- = '\0';

    /* If '.' at end of the mantissa, remove it */
    if ( ptre == ptr+1 )
    {
	if ( ptr == str )
	    { *ptr++ = '0'; *ptr = '\0'; }
	else
	    do { *ptr++ = *ptre; } while( *ptre++ );
	return;
    }

    /* Remove trailing '0' and '.', stop when '.' found */
    while ( ptrb >= ptrret && ( *ptrb == '0' || *ptrb == '.' ) )
    {
	*ptrb = '\0';
	if ( *ptrb-- == '.' ) break;
    }

    /* copy to string */
    ptrb = ptrret;
    while ( *ptrb ) *ptr++ = *ptrb++;
    while ( *ptre ) *ptr++ = *ptre++;
    *ptr = '\0';
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

    static const char* rets[] = { "th", "st", "nd", "rd" };
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

    static char* ret = 0;
    delete [] ret; ret = new char [nrchars+1];
    char* ptr = ret;

    static const char* dots = "...";
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

    const float km2 = m2*1e-6;

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
	    val = m2*mToFeetFactor*mToFeetFactor;
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

    static StaticStringManager stm;
    char* ret = str ? str : stm.getString().buf();
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
{ return getStringFromFloat(0,f, 0); }


const char* toString( double d )
{ return getStringFromDouble(0,d, 0); }


const char* toString( short i )
{ return getStringFromInt((int)i, 0); }


const char* toString( unsigned short i )
{ return getStringFromUInt( (unsigned int)i, 0 ); }


const char* toString( unsigned char c )
{ return toString( ((unsigned short)c) ); }


const char* toString( const char* str )
{
    static StaticStringManager stm;
    char* ret = stm.getString().buf();
    if ( str )
	strcpy( ret, str );
    else
	ret[0] = 0;

    return ret;
}


const char* toString( signed char c )
{
    static StaticStringManager stm;
    char* buf = stm.getString().buf();
    buf[0] = (char)c; buf[1] = '\0';
    return buf;
}


const char* toString( bool b )
{ const char* res = getYesNoString(b); return res; }
