/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 7-7-1994
-*/

static const char* rcsID = "$Id: ascstream.cc,v 1.1.1.1 1999-09-03 10:11:27 dgb Exp $";

#include "ascstream.h"
#include "unitscale.h"
#include "string2.h"
#include "general.h"
#include "timefun.h"
#include <string.h>
#include <stdlib.h>
#include <iostream.h>


ascostream::~ascostream()
{
    if ( mystrm ) delete streamptr;
}


void ascostream::newParagraph()
{
    stream() << mParagraphMarker << endl;
}


void ascostream::putKeyword( const char* keyword )
{
    if ( !keyword ) return;

    if ( pad )
    {
	int keywlength = colonpos;
	if ( tabs ) { stream() << '\t'; keywlength -= 8; }
	if ( (int)strlen(keyword) > keywlength )
	    stream() << keyword << mKeyValSepar << ' ';
	else
	{
	    char buf[132];
	    if ( strlen((char*)keyword) > 131 )
		stream() << keyword << mKeyValSepar << '\t';
	    else
	    {
		getPaddedString( (char*)keyword, buf, keywlength );
		stream() << buf << mKeyValSepar << '\t';
	    }
	}
    }
    else
    {
	if ( tabs ) stream() << '\t';
	stream() << keyword << mKeyValSepar << ' ';
    }
}


bool ascostream::put( const char* keyword, const char* value )
{
    if ( !value ) stream() << keyword;
    else
    {
	putKeyword( keyword );
	stream() << value;
    }
    stream() << '\n';
    return stream().fail() ? NO : YES;
}


bool ascostream::putYN( const char* keyword, bool yn )
{
    putKeyword( keyword );
    stream() << getYesNoString(yn) << '\n';
    return stream().fail() ? NO : YES;
}


bool ascostream::put( const char* keyword, int value )
{
    putKeyword( keyword );
    stream() << value << '\n';
    return stream().fail() ? NO : YES;
}


bool ascostream::put( const char* keyword, float value, const MeasureUnit* mu )
{
    if ( !mIsUndefined(value) )
    {
	putKeyword( keyword );
	if ( mu ) value = mu->fromSI( value );
	stream() << value;
	stream() << '\n';
    }
    return stream().fail() ? NO : YES;
}


bool ascostream::put( const char* keyword, double value, const MeasureUnit* mu )
{
    if ( !mIsUndefined(value) )
    {
	if ( mu ) value = mu->fromSI( value );
	return put( keyword, getStringFromDouble("%lg",value) );
    }
    return stream().fail() ? NO : YES;
}


bool ascostream::putHeader( const char* fltyp, const char* pspec )
{
    if ( !pspec ) pspec = GetProjectVersionName();
    stream() << pspec << '\n' << fltyp << '\n' << Time_getLocalString() << '\n';
    newParagraph();
    return stream().good();
}


ascistream::~ascistream()
{
    if ( mystrm ) delete streamptr;
}


void ascistream::init( istream* strm, int rdhead )
{
    streamptr = strm;
    filetype[0] = header[0] = timestamp[0] =
    keyword[0] = valstr[0] = curword[0] =
    tabbed = NO;
    nextwordptr = valstr;
    if ( !streamptr ) return;

    if ( rdhead )
    {
	stream().getline( header, mMaxFileHeadLength );
	stream().getline( filetype, mMaxFileHeadLength );
	stream().getline( timestamp, mMaxFileHeadLength );
	stream().getline( keyword, mMaxFileEntryLength );

	removeTrailingBlanks(filetype);
	char* ptr = filetype + strlen(filetype) - 4;
	if ( caseInsensitiveEqual(ptr,"file",0) ) *ptr = '\0';
	removeTrailingBlanks(filetype);
    }
}


void ascistream::resetCurrent()
{
    keyword[0] = valstr[0] = curword[0] = '\0';
    nextwordptr = valstr;
}


static char linebuf[1024];

ascistream& ascistream::next()
{
    resetCurrent();
    if ( !streamptr || stream().fail() || stream().eof() )
	return *this;

    nextLine( linebuf, 1024 );
    if ( linebuf[0] == '\0' || ( linebuf[0]=='-' && linebuf[1]=='-' ) )
	return next();
    else if ( linebuf[0] == mParagraphMarker[0] )
    {
	strcpy( keyword, mParagraphMarker );
	return *this;
    }
    tabbed = linebuf[0] == '\t';

    char* separptr = strchr( linebuf, mKeyValSepar );
    char* startptr = separptr + 1;
    if ( separptr )
    {
	skipLeadingBlanks(startptr);
	removeTrailingBlanks(startptr);
	strcpy( valstr, startptr );
	*separptr = '\0';
    }

    startptr = linebuf;
    skipLeadingBlanks(startptr);
    removeTrailingBlanks(startptr);
    strcpy( keyword, startptr );

    return *this;
}


ascistream& ascistream::nextLine( char* buf, int maxlen )
{
    stream().getline( buf, maxlen );
    return *this;
}


bool ascistream::isOfFileType( const char* ftyp ) const
{
    return matchStringCI( ftyp, filetype );
}


const char* ascistream::version() const
{
    const char* Vptr = strrchr( header, 'V' );
    return Vptr ? Vptr+1 : &header[strlen(header)];
}


const char* ascistream::nextWord()
{
    nextwordptr = getNextWord( nextwordptr, curword );
    return curword;
}


ascistream::EntryType ascistream::type() const
{
    if ( !streamptr || stream().fail() || stream().eof() )
	return EndOfFile;
    if ( !keyword[0] )
	return Empty;
    if ( keyword[0] == mParagraphMarker[0] )
	return ParagraphMark;
    if ( !valstr[0] )
	return Keyword;

    return KeyVal;
}


bool ascistream::hasKeyword( const char* keyw ) const
{
    if ( !keyw ) return keyword[0] ? NO : YES;
    skipLeadingBlanks(keyw);
    return !strcmp( keyword, keyw );
}


bool ascistream::hasValue( const char* val ) const
{
    if ( !val ) return valstr[0] ? NO : YES;
    skipLeadingBlanks(val);
    return !strcmp( valstr, val );
}


bool ascistream::getYN() const
{
    return yesNoFromString( valstr );
}


int ascistream::getVal() const
{
    return atoi( valstr );
}


double ascistream::getValue( const MeasureUnit* mu ) const
{
    double res = atof( valstr );
    if ( mu && !mIsUndefined(res) )
	res = mu->toSI( res );
    return res;
}
