/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "strmoper.h"

#include "genc.h"
#include "strmdata.h"
#include "uistrings.h"

#include <iostream>
#include <stdio.h>
#include <limits.h>
#include <sstream>
#include <iosfwd>

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
    while ( ch != '\n' && ch != '\r' )
    {
	if ( bs )
	    addToBS( *bs, partbuf, bufidx, ch );
	if ( !readChar(strm,ch,true) )
	    break;
    }

    if ( ch == '\r' )
    {
	peekChar( strm, ch );
	if ( ch == '\n' )
	    strm.ignore( 1 );
	else
	    ch = '\n';
    }

    if ( nlfound )
	*nlfound = ch == '\n';

    if ( bs && bufidx )
	{ partbuf[bufidx] = '\0'; *bs += partbuf; }

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

    const bool havestrm = sd.iStrm() || sd.oStrm();
    BufferString fnmstr( "'", sd.fileName(), "'" );
    if ( fnmstr == "''" )
	fnmstr.setEmpty();

    if ( !havestrm )
    {
	if ( fnmstr.isEmpty() )
	    msg = uiStrings::phrCannotOpenInpFile();
	else
	    msg = uiStrings::phrCannotOpen( toUiString(fnmstr).quote(true) );
    }
    else if ( sd.streamPtr()->good() )
	msg = od_static_tr( "StrmOpergetErrorMessage",
			    "Successfully opened %1" ).arg( fnmstr );
    else
	msg = getErrorMessage( *sd.streamPtr() );

    if ( !havestrm || !sd.streamPtr()->good() )
	msg.appendPhrase( uiStrings::phrCheckPermissions() );

    return msg;
}


//---- StreamData ----


mStartAllowDeprecatedSection
StreamData::StreamData() { setImpl(new StreamDataImpl); }


StreamData::StreamData( StreamData&& n )
{
    setImpl( n.impl_.set( 0, false ) );
}
mStopAllowDeprecatedSection


StreamData& StreamData::operator=( StreamData&& n )
{
    setImpl( n.impl_.set( 0, false ) );
    return *this;
}

void StreamData::close()
{
    if ( impl_ ) impl_->close();
}


bool StreamData::usable() const
{
    return iStrm() || oStrm();
}


void StreamData::transferTo( StreamData& sd )
{
    sd.impl_ = impl_.set( 0, false );
}


void StreamData::setFileName( const char* fn )
{
    if ( !impl_ )
	return;

    impl_->fname_ = fn;
}


const char* StreamData::fileName() const
{
    if ( !impl_ )
	return 0;

    return impl_->fname_.buf();
}


std::ios* StreamData::streamPtr() const
{
    std::ios* ret = iStrm();
    if ( !ret )
	ret = oStrm();

    return ret;
}

void StreamData::setImpl( StreamDataImpl* n )
{
    impl_ = n;
}


void StreamData::setIStrm( std::istream* strm )
{
    impl_->istrm_ = strm;
}


void StreamData::setOStrm( std::ostream* strm )
{
    impl_->ostrm_ = strm;
}


void StreamData::StreamDataImpl::close()
{
    if ( istrm_ && istrm_ != &std::cin )
	deleteAndZeroPtr( istrm_ );

    if ( ostrm_ )
    {
	ostrm_->flush();
	if ( ostrm_ != &std::cout && ostrm_ != &std::cerr )
	    deleteAndZeroPtr( ostrm_ );
	else
	    ostrm_ = 0;
    }
}
