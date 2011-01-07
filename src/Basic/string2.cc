/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 11-4-1994
 * FUNCTION : Functions for string manipulations
-*/

static const char* rcsID = "$Id: string2.cc,v 1.1 2011-01-07 17:16:33 cvskris Exp $";

#include "string2.h"
#include "staticstring.h"
#include "undefval.h"
#include "mallocdefs.h"
#include <stdio.h>

#ifdef __win__
# define sDirSep        "\\"
#else
# define sDirSep        "/"
#endif


void removeTrailingBlanks( char* str )
{
    char* endptr;

    /* NULL or empty string */
    if ( !str || ! *str ) return;

    endptr = str + strlen(str) - 1;
    while( isspace(*endptr) )
    {
	*endptr = '\0';
	if ( str == endptr ) break;
	endptr--;
    }
}


const char* getStringFromInt( od_int32 val, char* str )

{
    char* ret = str ? str : StaticStringManager::STM().getString();
    sprintf( ret, "%d", val );
    return ret;
}


const char* getStringFromUInt( od_uint32 val, char* str )
{
    char* ret = str ? str : StaticStringManager::STM().getString();
    sprintf( ret, "%du", val );
    return ret;
}


/* Made the mkUIntStr function because %lld doesn't work on Windows */

static void mkUIntStr( char* buf, od_uint64 val, int isneg )
{
    int restval;
    char* pbuf = buf;
    char* pbuf2 = buf;
    char tmp;

    /* Fill the string with least significant first, i.e. reversed: */
    while ( val )
    {
	restval = val % 10;
	val /= 10;
	*pbuf++ = '0' + restval;
    }
    if ( isneg ) *pbuf++ = '-';
    *pbuf = '\0';

    /* Reverse to normal: */
    pbuf--;
    while ( pbuf > pbuf2 )
    {
	tmp = *pbuf; *pbuf = *pbuf2; *pbuf2 = tmp;
	pbuf--; pbuf2++;
    }
}


const char* getStringFromInt64( od_int64 val, char* str )
{
    char* ret = str ? str : StaticStringManager::STM().getString();
    int isneg = val < 0 ? 1 : 0;
    if ( isneg ) val = -val;
    mkUIntStr( ret, (od_uint64)val, isneg );
    return ret;
}


const char* getStringFromUInt64( od_uint64 val, char* str )
{
    char* ret = str ? str : StaticStringManager::STM().getString();
    mkUIntStr( ret, val, 0 );
    return ret;
}


const char* getStringFromDouble( const char* fmt, double actualval, char* str )
{
    char* ret = str ? str : StaticStringManager::STM().getString();
    int isneg = actualval < 0 ? mC_True : mC_False;
    double val = isneg ? -actualval : actualval;
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
    char nrshifts = 0;

    while ( nrshifts<4 && sz>=1024 )
    {
	nrshifts++;
	sz >>= 10;
    }

    char* res = StaticStringManager::STM().getString();
    getStringFromUInt64( sz, res );
    strcat( res, postfix[nrshifts] );

    return res;
}


