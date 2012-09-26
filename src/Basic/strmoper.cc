/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream operations
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "strmoper.h"
#include "strmio.h"

#include "bufstring.h"
#include "thread.h"
#ifdef __win__
# include "winstreambuf.h"
#endif

#include <iostream>
#include <limits.h>

static const unsigned int nrretries = 4;
static const float retrydelay = 1;


bool StrmOper::readBlock( std::istream& strm, void* ptr, unsigned int nrbytes )
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
	for ( unsigned int idx=0; idx<nrretries; idx++ )
	{
	    Threads::sleep( retrydelay );
	    strm.clear();
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
			   unsigned int nrbytes )
{
    if ( strm.bad() || !ptr ) return false;

    strm.clear();
    strm.write( (const char*)ptr, nrbytes );
    if ( strm.good() ) return true;
    if ( strm.bad() ) return false;

    strm.flush();
    for ( unsigned int idx=0; idx<nrretries; idx++ )
    {
	Threads::sleep( retrydelay );
	strm.clear();
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
	ch = strm.peek();
	strm.ignore( 1 );
	return strm.good();
    }

    ch = (char)strm.peek();
    strm.ignore( 1 );
    return true;
}


bool StrmOper::wordFromLine( std::istream& strm, char* ptr, int maxnrchars )
{
    if ( !ptr ) return false;
    *ptr = '\0';

    char ch;
    char* start = ptr;
    while ( getNextChar(strm,ch) && ch != '\n' )
    {
	if ( !isspace(ch) )
	    *ptr++ = ch;
	else if ( ch == '\n' || *start )
	    break;

	maxnrchars--; if ( maxnrchars == 0 ) break;
    }

    *ptr = '\0';
    return ptr != start;
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
	if ( !getres ) return false;
    }

    if ( bs && bsidx )
	{ bsbuf[bsidx] = '\0'; *bs += bsbuf; }
    return strm.good();
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

    return sz > 0;
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



#define mWriteImpl(fn,typ) \
bool StreamIO::fn( const typ& val, const char* post ) \
{ \
    if ( binary_ ) \
	ostrm_->write( (const char*)&val, sizeof(val) ); \
    else \
	(*ostrm_) << val << post; \
    return ostrm_; \
}

mWriteImpl( writeInt16, od_int16 )
mWriteImpl( writeInt32, od_int32 )
mWriteImpl( writeInt64, od_int64 )
mWriteImpl( writeFloat, float )


#define mReadImpl(fn,typ) \
typ StreamIO::fn() const \
{ \
    typ val; \
    if ( binary_ ) \
	istrm_->read( (char*)(&val), sizeof(val) ); \
    else \
	(*istrm_) >> val; \
    return val; \
}

mReadImpl( readInt16, od_int16 )
mReadImpl( readInt32, od_int32 )
mReadImpl( readInt64, od_int64 )
mReadImpl( readFloat, float )

od_int64 StreamIO::tellg() const
{
    return istrm_ ? StrmOper::tell( *istrm_ ) : -1;
}

od_int64 StreamIO::tellp() const
{
    return ostrm_ ? StrmOper::tell( *ostrm_ ) : -1;
}

bool StreamIO::seekg( od_int64 pos, std::ios_base::seekdir dir )
{
    if ( !istrm_ ) return false;
    StrmOper::seek( *istrm_, pos, dir );
    return !istrm_->fail();
}

bool StreamIO::seekp( od_int64 pos, std::ios_base::seekdir dir )
{
    if ( !ostrm_ ) return false;
    StrmOper::seek( *ostrm_, pos, dir ); 
    return !ostrm_->fail();
}

