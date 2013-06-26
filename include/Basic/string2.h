#ifndef string2_h
#define string2_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		11-4-1994
 Contents:	Extra string functions
 RCS:		$Id$
________________________________________________________________________
-*/

#include "basicmod.h"
#include "commondefs.h"

class BufferString;

/*!
\brief Advances given pointer to first non-whitespace.
*/

#define mSkipBlanks(ptr) \
    { if ( ptr ) { while ( *(ptr) && isspace(*(ptr)) ) (ptr)++; } }

/*!
\brief Advances given pointer to first whitespace.
*/

#define mSkipNonBlanks(ptr) \
    { if ( ptr ) { while ( *(ptr) && !isspace(*(ptr)) ) (ptr)++; } }

/*!
\brief Advances to first whitespace and removes trailing whitespace.
*/

#define mTrimBlanks(ptr) \
    { mSkipBlanks(ptr); removeTrailingBlanks(ptr); }



/*!> bluntly puts a '\0' on trailing white space. */
mGlobal(Basic) void removeTrailingBlanks(char*);


#ifndef __cpp__
# include "string2_c.h"
#else

#include "undefval.h"
#include "fixedstring.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
/*!> stricmp with option to compare part, default is all */
mGlobal(Basic) bool caseInsensitiveEqual(const char*,const char*,int match_nrchars=-1);
/*!> checks whether a string is the start of another string. */
mGlobal(Basic) bool matchString(const char* startstring,const char* maybebigger);
/*!> is a case insensitive version of matchString */
mGlobal(Basic) bool matchStringCI(const char*,const char*);
mGlobal(Basic) bool stringEndsWith(const char* endstring,const char* maybebigger);
mGlobal(Basic) bool stringEndsWithCI(const char*,const char*);

/*!> counts occurrences of a char in string */
mGlobal(Basic) int countCharacter(const char*,char);
/*!> replaces all occurrences of a char with another */
mGlobal(Basic) void replaceCharacter(char*,char from,char to);
/*!> replaces all occurrences of a string with another */
mGlobal(Basic) void replaceString(char*,const char* from,const char* to);
/*!> removes all occurrences of a char */
mGlobal(Basic) void removeCharacter(char*,char);
/*!> cleans a string from non-alpha numeric by replacing with underscores.
     params: allow whitespace, allow slashes, allow dots */
mGlobal(Basic) void cleanupString(char*,bool,bool,bool);
/*!> Removes initial and trailing spaces and tabs*/
mGlobal(Basic) void removeStartAndEndSpaces(char*);
/*!> tells whether a string holds a parseable number */
mGlobal(Basic) bool isNumberString(const char*,bool int_only=false);
/*!> tells whether has printable characters only. */
mGlobal(Basic) bool isAlphaNumString(const char*,bool allowspace = false);

/*!> fills a buffer with the next word (delimited by whitespace) in string.
     It returns a ptr just after the word. */
mGlobal(Basic) const char* getNextWord(const char*,char*);

/*!> Fills string with string for an int.
     If you pass 0 for retbuf, then a static buffer is used. */
mGlobal(Basic) const char* getStringFromInt(od_int32,char* retbuf);
mGlobal(Basic) const char* getStringFromUInt(od_uint32,char* retbuf);
mGlobal(Basic) const char* getStringFromInt64(od_int64,char* retbuf);
mGlobal(Basic) const char* getStringFromUInt64(od_uint64,char* retbuf);

/*!> Normally, pass null for fmt. Then it will do removal of
     trailing zeros and use %lf in more cases than std.
     If you pass 0 for retbuf, then a static buffer is used (not MT safe). */
mGlobal(Basic) const char* getStringFromDouble(const char* fmt,double,
					       char* retbuf);

/*>Prints a double with the requested nr of digits. 
    Use the returned string result immediately.*/
mGlobal(Basic) const char* getStringFromDouble(double,char* retbuf=0, 
					       int nrdigits=15);

/*!> is like getStringFromDouble, with special %f treatment. */

mGlobal(Basic) const char* getStringFromFloat(const char* fmt,float,char* retbuf);

/*>Prints a float with the requested nr of digits.
	    Use the returned string result immediately.*/
mGlobal(Basic) const char* getStringFromFloat(float,char* retbuf=0,
					      int nrdigits=7);
/*!> removes unwanted zeros and dots from a floating point in string. */
mGlobal(Basic) void prettyNumber(char*,bool is_float);

