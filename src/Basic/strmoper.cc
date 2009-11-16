/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream operations
-*/

static const char* rcsID = "$Id: strmoper.cc,v 1.21 2009-11-16 07:14:04 cvsranojay Exp $";

#include "strmoper.h"
#include "timefun.h"
#include "bufstring.h"
#include <iostream>

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
	for ( int idx=0; idx<nrretries; idx++ )
	{
	    Time_sleep( retrydelay );
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
    for ( int idx=0; idx<nrretries; idx++ )
    {
	Time_sleep( retrydelay );
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
	Time_sleep( retrydelay );
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
    static char bsbuf[1024+1];

    int bsidx = 0; char ch;
    bool getres = getNextChar(strm,ch);
    if ( !getres ) return false;

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
    BufferString curln;
    while ( true )
    {
	curln.setEmpty();
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


void StrmOper::seek( std::istream& strm, od_int64 offset, std::ios::seek_dir dir )
{
    int smalloffset = INT_MAX - 1;
    od_int64 curoffset = 0;
    od_int64 diff = 0;
    
    strm.seekg( 0, dir );
    
    if ( offset < smalloffset )
    { strm.seekg( offset, std::ios::cur ); return; }

    while ( curoffset < offset )
    {
	diff = offset - curoffset;
	diff > smalloffset ? strm.seekg( smalloffset, std::ios::cur ) 
			   : strm.seekg( diff, std::ios::cur );
	curoffset += smalloffset;
    }
}


void StrmOper::seek( std::istream& strm, od_int64 pos )
{
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
}