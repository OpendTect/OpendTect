/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream operations
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "strmoper.h"
#include "strmdata.h"
#include "staticstring.h"

#include "bufstring.h"
#include "thread.h"
#ifdef __win__
# include "winstreambuf.h"
# include <stdio.h>
# define pclose _pclose
#endif

#include <iostream>
#include <limits.h>

static const unsigned int nrretries = 4;
static const float retrydelay = 0.001;

bool StrmOper::resetSoftError( std::istream& strm, int& retrycount )
{
    if ( strm.good() || strm.eof() ) return true;
    else if ( strm.bad() ) return false;
    strm.clear();
    if ( retrycount > nrretries )
	return false;
    Threads::sleep( retrydelay );
    retrycount++;
    return true;
}

bool StrmOper::resetSoftError( std::ostream& strm, int& retrycount )
{
    if ( strm.good() ) return true;
    else if ( strm.bad() ) return false;
    strm.clear();
    strm.flush();
    if ( retrycount > nrretries )
	return false;
    Threads::sleep( retrydelay );
    retrycount++;
    return true;
}


bool StrmOper::readBlock( std::istream& strm, void* ptr, od_uint64 nrbytes )
{
    if ( strm.bad() || strm.eof() || !ptr ) return false;
    strm.clear();

    strm.read( (char*)ptr, nrbytes );
    if ( strm.bad() ) return false;

    nrbytes -= strm.gcount();
    if ( nrbytes > 0 )
    {
	if ( strm.eof() ) return false;

	char* cp = (char*)ptr + strm.gcount();
	int retrycount = 0;
	while ( resetSoftError(strm,retrycount) )
	{
	    strm.read( cp, nrbytes );
	    if ( strm.bad() || strm.eof() ) break;

	    nrbytes -= strm.gcount();
	    if ( nrbytes == 0 )
		{ strm.clear(); break; }

	    cp += strm.gcount();
	}
    }

    return nrbytes ? false : true;
}


bool StrmOper::writeBlock( std::ostream& strm, const void* ptr,
			   od_uint64 nrbytes )
{
    if ( strm.bad() || !ptr ) return false;

    strm.clear();
    strm.write( (const char*)ptr, nrbytes );
    if ( strm.good() ) return true;
    if ( strm.bad() ) return false;

    strm.flush();
    int retrycount = 0;
    while ( resetSoftError(strm,retrycount) )
    {
	strm.write( (const char*)ptr, nrbytes );
	if ( !strm.fail() )
	    break;
    }

    strm.flush();
    return strm.good();
}


bool StrmOper::getNextChar( std::istream& strm, char& ch )
{
    if ( strm.bad() || strm.eof() )
	return false;
    else if ( strm.fail() )
    {
	Threads::sleep( retrydelay );
	strm.clear();
	ch = (char) strm.peek();
	strm.ignore( 1 );
	return strm.good();
    }

    ch = (char)strm.peek();
    strm.ignore( 1 );
    return strm.good();
}


bool StrmOper::wordFromLine( std::istream& strm, char* ptr, int maxnrchars )
{
    BufferString bs;
    if ( wordFromLine( strm, bs ) )
	strncpy( ptr, bs.buf(), maxnrchars );

    *ptr = '\0';
    return false;
}


bool StrmOper::wordFromLine( std::istream& strm, BufferString& bs )
{
    bs.setEmpty();

    char ch;
    char* ptr = bs.buf();
    while ( getNextChar(strm,ch) && ch != '\n' )
    {
	if ( isspace(ch) )
	{
	    if ( ptr == bs.buf() )
		continue; // 'skip leading blanks'
	    break;
	}

	*ptr++ = ch;
	const od_int64 nrchar = ptr - bs.buf();
	if ( nrchar >= bs.bufSize() )
	{
	    bs.setBufSize( (unsigned int)(nrchar + 256) );
	    ptr = bs.buf() + nrchar;
	}
    }

    *ptr = '\0';
    return ptr != bs.buf();
}


#define mGetNextChar() \
    { getres = getNextChar(strm,ch); if ( !getres ) return false; }
#define mIsQuote() (ch == '\'' || ch == '"')


