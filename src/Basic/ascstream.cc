/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 7-7-1994
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ascstream.h"
#include "string2.h"
#include "general.h"
#include "timefun.h"
#include "convert.h"
#include "odver.h"
#include "strmoper.h"
#include "separstr.h"
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


mExternC( Basic)  void SetProjectVersionName(const char*);
extern "C" void SetProjectVersionName( const char* s )
{
    getPVN() = s;
}


ascostream::~ascostream()
{
    if ( mystrm ) delete streamptr;
}


bool ascostream::putHeader( const char* fltyp )
{
    stream() << GetProjectVersionName() << '\n' << fltyp << '\n'
	     << Time::getDateTimeString() << '\n';
    newParagraph();
    return stream().good();
}


void ascostream::newParagraph()
{
    stream() << mAscStrmParagraphMarker << std::endl;
}


void ascostream::putKeyword( const char* keyword, bool withsep )
{
    if ( !keyword || !*keyword ) return;

    BufferString towrite = keyword;
    char* ptr = strchr( towrite.buf(), mAscStrmKeyValSep );
    while ( ptr )
    {
	// Need to escape mAscStrmKeyValSep in keyword
	const od_int64 offs = ptr - towrite.buf();
	BufferString tmp( ptr + 1 );
	char escbuf[3];
	escbuf[0] = '\\'; escbuf[1] = mAscStrmKeyValSep; escbuf[2] = '\0';
	*ptr = 0;
	towrite += escbuf;
	towrite += tmp;
	ptr = strchr( towrite.buf() + offs + 2, mAscStrmKeyValSep );
    }

    stream() << towrite;
    if ( withsep )
	stream() << mAscStrmKeyValSep << ' ';
}


bool ascostream::put( const char* keyword, const char* value )
{
    putKeyword( keyword, (bool)value );

    if ( value )
    {
	const char* nlptr = strchr( value, '\n' );
	if ( nlptr )
	{
	    BufferString str( value );
	    char* startptr = str.buf();
	    char* ptr = startptr + (nlptr - value);
	    while ( ptr )
	    {
		*ptr++ = '\0';
		stream() << startptr << "\\n";
		startptr = ptr; 
		ptr = strchr( ptr, '\n' );
	    }
	    value += startptr - str.buf();
	}
	stream() << value;
    }

    stream() << '\n';
    return stream().good();
}


#define mDeclPut1IFn(typ) \
bool ascostream::put( const char* keyword, typ value ) \
{ \
    putKeyword( keyword ); stream() << value << '\n'; \
    return stream().good(); \
}
mDeclPut1IFn(int)
mDeclPut1IFn(od_uint32)
mDeclPut1IFn(od_int64)
mDeclPut1IFn(od_uint64)


bool ascostream::put( const char* keyword, float value )
{
    return mIsUdf(value) ? stream().good()
	 : put( keyword, toString(value) );
}


bool ascostream::put( const char* keyword, double value )
{
    return mIsUdf(value) ? stream().good()
	 : put( keyword, toString(value) );
}


bool ascostream::putYN( const char* keyword, bool yn )
{
    putKeyword( keyword );
    stream() << getYesNoString(yn) << '\n';
    return stream().good();
}


#define mDeclPut2Fn(fn,typ) \
bool ascostream::fn( const char* keyword, typ v1, typ v2 ) \
{ \
    FileMultiString fms; fms += v1; fms += v2; \
    putKeyword( keyword ); stream() << fms << '\n'; \
    return stream().good(); \
}
mDeclPut2Fn(put,int)
mDeclPut2Fn(put,od_uint32)
mDeclPut2Fn(put,od_int64)
mDeclPut2Fn(put,od_uint64)
mDeclPut2Fn(put,float)
mDeclPut2Fn(put,double)
mDeclPut2Fn(putYN,bool)


#define mDeclPut3Fn(fn,typ) \
bool ascostream::fn( const char* keyword, typ v1, typ v2, typ v3 ) \
{ \
    FileMultiString fms; fms += v1; fms += v2; fms += v3; \
    putKeyword( keyword ); stream() << fms << '\n'; \
    return stream().good(); \
}
mDeclPut3Fn(put,int)
mDeclPut3Fn(put,od_uint32)
mDeclPut3Fn(put,od_int64)
mDeclPut3Fn(put,od_uint64)
mDeclPut3Fn(put,float)
mDeclPut3Fn(put,double)
mDeclPut3Fn(putYN,bool)


#define mDeclPut4Fn(fn,typ) \
bool ascostream::fn( const char* keyword, typ v1, typ v2, typ v3, typ v4 ) \
{ \
    FileMultiString fms; fms += v1; fms += v2; fms += v3; fms += v4; \
    putKeyword( keyword ); stream() << fms << '\n'; \
    return stream().good(); \
}
mDeclPut4Fn(put,int)
mDeclPut4Fn(put,od_uint32)
mDeclPut4Fn(put,od_int64)
mDeclPut4Fn(put,od_uint64)
mDeclPut4Fn(put,float)
mDeclPut4Fn(put,double)
mDeclPut4Fn(putYN,bool)


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
    static const char* toreplace_newln =     "\\n";
    static const char* toreplace_to_newln =  "\n";
    static const char toreplace_separ[] =    { '\\', mAscStrmKeyValSep, '\0' };
    static const char toreplace_to_separ[] = { mAscStrmKeyValSep, '\0' };

    keybuf.setEmpty(); valbuf.setEmpty();
    if ( !streamptr || !streamptr->good() )
	return *this;

    BufferString lineread;
    if ( !StrmOper::readLine(stream(),&lineread) )
	return *this;
    char* linebuf = lineread.buf();
    if ( linebuf[0] == '\0' || ( linebuf[0]=='-' && linebuf[1]=='-' ) )
	return next();
    if ( linebuf[0] == mAscStrmParagraphMarker[0] )
	{ keybuf = mAscStrmParagraphMarker; return *this; }

    const int sz = lineread.size();
    char* separptr = 0;
    for ( int ich=1; ich<sz; ich++ )
    {
	const bool isunesc = linebuf[ich] == mAscStrmKeyValSep
	    		  && linebuf[ich-1]!='\\';
	if ( isunesc )
	    { separptr = linebuf+ich; break; }
    }

    if ( separptr )
    {
	char* startptr = separptr + 1;
	*separptr = '\0';
	mTrimBlanks(startptr);
	replaceString( startptr, toreplace_separ, toreplace_to_separ );
	replaceString( startptr, toreplace_newln, toreplace_to_newln );
	valbuf = startptr;
    }

    char* startptr = linebuf;
    mTrimBlanks(startptr);
    replaceString( startptr, toreplace_separ, toreplace_to_separ );
    replaceString( startptr, toreplace_newln, toreplace_to_newln );

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
    return toInt( v.buf() );
}


int ascistream::minorVersion() const
{
    BufferString v( version() );
    char* ptr = strrchr( v.buf(), '.' );
    return toInt( ptr ? ptr+1 : v.buf() );
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


#define mDeclGetFn(typ,fn) \
typ ascistream::get##fn( int idx ) const \
{ \
    if ( idx < 1 ) return Conv::to<typ>( valbuf.buf() ); \
    FileMultiString fms( valbuf.buf() ); \
    return Conv::to<typ>( fms[idx] ); \
}

mDeclGetFn(int,IValue)
mDeclGetFn(od_uint32,UIValue)
mDeclGetFn(od_int64,I64Value)
mDeclGetFn(od_uint64,UI64Value)
mDeclGetFn(float,FValue)
mDeclGetFn(double,DValue)
mDeclGetFn(bool,YN)
