/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 7-7-1994
-*/

static const char* rcsID = "$Id: ascstream.cc,v 1.14 2004-04-27 15:51:15 bert Exp $";

#include "ascstream.h"
#include "string2.h"
#include "general.h"
#include "timefun.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>


static BufferString& getPVN()
{
    static BufferString* projvernm = 0;
    if ( !projvernm )
	projvernm = new BufferString;
    return *projvernm;
}


extern "C" const char* GetProjectVersionName()
{
    BufferString& pvn = getPVN();
    if ( pvn == "" )
    {
	pvn = "dTect";
	pvn += " V";
	pvn += mODMajorVersion;
	pvn += ".";
	pvn += mODMinorVersion;
    }
    return pvn.buf();
}


extern "C" void SetProjectVersionName( const char* s )
{
    getPVN() = s;
}


ascostream::~ascostream()
{
    if ( mystrm ) delete streamptr;
}


void ascostream::newParagraph()
{
    stream() << mParagraphMarker << std::endl;
}


void ascostream::putKeyword( const char* keyword )
{
    if ( !keyword ) return;

    if ( tabs ) stream() << '\t';
    stream() << keyword << keyvalsep;
    if ( keyvalsep != '=' )
	stream() << ' ';
}


bool ascostream::put( const char* keyword, const char* value )
{
    if ( !value )
	stream() << keyword;
    else
    {
	putKeyword( keyword );
	stream() << value;
    }
    stream() << '\n';
    return stream().good();
}


bool ascostream::putYN( const char* keyword, bool yn )
{
    putKeyword( keyword );
    stream() << getYesNoString(yn) << '\n';
    return stream().good();
}


bool ascostream::put( const char* keyword, int value )
{
    putKeyword( keyword );
    stream() << value << '\n';
    return stream().good();
}


bool ascostream::put( const char* keyword, float value )
{
    return mIsUndefined(value) ? stream().good()
	 : put( keyword, getStringFromFloat(0,value) );
}


bool ascostream::put( const char* keyword, double value )
{
    return mIsUndefined(value) ? stream().good()
	 : put( keyword, getStringFromDouble(0,value) );
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


void ascistream::init( std::istream* strm, int rdhead )
{
    streamptr = strm;
    filetype[0] = header[0] = timestamp[0] = curword[0] = '\0';
    tabbed = false;
    nextwordptr = valbuf;
    if ( !streamptr ) return;

    if ( rdhead )
    {
	stream().getline( header, mMaxFileHeadLength );
	stream().getline( filetype, mMaxFileHeadLength );
	stream().getline( timestamp, mMaxFileHeadLength );

	removeTrailingBlanks(filetype);
	char* ptr = filetype + strlen(filetype) - 4;
	if ( caseInsensitiveEqual(ptr,"file",0) ) *ptr = '\0';
	removeTrailingBlanks(filetype);

	next();
    }
}


void ascistream::resetPtrs( bool mkempty )
{
    if ( mkempty )
	{ keybuf = ""; valbuf = ""; curword[0] = '\0'; }

    nextwordptr = valbuf;
}


ascistream& ascistream::next()
{
    resetPtrs( true );
    if ( !streamptr || !streamptr->good() )
	return *this;

    static char linebuf[mMaxLineLength+1];
    if ( !stream().getline(linebuf,mMaxLineLength) )
	return *this;

    if ( linebuf[0] == '\0' || ( linebuf[0]=='-' && linebuf[1]=='-' ) )
	return next();

    else if ( linebuf[0] == mParagraphMarker[0] )
    {
	keybuf = mParagraphMarker;
	return *this;
    }
    tabbed = linebuf[0] == '\t';

    char* separptr = strchr( linebuf, keyvalsep );
    char* startptr = separptr + 1;
    if ( separptr )
    {
	skipLeadingBlanks(startptr);
	removeTrailingBlanks(startptr);
	valbuf = startptr;
	*separptr = '\0';
    }

    startptr = linebuf;
    skipLeadingBlanks(startptr);
    removeTrailingBlanks(startptr);
    keybuf = startptr;

    resetPtrs( false );
    return *this;
}


bool ascistream::isOfFileType( const char* ftyp ) const
{
    return matchStringCI( ftyp, filetype );
}


const char* ascistream::projName() const
{
    static char buf[20];
    strncpy( buf, header, 20 );
    char* ptr = strchr( buf, ' ' );
    if ( ptr ) *ptr = '\0';
    return buf;
}


const char* ascistream::version() const
{
    const char* Vptr = strrchr( header, 'V' );
    return Vptr ? Vptr+1 : &header[strlen(header)];
}


int ascistream::majorVersion() const
{
    BufferString v( version() );
    char* ptr = strchr( v.buf(), '.' );
    if ( ptr ) *ptr = '\0';
    return atoi( v.buf() );
}


int ascistream::minorVersion() const
{
    BufferString v( version() );
    char* ptr = strrchr( v.buf(), '.' );
    return atoi( ptr ? ptr+1 : v.buf() );
}


const char* ascistream::nextWord()
{
    nextwordptr = getNextWord( nextwordptr, curword );
    return curword;
}


ascistream::EntryType ascistream::type() const
{
    if ( !streamptr || !stream().good() )
	return EndOfFile;
    if ( keybuf == "" )
	return Empty;
    if ( *(const char*)keybuf == *mParagraphMarker )
	return ParagraphMark;
    if ( valbuf == "" )
	return Keyword;

    return KeyVal;
}


bool ascistream::hasKeyword( const char* keyw ) const
{
    if ( !keyw ) return keybuf == "";
    skipLeadingBlanks(keyw);
    return keybuf == keyw;
}


bool ascistream::hasValue( const char* val ) const
{
    if ( !val ) return valbuf == "";
    skipLeadingBlanks(val);
    return valbuf == val;
}


bool ascistream::getYN() const
{
    return yesNoFromString( valbuf );
}


int ascistream::getVal() const
{
    return atoi( valbuf );
}


double ascistream::getValue() const
{
    double res = atof( valbuf );
    if ( mIsUndefined(res) )
	res = mUndefValue;
    return res;
}