bool StrmOper::readWord( std::istream& strm, BufferString* bs )
{
    if ( bs ) bs->setEmpty();
    int bsidx = 0; char ch;
    bool getres = getNextChar(strm,ch);
    if ( !getres ) return false;

    while ( isspace(ch) )
	mGetNextChar();

    char quotetofind = '\0';
    if ( ch == '\'' || ch == '"' )
    {
	quotetofind = ch;
	mGetNextChar()
    }

    char bsbuf[1024+1];
    while ( !isspace(ch) )
    {
	if ( ch == quotetofind )
	    break;

	if ( bs )
	{
	    bsbuf[bsidx] = ch;
	    bsidx++;
	    if ( bsidx == 1024 )
	    {
		bsbuf[bsidx] = '\0';
		*bs += bsbuf;
		bsidx = 0;
	    }
	}
	mGetNextChar()
    }

    if ( bs && bsidx )
	{ bsbuf[bsidx] = '\0'; *bs += bsbuf; }
    return !strm.bad();
}


bool StrmOper::readLine( std::istream& strm, BufferString* bs )
{
    if ( bs ) bs->setEmpty();
    int bsidx = 0; char ch;
    bool getres = getNextChar(strm,ch);
    if ( !getres ) return false;

    char bsbuf[1024+1];
    while ( ch != '\n' )
    {
	if ( bs )
	{
	    bsbuf[bsidx] = ch;
	    bsidx++;
	    if ( bsidx == 1024 )
	    {
		bsbuf[bsidx] = '\0';
		*bs += bsbuf;
		bsidx = 0;
	    }
	}
	getres = getNextChar(strm,ch);
	if ( !getres )
	    break;
    }

    if ( bs && bsidx )
	{ bsbuf[bsidx] = '\0'; *bs += bsbuf; }
    return !strm.bad();
}


bool StrmOper::readFile( std::istream& strm, BufferString& bs )
{
    bs.setEmpty();
    BufferString curln;
    while ( true )
    {
	if ( !readLine(strm,&curln) )
	    break;
	curln += "\n";
	bs += curln;
    }

    int sz = bs.size();
    if ( sz > 0 && bs[sz-1] == '\n' )
	{ bs[sz-1] = '\0'; sz--; }

    return !strm.bad();
}


void StrmOper::seek( std::istream& strm, od_int64 offset,
		     std::ios::seekdir dir )
{
#ifndef __win32__
    strm.seekg( offset, dir );
#else
    static int smalloffset = INT_MAX - 1;
    
    if ( offset < smalloffset )
    { strm.seekg( offset, dir ); return; }

    strm.seekg( smalloffset, dir );
    od_int64 curoffset = smalloffset;

    while ( curoffset < offset )
    {
	const od_int64 diff = offset - curoffset;
	diff > smalloffset ? strm.seekg( smalloffset, std::ios::cur ) 
			   : strm.seekg( diff, std::ios::cur );
	curoffset += smalloffset;
    }
#endif
}


void StrmOper::seek( std::istream& strm, od_int64 pos )
{
#ifndef __win32__
    strm.seekg( pos, std::ios::beg );
#else
    int smalloffset = INT_MAX - 1;
    od_int64 curoffset = 0;
    od_int64 diff = 0;
    
    strm.seekg( 0, std::ios::beg );
    
    if ( pos < smalloffset )
    { strm.seekg( pos, std::ios::cur ); return; }

    while ( curoffset < pos )
    {
	diff = pos - curoffset;
	diff > smalloffset ? strm.seekg( smalloffset, std::ios::cur ) 
			   : strm.seekg( diff, std::ios::cur );
	curoffset += smalloffset;
    }
#endif
}


void StrmOper::seek( std::ostream& strm, od_int64 offset,
		     std::ios::seekdir dir )
{
#ifndef __win32__
    strm.seekp( offset, dir );
#else
    static int smalloffset = INT_MAX - 1;
    
    if ( offset < smalloffset )
    { strm.seekp( offset, dir ); return; }

    strm.seekp( smalloffset, dir );
    od_int64 curoffset = smalloffset;

    while ( curoffset < offset )
    {
	const od_int64 diff = offset - curoffset;
	diff > smalloffset ? strm.seekp( smalloffset, std::ios::cur ) 
			   : strm.seekp( diff, std::ios::cur );
	curoffset += smalloffset;
    }
#endif
}


