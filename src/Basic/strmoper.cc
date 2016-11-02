/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream operations
-*/


#include "strmoper.h"

#include "genc.h"
#include "strmdata.h"
#include "perthreadrepos.h"
#include "uistrings.h"

#include <iostream>
#include <stdio.h>
#include <limits.h>

#include "bufstring.h"
#include "thread.h"
#ifdef __win__
# include "winstreambuf.h"
# define pclose _pclose
#endif

static const unsigned int nrretries = 4;
static const float retrydelay = 0.001;

bool StrmOper::resetSoftError( std::istream& strm, int& retrycount )
{
    if ( strm.good() )
	return true;
    else if ( strm.eof() || strm.fail() )
	return false;

    strm.clear();
    if ( retrycount > nrretries )
	return false;

    sleepSeconds( retrydelay );
    retrycount++;
    return true;
}


bool StrmOper::resetSoftError( std::ostream& strm, int& retrycount )
{
    if ( strm.good() )
	return true;
    else if ( strm.fail() )
	return false;

    strm.clear(); strm.flush();
    if ( retrycount > nrretries )
	return false;

    sleepSeconds( retrydelay );
    retrycount++;
    return true;
}


void StrmOper::clear( std::ios& strm )
{
    strm.clear();
}


static void readPrep(std::istream& strm)
{
    if (strm.bad())
    {
	sleepSeconds(retrydelay);
	strm.clear();
    }
    else if (strm.eof())
    {
	strm.clear();
    }
}


bool StrmOper::readBlock( std::istream& strm, void* ptr, od_uint64 nrbytes )
{
    if ( !ptr || mIsUdf(nrbytes) )
	return false;

    readPrep(strm);

    strm.read( (char*)ptr, nrbytes );

    nrbytes -= strm.gcount();
    if ( nrbytes > 0 )
    {
	if ( strm.eof() )
	    return false;

	char* cp = (char*)ptr + strm.gcount();
	int retrycount = 0;
	while ( resetSoftError(strm,retrycount) )
	{
	    strm.read( cp, nrbytes );
	    if ( strm.eof() )
		break;

	    nrbytes -= strm.gcount();
	    if ( nrbytes == 0 )
		break;

	    cp += strm.gcount();
	}
    }

    return nrbytes > 0 ? false : true;
}