static void truncFloatStr( float val, char* str )
{
    int len = strlen( str );
    int pos = val < 0 ? 10 : 9;
    char c;

    if ( len > pos )
    {
	c = str[pos];
	str[pos--] = '\0';
	while ( 1 )
	{
	    if ( pos < 0 )
	    {
		/* We ran into a number like 9.999 */
		sprintf( str, "%d", mNINT(val) );
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
    char* ret = str ? str : StaticStringManager::STM().getString();
    static const char* normalfmt = "%f";
    int isneg = actualval < 0 ? mC_True : mC_False;
    float val = isneg ? -actualval : actualval;
    char* bufptr;

    if ( mIsUdf(val) )
	strcpy( ret, "1e30" ); /* Also for -1e30 therefore */
    else
    {
	bufptr = ret;
	if ( isneg ) *bufptr++ = '-';
	if ( !fmt ) fmt = val > 1e-3 && val < 1e8 ? normalfmt : "%g";
	sprintf( bufptr, fmt, val );
	if ( fmt == normalfmt )
	    truncFloatStr( actualval, ret );
	prettyNumber( ret, 0 );
    }
    return ret;
}


const char* getYesNoString( int yn )
{
    static const char* yes = "Yes"; static const char* no = "No";
    return yn ? yes : no;
}


int yesNoFromString( const char* yn )
{
    if ( !yn ) return mC_False;
    mSkipBlanks( yn );
    return *yn == 'Y' || *yn == 'y' || *yn == 'T' || *yn == 't'
	 ? mC_True : mC_False;
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
    char* ptr;
    if ( !str || ! *str ) return;

    ptr = str;
    while ( *ptr )
    {
	if ( *ptr == chorg ) *ptr = chnew;
	ptr++;
    }
}


void replaceString( char* str, const char* tok, const char* repl )
{
    char* tokptr; char* restbuf;
    const int toksz = tok ? strlen( tok ) : 0;
    const int replsz = repl ? strlen( repl ) : 0;

    if ( !str || !*str || toksz == 0 ) return;
    if ( !repl ) repl = "";

    tokptr = strstr( str, tok );
    if ( !tokptr ) return;

    restbuf = mMALLOC( strlen(str), char );

    while ( tokptr )
    {
	strcpy( restbuf, tokptr + toksz );
	strcpy( tokptr, repl );
	tokptr += replsz;
	strcpy( tokptr, restbuf );
	tokptr = strstr( tokptr, tok );
    }

    mFREE( restbuf );
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


int isNumberString( const char* str, int int_only )
{
    int curdigit, prevdigit;
    int eseen = mC_False, dotseen = mC_False;

    if ( !str || !*str ) return mC_False;

    curdigit = isdigit(*str);
    if ( !*(str+1) )
	return curdigit;
    if ( !curdigit )
    {
	dotseen = *str == '.';
	if ( *str != '+' && *str != '-' && (int_only || !dotseen) )
	    return mC_False;
    }

    str++;
    prevdigit = curdigit;
    curdigit = isdigit(*str);
    if ( !curdigit )
    {
	dotseen = *str == '.';
	if ( !prevdigit )
	{
	    if ( !dotseen || (*(str-1) != '+' && *(str-1) != '-') )
		return mC_False;
	}
	eseen = *str == 'e' || *str == 'E';
	if ( int_only || (!dotseen && !eseen) )
	    return mC_False;
	if ( eseen && *(str+1) && (*(str+1) == '+' || *(str+1) == '-') )
	    str++;
    }

    str++;
    while ( *str )
    {
	curdigit = isdigit(*str);

	if ( !curdigit )
	{
	    if ( (*str == 'e' || *str == 'E') )
	    {
		if ( eseen )
		    return mC_False;

		eseen = mC_True;
		if ( *(str+1) && (*(str+1) == '+' || *(str+1) == '-') )
		    str++;
	    }
	    else if ( *str == '.' )
	    {
		if ( dotseen || eseen )
		    return mC_False;
		dotseen = mC_True;
	    }
	    else
		return mC_False;
	}

	str++;
    }

    return mC_True;
}


void cleanupString( char* str, int spaceallow, int slashallow, int dotallow )
{
    static int plusallow = mC_True;
    static int minusallow = mC_True;
    int dorepl;

    if ( !str ) return;
    while ( *str )
    {
	if ( !isalnum(*str) )
	{
	    dorepl = mC_True;
	    switch ( *str )
	    {
	    case ' ': case '\n' : case '\t':
		    	if ( spaceallow )	dorepl = mC_False;	break;
	    case '.':	if ( dotallow )		dorepl = mC_False;	break;
	    case '+':	if ( plusallow )	dorepl = mC_False;	break;
	    case '-':	if ( minusallow )	dorepl = mC_False;	break;
	    default:					break;
	    }
	    if ( slashallow )
	    {
		if ( *str == *sDirSep )
		    dorepl = mC_False;
#ifdef __win__
		if ( *str == ':' )
		    dorepl = mC_False;
#endif
	    }

	    if ( dorepl )
		*str = '_';
	}
	str++;
    }
}


int caseInsensitiveEqual( const char* str1, const char* str2, int nrchar )
{
    int chcount = 1;

    if ( !str1 && !str2 ) return mC_True;
    if ( !str1 || !str2 ) return mC_False;

    while ( *str1 && *str2 )
    {
	if ( tolower(*str1) != tolower(*str2) )
	    return mC_False;
	str1++ ; str2++; chcount++;
	if ( nrchar && chcount > nrchar )
	    return mC_True;
    }

    return *str1 || *str2 ? mC_False : mC_True;
}


static int getStringMatch( const char* str1, const char* str2, int ci )
{
    if ( !str1 && !str2 ) return mC_True;
    if ( !str1 || !str2 ) return mC_False;
    if ( ! *str1 )	  return mC_True;
    if ( ! *str2 )	  return mC_False;

    while ( *str1 )
    {
	if ( ci )
	    { if ( tolower(*str1) != tolower(*str2) ) return mC_False; }
	else
	    { if ( *str1 != *str2 ) return mC_False; }
	str1++ ; str2++;
    }
    return mC_True;
}


int matchString( const char* str1, const char* str2 )
{ return getStringMatch( str1, str2, mC_False ); }
int matchStringCI( const char* str1, const char* str2 )
{ return getStringMatch( str1, str2, mC_True ); }


static int getStringEndsWith( const char* str1, const char* str2, int ci )
{
    const char* start1 = str1; const char* start2 = str2;
    if ( !str1 && !str2 )	return mC_True;
    if ( !str1 || !str2 )	return mC_False;
    if ( !*str1 )		return mC_True;
    if ( !*str2 )		return mC_False;
    while ( *str1 ) str1++; while ( *str2 ) str2++;

    while ( mC_True )
    {
	if ( ci )
	    { if ( tolower(*str1) != tolower(*str2) ) return mC_False; }
	else
	    { if ( *str1 != *str2 ) return mC_False; }
	str1-- ; str2--;
	if ( str1 == start1 ) return mC_True;
	if ( str2 == start2 ) return mC_False;
    }
    return mC_True;
}

int stringEndsWith( const char* str1, const char* str2 )
{ return getStringEndsWith( str1, str2, mC_False ); }
int stringEndsWithCI( const char* str1, const char* str2 ) \
{ return getStringEndsWith( str1, str2, mC_True ); }


const char* getNextWord( const char* strptr, char* wordbuf )
{
    char* ptrwordbuf = wordbuf;
    if ( !wordbuf ) return 0;
    *wordbuf = '\0';
    if ( !strptr || !*strptr ) return strptr;

    mSkipBlanks( strptr );
    while ( *strptr && !isspace(*strptr) )
	*ptrwordbuf++ = *strptr++;
    *ptrwordbuf = '\0';

    return strptr;
}


void prettyNumber( char* str, int is_double )
{
    char ret[255];
    char* ptrret = ret;
    char* ptr;
    char* ptre;
    char* ptrb;

    if ( !str ) return;
    if ( !*str ) { *str = '0'; *(str+1) = '\0'; return; }

    /* find '.' and copy to end or 'E' */ 
    ptr = strrchr( str, '.' );
    if ( !ptr ) return;
    ptre = ptr;
    ptrb = ptrret;
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
    static const char* rets[] = { "th", "st", "nd", "rd" };
    if ( nr < 0 ) nr = -nr;

    if ( nr > 3 && nr < 21 )
	nr = 0;
    else
    {
	nr = nr % 10;
	if ( nr > 3 ) nr = 0;
    }
    return rets[ nr ];
}


int getIndexInStringArrCI( const char* text, const char* const * namearr,
			   int startnr, int nrchar, int notfoundidx )
{
    int idx = startnr;

    /* some sanity */
    if ( !text || !namearr || !*namearr ) return notfoundidx;
    mSkipBlanks(text);

    /* Look for match */
    while ( namearr[idx] )
    {
        if ( caseInsensitiveEqual( text, namearr[idx], nrchar ) )
            return idx;
        idx++;
    }

    /* No match found */
    return notfoundidx;
}


const char* getLimitedDisplayString( const char* inp, int nrchars,
				     int trimright )
{
    int inplen; char* ptr;
    static char* ret = 0;
    static const char* dots = "...";
    int i=0;

    if ( nrchars < 1 || !inp || !*inp ) return "";
    inplen = strlen( inp );
    if ( inplen < nrchars )
	return inp;

    mFREE(ret); ret = mMALLOC(nrchars+1,char); *ret = '\0';
    ptr = ret;
    if ( !trimright )
    {
	inp += inplen - nrchars + 3;
	strcpy( ret,  dots );
	ptr = ret + 3;
    }

    for( i=0; i<nrchars-3; i++ )
	*ptr++ = *inp++;
    *ptr = '\0';

    if ( trimright )
	strcat( ret,  dots );
    
    return ret;

    
}