/*!> returns ptr to static buffer with "yes" or "No". */
mGlobal(Basic) const char* getYesNoString(bool);
/*!> returns ptr to static buffer with "m" or "ft" */
mGlobal(Basic) const char* getDistUnitString(bool isfeet,bool withparentheses);

/*!> returns 1 or 0 by inspecting string */
mGlobal(Basic) int yesNoFromString(const char*);
/*!> returns "th" or "st" or "nd" or "rd"; like in 1st, 2nd, 3rd etc. */
mGlobal(Basic) const char* getRankPostFix(int);
/*!> returns a nicely, readable size, in bytes, KB, MB, GB, or TB */
mGlobal(Basic) const char* getBytesString(od_uint64);
/*!> returns a string for display, never larger than specified nr of chars */
mGlobal(Basic) const char* getLimitedDisplayString(const char*,int nrchars,
					    bool trimright);

mGlobal(Basic) const char* quoteString(const char* str, char qt='"' );
mGlobal(Basic) inline const char* backQuoteString(const char* str, char qt='"' )
{ return quoteString( str, '`' ); }

/*!> Finds a string in string array, case insensitive */
mGlobal(Basic) int getIndexInStringArrCI(const char*,const char* const* arr,
				  int startnr=0,int nr_chars_to_match=0,
				  int notfoundidx=-1);

/*!>Returns a string with an area and its unit, depending on survey and
    area size, unit is ft^2, m^2, km^2 or mile^2. */
mGlobal(Basic) const char* getAreaString( float m2, bool parensonunit, char* str=0 );

// toString functions. 
mGlobal(Basic) const char* toString( od_int32 i );
mGlobal(Basic) const char* toString( od_uint32 i );
mGlobal(Basic) const char* toString( od_int64 i );
mGlobal(Basic) const char* toString( od_uint64 i );
mGlobal(Basic) const char* toString( float f )	;
mGlobal(Basic) const char* toString( double d );
mGlobal(Basic) const char* toString( short i );
mGlobal(Basic) const char* toString( unsigned short i );
mGlobal(Basic) const char* toString( const char* str );
mGlobal(Basic) const char* toString( unsigned char c );
mGlobal(Basic) const char* toString( signed char c );
mGlobal(Basic) const char* toString( bool b );

/*!Converts integer with number of bytes to a string with KB, GB or similar
   unit. */

mExpClass(Basic) NrBytesToStringCreator
{
public:
			NrBytesToStringCreator();
    enum Unit		{ Bytes=0, KB=1, MB=2, GB=3, TB=4, PB=5 };
    
    void		setUnitFrom(od_uint64 number,bool maximum=true);
			/*!<Sets the unit (B, KB, MB, GB, TB) based on the
			 number.
			 \param maximum will only change unit if a larger
			 unit is needed.
			 */
    
    FixedString		getString(od_uint64 number,int nrdecimals=2,
				  bool withunit=true) const;
			/*!<Use string before doing anything else, as it will be
			    overwritten at next call from same thread. */
    
    FixedString		getUnitString() const;
    static FixedString	toString(Unit);
    
protected:
    Unit		unit_;
};


#define mImplGetFromStrFunc( type, func, udfv ) \
inline bool getFromString( type& i, const char* s, type undef=udfv ) \
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



// inline bool getFromString( double& d, const char* s, double udefval );
mImplGetFromStrFunc(double, strtod(s,&e), mUdf(double) )
// inline bool getFromString( float& d, const char* s, float udefval );
mImplGetFromStrFunc(float, strtod(s,&e), mUdf(float) )
// inline bool getFromString( int& d, const char* s, int udefval );
mImplGetFromStrFunc(int, strtol(s,&e,10), mUdf(int) )
#undef mImplGetFromStrFunc

mGlobal(Basic) bool getFromString(BufferString&,const char*);

inline bool getFromString( bool& b, const char* s )
{
    if ( s )
    {
	b = ( yesNoFromString( s ) ? true : false );
	return true;
    }

    b = false;
    return false;
}

inline bool toBool( const char* s, bool defval=true )
{
    return s && *s ? yesNoFromString(s) : defval;
}

inline float toFloat( const char* s, float defval=0 )
{
    float ret = defval; getFromString( ret, s, ret ); return ret;
}

inline double toDouble( const char* s, double defval=0 )
{
    double ret = defval; getFromString( ret, s, ret ); return ret;
}

inline int toInt( const char* s, int defval=0 )
{
    int ret = defval; getFromString( ret, s, ret ); return ret;
}


#endif


#endif

