/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 7-7-1994
-*/

static const char* rcsID = "$Id: ascstream.cc,v 1.25 2008-11-19 09:44:26 cvsbert Exp $";

#include "ascstream.h"
#include "string2.h"
#include "general.h"
#include "timefun.h"
#include "convert.h"
#include "odver.h"
#include "strmoper.h"
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
    if ( pvn.isEmpty() )
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
    stream() << mAscStrmParagraphMarker << std::endl;
}


void ascostream::putKeyword( const char* keyword )
{
    if ( !keyword || !*keyword ) return;

    BufferString towrite = keyword;
    char* ptr = strchr( towrite.buf(), mAscStrmKeyValSep );
    while ( ptr )
    {
	// Need to escape mAscStrmKeyValSep in keyword
	const int offs = ptr - towrite.buf();
	BufferString tmp( ptr + 1 );
	char escbuf[3];
	escbuf[0] = '\\'; escbuf[1] = mAscStrmKeyValSep; escbuf[2] = '\0';
	*ptr = 0;
	towrite += escbuf;
	towrite += tmp;
	ptr = strchr( towrite.buf() + offs + 2, mAscStrmKeyValSep );
    }

    stream() << towrite << mAscStrmKeyValSep;
    if ( mAscStrmKeyValSep != '=' )
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
    return mIsUdf(value) ? stream().good()
	 : put( keyword, Conv::to<const char*>(value) );
}


bool ascostream::put( const char* keyword, double value )
{
    return mIsUdf(value) ? stream().good()
	 : put( keyword, Conv::to<const char*>(value) );
}


bool ascostream::putHeader( const char* fltyp )
{
    stream() << GetProjectVersionName() << '\n' << fltyp << '\n'
	     << Time_getFullDateString() << '\n';
    newParagraph();
    return stream().good();
}


ascistream::~ascistream()
{
    if ( mystrm ) delete streamptr;
}


void ascistream::init( std::istream* strm, bool rdhead )
{
    streamptr = strm;
    filetype = header = timestamp = "";
    if ( !streamptr ) return;

    if ( rdhead )
    {
	if ( !StrmOper::readLine(stream(),&header)
	  || !StrmOper::readLine(stream(),&filetype)
	  || !StrmOper::readLine(stream(),&timestamp) )
	    return;

	removeTrailingBlanks(filetype.buf());
	if ( filetype.size()>=4 )
	{
	    char* ptr = filetype.buf() + strlen(filetype) - 4;
	    if ( caseInsensitiveEqual(ptr,"file",0) ) *ptr = '\0';
	    removeTrailingBlanks(filetype.buf());
	}


	next();
    }
}


ascistream& ascistream::next()
{
    keybuf.setEmpty(); valbuf.setEmpty();

    if ( !streamptr || !streamptr->good() )
	return *this;

    BufferString lineread;
    if ( !StrmOper::readLine(stream(),&lineread) )
	return *this;

    char* linebuf = lineread.buf();
    if ( linebuf[0] == '\0' || ( linebuf[0]=='-' && linebuf[1]=='-' ) )
	return next();

    else if ( linebuf[0] == mAscStrmParagraphMarker[0] )
    {
	keybuf = mAscStrmParagraphMarker;
	return *this;
    }

    char* separptr = strchr( linebuf, mAscStrmKeyValSep );
    while ( separptr && separptr != linebuf && *(separptr-1) == '\\' )
    {
	for ( char* ptr=separptr-1; *ptr; ptr++ )
	    *ptr = *(ptr+1);
	separptr = strchr( separptr+1, mAscStrmKeyValSep );
    }

    char* startptr = separptr + 1;
    if ( separptr )
    {
	mTrimBlanks(startptr);
	valbuf = startptr;
	*separptr = '\0';
    }

    startptr = linebuf;
    mTrimBlanks(startptr);
    keybuf = startptr;

    return *this;
}


bool ascistream::isOfFileType( const char* ftyp ) const
{
    return matchStringCI( ftyp, filetype.buf() );
}


const char* ascistream::version() const
{
    const char* vptr = strrchr( header.buf(), 'V' );
    if ( vptr ) return vptr + 1;
    return "0.0";
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


ascistream::EntryType ascistream::type() const
{
    if ( !streamptr || !stream().good() )
	return EndOfFile;
    if ( keybuf.isEmpty() )
	return Empty;
    if ( *keybuf.buf() == *mAscStrmParagraphMarker )
	return ParagraphMark;
    if ( valbuf.isEmpty() )
	return Keyword;

    return KeyVal;
}


bool ascistream::hasKeyword( const char* keyw ) const
{
    if ( !keyw ) return keybuf.isEmpty();
    mSkipBlanks(keyw);
    return keybuf == keyw;
}


bool ascistream::hasValue( const char* val ) const
{
    if ( !val ) return valbuf.isEmpty();
    mSkipBlanks(val);
    return valbuf == val;
}


bool ascistream::getYN() const
{
    return yesNoFromString( valbuf );
}


int ascistream::getVal() const
{
    return valbuf.isEmpty() ? mUdf(int) : atoi( valbuf.buf() );
}


double ascistream::getValue() const
{
    return valbuf.isEmpty() ? mUdf(double) : atof( valbuf.buf() );
}
