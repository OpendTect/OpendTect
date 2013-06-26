/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 11-4-1994
 * FUNCTION : Functions for string manipulations
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    if ( !fmt )
	return getStringFromDouble( actualval, str );

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
	
	sprintf( bufptr, fmt, val );
	prettyNumber( ret, true );
    }
    return ret;
}


const char* getStringFromDouble( double actualval, char* str, int nrdigits )
{
    if ( nrdigits<=0 || nrdigits>15 ) nrdigits = 15;

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
	BufferString usedformat =  "%.";

	if ( val > 1e-3 && val < 1e8 )
	{
	    const int nonfractionsize = ((int) log10( val ))+1;
	    int fractionsize = nrdigits-nonfractionsize;
	    if ( fractionsize<0 ) fractionsize = 0;

	    usedformat += fractionsize;
	    usedformat += "f";
	}
	else
	{
	    usedformat += nrdigits;
	    usedformat += "g";
	}

	sprintf( bufptr, usedformat.buf(), val );
	prettyNumber( ret, true );
    }

    return ret;
}


const char* getBytesString( od_uint64 sz )
{
    NrBytesToStringCreator converter;
    converter.setUnitFrom( sz );
    return converter.getString( sz, 0 );
}


static void truncFloatStr( float val, char* str )
{
    const int len = strlen( str );
    int pos = val < 0 ? 10 : 9;

    if ( len > pos )
    {
	char c = str[pos];
	str[pos--] = '\0';
	while ( true )
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

const char* getStringFromFloat( float actualval, char* str, int nrdigits )
{
    if ( nrdigits<=0 || nrdigits>7 ) nrdigits = 7;

    static StaticStringManager stm;
    char* ret = str ? str : stm.getString().buf();
    const bool isneg = actualval < 0;
    const float val = isneg ? -actualval : actualval;
    char* bufptr;

    if ( mIsUdf(val) )
	strcpy( ret, "1e30" );
    else
    {
	bufptr = ret;
	if ( isneg ) *bufptr++ = '-';
	BufferString usedformat =  "%.";
	bool decimalformat = val > 1e-3 && val < 1e8;

	if ( decimalformat )
	{
	    const int nonfractionsize = ((int) log10( val ))+1;
	    int fractionsize = nrdigits-nonfractionsize;
	    if ( fractionsize<0 ) fractionsize = 0;

	    usedformat += fractionsize;
	    usedformat += "f";
	}
	else
	{
	    usedformat += nrdigits;
	    usedformat += "g";
	}

	sprintf( bufptr, usedformat.buf(), val );

	if ( decimalformat )
	    truncFloatStr( actualval, ret );

	prettyNumber( ret, true );
    }

    return ret;

}



const char* getStringFromFloat( const char* fmt, float actualval, char* str )
{
    if ( !fmt )
	return getStringFromFloat( actualval, str );

    static StaticStringManager stm;
    char* ret = str ? str : stm.getString().buf();
    const bool isneg = actualval < 0;
    const float val = isneg ? -actualval : actualval;
    char* bufptr;

    if ( mIsUdf(val) )
	strcpy( ret, "1e30" );
    else
    {
	bufptr = ret;
	if ( isneg ) *bufptr++ = '-';

	sprintf( bufptr, fmt, val );
	prettyNumber( ret, true );
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
{ return getStringFromFloat( f ); }


const char* toString( double d )
{ return getStringFromDouble( d ); }


const char* toString( short i )
{ return getStringFromInt((int)i, 0); }


const char* toString( unsigned short i )
{ return getStringFromUInt( (unsigned int)i, 0 ); }


const char* toString( unsigned char c )
{ return toString( ((unsigned short)c) ); }


const char* toString( const char* str )
{
    static StaticStringManager stm;
    BufferString& res = stm.getString();
    res = str;
    return res.buf();
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


bool getFromString( BufferString& res, const char* s )
{
    res = s;
    return true;
}


NrBytesToStringCreator::NrBytesToStringCreator()
    : unit_( Bytes )
{}


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
    
    static StaticStringManager stm;
    BufferString& res = stm.getString();
    
    getStringFromFloat( formatstr, fsz, res.buf() );
    
    if ( withunit )
    {
	res.add( " " );
	res.add( getUnitString() );
    }
    
    return res.str();
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