bool StrmOper::writeBlock( std::ostream& strm, const void* ptr,
			   od_uint64 nrbytes )
{
    if ( strm.fail() || !ptr ) return false;

    strm.clear();
    strm.write( (const char*)ptr, nrbytes );
    if ( strm.good() )
	return true;
    if ( strm.fail() )
	return false;

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


bool StrmOper::peekChar( std::istream& strm, char& ch )
{
    readPrep(strm);

    int ich = strm.peek();
    if ( ich == 255 )
	ich = (int)' '; // Non-breaking-space.

    ch = (char)ich;
    return ich != EOF;
}


bool StrmOper::readChar( std::istream& strm, char& ch, bool allownls )
{
    if ( !peekChar(strm,ch) )
	return false;
    if ( !allownls && ch == '\n' )
	return false;
    strm.ignore( 1 );
    return true;
}


bool StrmOper::skipWhiteSpace( std::istream& strm )
{
    char ch;
    while ( true )
    {
	if ( !peekChar(strm,ch) )
	    return false;
	if ( iswspace(ch) )
	    strm.ignore( 1 );
	else
	    break;
    }
    return true;
}


static void addToBS( BufferString& bs, char* partbuf, int& pos, char ch )
{
    partbuf[pos] = ch;
    pos++;
    if ( pos == 1024 )
    {
	partbuf[pos] = '\0';
	bs.add( partbuf );
	pos = 0;
    }
}


static void removeLastIfCR( BufferString& bs )
{
    const int sz = bs.size();
    if ( sz < 1 )
	return;

    if ( bs[sz-1] == '\r' )
    {
	if ( sz == 1 )
	    bs.setEmpty();
	else
	    bs[sz-1] = '\0';
    }
}

#define mIsQuote() (ch == '\'' || ch == '"')


bool StrmOper::readWord( std::istream& strm, bool allownl, BufferString* bs )
{
    if ( bs )
	bs->setEmpty();

    char ch = ' ';
    while ( iswspace(ch) )
    {
	if ( !readChar(strm,ch,allownl) )
	    return false;
    }

    char quotechar = '\0';
    if ( ch == '\'' || ch == '"' )
    {
	quotechar = ch;
	if ( !readChar(strm,ch,allownl) )
	    return false;
    }

    char partbuf[1024+1]; partbuf[0] = ch;
    int bufidx = 1;

    while ( readChar(strm,ch,allownl) )
    {
	if ( quotechar == '\0' ? iswspace(ch) : ch == quotechar )
	    break;

	if ( bs )
	    addToBS( *bs, partbuf, bufidx, ch );
    }

    if ( bs && bufidx )
	{ partbuf[bufidx] = '\0'; bs->add( partbuf ); }

    if ( bs )
	removeLastIfCR( *bs );
    return !strm.bad();
}


bool StrmOper::readLine( std::istream& strm, BufferString* bs, bool* nlfound )
{
    if ( bs ) bs->setEmpty();
    if ( nlfound ) *nlfound = false;

    char ch;
    if ( !readChar(strm,ch,true) )
	return false;

    char partbuf[1024+1]; int bufidx = 0;
    while ( ch != '\n' )
    {
	if ( bs )
	    addToBS( *bs, partbuf, bufidx, ch );
	if ( !readChar(strm,ch,true) )
	    break;
    }

    if ( nlfound )
	*nlfound = ch == '\n';

    if ( bs && bufidx )
	{ partbuf[bufidx] = '\0'; *bs += partbuf; }

    if ( bs )
	removeLastIfCR( *bs );

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
    const int smalloffset = INT_MAX - 1;

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
    const int smalloffset = INT_MAX - 1;

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
    const std::streamoff pos32 = strm.tellg();
    //tellg updates the realpos in winfilebuf
    mDynamicCastGet(const std::winfilebuf*,winbuf,strm.rdbuf())
    return winbuf ? winbuf->getRealPos() : pos32;
#endif
}


od_int64 StrmOper::tell( std::ostream& strm )
{
#ifndef __win__
    return strm.tellp();
#else
    const std::streamoff pos32 = strm.tellp();
    //tellp updates the realpos in winfilebuf
    mDynamicCastGet(const std::winfilebuf*,winbuf,strm.rdbuf())
    return winbuf ? winbuf->getRealPos() : pos32;
#endif
}


od_int64 StrmOper::lastNrBytesRead( std::istream& strm )
{
    return strm.gcount();
}


uiString StrmOper::getErrorMessage( std::ios& strm )
{
    uiString msg;
    if ( strm.good() )
	{ msg.setEmpty(); return msg; }

    if ( strm.rdstate() & std::ios::eofbit )
	msg = od_static_tr( "StrmOperGetErrorMessage",
			    "File ended unexpectedly " );
    else
    {
	msg = toUiString( GetLastSystemErrorMessage() );
	if ( msg.isEmpty() )
	{
	    if ( strm.rdstate() & std::ios::failbit )
		msg = od_static_tr( "StrmOperGetErrorMessage",
				    "Recoverable error encountered " );
	    else if ( strm.rdstate() & std::ios::badbit )
		msg = od_static_tr( "StrmOperGetErrorMessage",
				    "Unrecoverable error encountered " );
	    else
		msg = od_static_tr( "StrmOperGetErrorMessage",
				    "Unknown error encountered " );
	}
    }

    return msg;
}


uiString StrmOper::getErrorMessage( const StreamData& sd )
{
    uiString msg;

    const int iotyp = sd.istrm ? -1 : (sd.ostrm ? 1 : 0);
    if ( iotyp == 0 )
    {
	uiString addedmsg = od_static_tr( "StrmOpergetErrorMessage", "file: " );
	msg = uiStrings::phrCannotOpen( addedmsg );
    }
    else if ( sd.streamPtr()->good() )
	msg = od_static_tr( "StrmOpergetErrorMessage", "Successfully opened " );
    else
	msg = getErrorMessage( *sd.streamPtr() );

    if ( sd.fileName() && *sd.fileName() )
	msg.append( toUiString(sd.fileName()) );

    if ( iotyp == 0 || !sd.streamPtr()->good() )
	msg.append( uiStrings::sCheckPermissions(), true );

    return msg;
}


//---- StreamData ----


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

    initStrms();
}


bool StreamData::usable() const
{
    return istrm || ostrm;
}


void StreamData::transferTo( StreamData& sd )
{
    sd = *this;
    initStrms();
}


std::ios* StreamData::streamPtr() const
{
    const std::ios* ret = istrm;
    if ( !istrm )
	ret = ostrm;
    return const_cast<std::ios*>( ret );
}