void StrmOper::seek( std::ostream& strm, od_int64 pos )
{
    StrmOper::seek( strm, pos, std::ios::beg );
}


od_int64 StrmOper::tell( std::istream& strm )
{
#ifndef __win__
    return strm.tellg();
#else
    strm.tellg();
    mDynamicCastGet(const std::winfilebuf*,winbuf,strm.rdbuf())
    return winbuf ? winbuf->getRealPos() : -1;
#endif
}


od_int64 StrmOper::tell( std::ostream& strm )
{
#ifndef __win__
    return strm.tellp();
#else
    strm.tellp();
    mDynamicCastGet(const std::winfilebuf*,winbuf,strm.rdbuf())
    return winbuf ? winbuf->getRealPos() : -1;
#endif
}


od_int64 StrmOper::lastNrBytesRead( std::istream& strm )
{
    return strm.gcount();
}


const char* StrmOper::getErrorMessage( std::ios& strm )
{
    mDeclStaticString( ret );
    if ( strm.good() )
	{ ret.setEmpty(); return ret; }

    if ( strm.rdstate() & std::ios::eofbit )
	ret = "File ended unexpectedly";
    else
    {
	ret = GetLastSystemErrorMessage();
	if ( ret.isEmpty() )
	{
	    if ( strm.rdstate() & std::ios::failbit )
		ret = "Recoverable error encountered";
	    else if ( strm.rdstate() & std::ios::badbit )
		ret = "Unrecoverable error encountered";
	    else
		ret = "Unknown error encountered";
	}
    }

    return ret.buf();
}


const char* StrmOper::getErrorMessage( const StreamData& sd )
{
    mDeclStaticString( ret );
    ret.setEmpty();

    const int iotyp = sd.istrm ? -1 : (sd.ostrm ? 1 : 0);

    if ( sd.fileName() && *sd.fileName() )
    {
	if ( iotyp )
	    ret.add( iotyp > 0 ? "Writing " : "Reading " );
	ret.add( "'" ).add( sd.fileName() ).add( "': " );
    }

    if ( iotyp == 0 )
	ret.add( "Cannot open file" );
    else if ( sd.streamPtr()->good() )
	ret.add( "Successfully opened" );
    else
	ret.add( getErrorMessage(*sd.streamPtr()) );

    return ret.buf();
}


//---- StreamData ----

StreamData& StreamData::operator =( const StreamData& sd )
{
    if ( this != &sd )
	copyFrom( sd );
    return *this;
}


void StreamData::close()
{
    if ( istrm && istrm != &std::cin )
    	delete istrm;

    if ( ostrm )
    {
	ostrm->flush();
	if ( ostrm != &std::cout && ostrm != &std::cerr )
	    delete ostrm;
    }

    if ( fp_ && fp_ != stdin && fp_ != stdout && fp_ != stderr )
	{ if ( ispipe_ ) pclose(fp_); else fclose(fp_); }

    initStrms();
}


bool StreamData::usable() const
{
    return ( istrm || ostrm ) && ( !ispipe_ || fp_ );
}


void StreamData::copyFrom( const StreamData& sd )
{
    istrm = sd.istrm; ostrm = sd.ostrm;
    fp_ = sd.fp_; ispipe_ = sd.ispipe_;
    setFileName( sd.fname_ );
}


void StreamData::transferTo( StreamData& sd )
{
    sd.copyFrom( *this );
    initStrms();
}


void StreamData::setFileName( const char* f )
{
    delete [] fname_;
    fname_ = f ? new char [strlen(f)+1] : 0;
    if ( fname_ ) strcpy( fname_, f );
}


FILE* StreamData::filePtr() const
{
    return const_cast<FILE*>( fp_ );
}


std::ios* StreamData::streamPtr() const
{
    const std::ios* ret = istrm;
    if ( !istrm )
	ret = ostrm;
    return const_cast<std::ios*>( ret );
}
