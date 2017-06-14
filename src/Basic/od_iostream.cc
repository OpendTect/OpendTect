/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "od_strstream.h"
#include "filepath.h"
#include "strmprov.h"
#include "strmoper.h"
#include "bufstring.h"
#include "fixedstring.h"
#include "separstr.h"
#include "compoundkey.h"
#include "iopar.h"
#include "ascstream.h"
#include "perthreadrepos.h"
#include "uistring.h"
#include <iostream>
#include <sstream>
#include <string.h>

const char* od_stream::sStdIO()		{ return StreamProvider::sStdIO(); }
const char* od_stream::sStdErr()	{ return StreamProvider::sStdErr(); }


od_istream& od_istream::nullStream()
{
    mDefineStaticLocalObject( PtrMan<od_istream>, ret, = 0 );
    if ( !ret )
    {
#ifndef __win__
	od_istream* newret = new od_istream( "/dev/null" );
#else
	od_istream* newret = new od_istream( "NUL:" );
#endif
	newret->setNoClose();

	if ( !ret.setIfNull(newret) )
	    delete newret;
    }
    return *ret;
}

od_ostream& od_ostream::nullStream()
{
    mDefineStaticLocalObject( PtrMan<od_ostream>, ret, = 0 );
    if ( !ret )
    {
#ifndef __win__
	od_ostream* newret = new od_ostream( "/dev/null" );
#else
	od_ostream* newret = new od_ostream( "NUL" );
#endif
	newret->setNoClose();

	if ( !ret.setIfNull(newret) )
	    delete newret;
    }
    return *ret;
}

namespace OD { extern Export_Basic od_ostream& logMsgStrm(); }

od_ostream& od_ostream::logStream()
{
    return OD::logMsgStrm();
}


#define mMkoStrmData(fnm,ex) StreamProvider(fnm).makeOStream(true,ex)
#define mMkiStrmData(fnm) StreamProvider(fnm).makeIStream()
#define mInitList(ismine) sd_(*new StreamData), mine_(ismine), noclose_(false)

od_stream::od_stream()
    : mInitList(false)
{
}

od_stream::od_stream( const od_stream& oth )
    : mInitList(false)
{
    *this = oth;
}

od_stream::od_stream( const char* fnm, bool forwrite, bool useexist )

    : mInitList(true)
{
    sd_ = forwrite ? mMkoStrmData( fnm, useexist ) : mMkiStrmData( fnm );
}


od_stream::od_stream( const FilePath& fp, bool forwrite, bool useexist )
    : mInitList(true)
{
    const BufferString fnm( fp.fullPath() );
    sd_ = forwrite ? mMkoStrmData( fnm, useexist ) : mMkiStrmData( fnm );
}


od_stream::od_stream( std::ostream* strm )
    : mInitList(true)
{
    sd_.ostrm = strm;
}


od_stream::od_stream( std::istream* strm )
    : mInitList(true)
{
    sd_.istrm = strm;
}


od_stream::od_stream( std::ostream& strm )
    : mInitList(false)
{
    sd_.ostrm = &strm;
}


od_stream::od_stream( std::istream& strm )
    : mInitList(false)
{
    sd_.istrm = &strm;
}


od_stream::~od_stream()
{
    close();
    delete &sd_;
}


od_stream& od_stream::operator =( const od_stream& oth )
{
    if ( this != &oth )
    {
	close();
	if ( oth.mine_ && !oth.noclose_ )
	    oth.sd_.transferTo( sd_ );
	else
	    sd_ = oth.sd_;
	mine_ = oth.mine_;
	noclose_ = oth.noclose_;
    }
    return *this;
}


void od_stream::close()
{
    if ( mine_ && !noclose_ )
	sd_.close();
}


bool od_stream::isOK() const
{
    if ( forWrite() )
	return sd_.ostrm && sd_.ostrm->good();
    else
	return sd_.istrm && sd_.istrm->good();
}


bool od_stream::isBad() const
{
    if ( forWrite() )
	return !sd_.ostrm || !sd_.ostrm->good();
    else
	return !sd_.istrm || sd_.istrm->bad() || sd_.istrm->fail();
}


bool od_istream::atEOF() const
{
    return !sd_.istrm || sd_.istrm->eof();
}


uiString od_stream::errMsg() const
{
    if ( errmsg_.isEmpty() )
    {
	const char* sysmsg = StrmOper::getErrorMessage( streamData() );
	return toUiString( sysmsg && *sysmsg ? sysmsg : 0 );
    }

    return errmsg_;
}


void od_stream::addErrMsgTo( BufferString& msg ) const
{
    uiString res = toUiString(msg);
    addErrMsgTo( res );
    msg = res.getFullString();
}


void od_stream::addErrMsgTo( uiString& msg ) const
{
    uiString foundmsg = errMsg();
    if ( !foundmsg.isEmpty() )
    {
	uiString oldmsg = msg;
	msg = toUiString("%1:\n%2").arg(oldmsg).arg(foundmsg);
    }
}


od_stream::Pos od_stream::position() const
{
    if ( sd_.ostrm )
	return StrmOper::tell( *sd_.ostrm );
    else if ( sd_.istrm )
	return StrmOper::tell( *sd_.istrm );
    return -1;
}


static std::ios::seekdir getSeekdir( od_stream::Ref ref )
{
    return ref == od_stream::Rel ? std::ios::cur
	: (ref == od_stream::Abs ? std::ios::beg
				 : std::ios::end);
}


void od_stream::setPosition( od_stream::Pos pos, od_stream::Ref ref )
{
    if ( sd_.ostrm )
    {
	if ( ref == Abs )
	    StrmOper::seek( *sd_.ostrm, pos );
	else
	    StrmOper::seek( *sd_.ostrm, pos, getSeekdir(ref) );
    }
    else if ( sd_.istrm )
    {
	if ( ref == Abs )
	    StrmOper::seek( *sd_.istrm, pos );
	else
	    StrmOper::seek( *sd_.istrm, pos, getSeekdir(ref) );
    }
}


const char* od_stream::fileName() const
{
    if ( sd_.fileName() )
	return sd_.fileName();
    if ( sd_.istrm == &std::cin || sd_.ostrm == &std::cout )
	return StreamProvider::sStdIO();
    else if ( sd_.ostrm == &std::cerr )
	return StreamProvider::sStdErr();
    return "";
}


void od_stream::setFileName( const char* fnm )
{
    sd_.setFileName( fnm );
}


od_stream::Pos od_stream::endPosition() const
{
    const Pos curpos = position();
    od_stream& self = *const_cast<od_stream*>( this );
    self.setPosition( 0, End );
    const Pos ret = position();
    self.setPosition( curpos, Abs );
    return ret;
}


bool od_stream::forRead() const
{
    return sd_.istrm;
}


bool od_stream::forWrite() const
{
    return sd_.ostrm;
}


void od_stream::releaseStream( StreamData& out )
{
    sd_.transferTo( out );
}


std::istream& od_istream::stdStream()
{
    if ( sd_.istrm )
	return *sd_.istrm;
    pErrMsg( "stdStream() requested but none available" );
    return nullStream().stdStream();
}


std::ostream& od_ostream::stdStream()
{
    if ( sd_.ostrm )
	return *sd_.ostrm;
    pErrMsg( "stdStream() requested but none available" );
    return nullStream().stdStream();
}


bool od_istream::open( const char* fnm )
{
    od_istream strm( fnm );
    if ( strm.isOK() )
	{ *this = strm; return true; }
    else
	{ close(); setFileName(fnm); return false; }
}


bool od_istream::reOpen()
{
    return noclose_ ? true : open( fileName() );
}


bool od_ostream::open( const char* fnm, bool useexist )
{
    od_ostream strm( fnm, useexist );
    if ( strm.isOK() )
	{ *this = strm; return true; }
    else
	{ close(); setFileName(fnm); return false; }
}

od_stream* od_stream::create( const char* fnm, bool forread,
			     BufferString& errmsg )
{
    uiString uimsg;
    od_stream* res = create( fnm, forread, uimsg );
    if ( !res )
    {
	errmsg = uimsg.getFullString();
    }

    return res;
}


od_stream* od_stream::create( const char* fnm, bool forread,
			      uiString& errmsg )
{
    od_stream* ret = 0;
    if ( forread )
    {
	if ( !fnm || !*fnm )
	    return new od_istream( od_istream::nullStream() );

	ret = new od_istream( fnm );
	if ( !ret )
	    errmsg = tr("Out of memory");
	else if ( !ret->isOK() )
	{
	    errmsg = tr( "Cannot open %1 for read" ).arg( fnm );
	    ret->addErrMsgTo( errmsg );
	    delete ret; return 0;
	}
    }
    else
    {
	if ( !fnm || !*fnm )
	    return new od_ostream( od_ostream::nullStream() );

	ret = new od_ostream( fnm );
	if ( !ret )
	    errmsg = tr("Out of memory");
	else if ( !ret->isOK() )
	{
	    errmsg = tr( "Cannot open %1 for write" ).arg( fnm );
	    ret->addErrMsgTo( errmsg );
	    delete ret; return 0;
	}
    }

    return ret;
}


void od_ostream::flush()
{
    if ( sd_.ostrm )
	sd_.ostrm->flush();
}


od_stream::Count od_istream::lastNrBytesRead() const
{
    if ( sd_.istrm )
	return mCast(od_stream::Count,StrmOper::lastNrBytesRead(*sd_.istrm));
    return 0;
}


static void fillNumberFmtErrMsg( od_istream& strm, uiString& errmsg )
{
    std::istream& stdstrm = strm.stdStream();
    if ( !stdstrm.fail() )
	errmsg.setEmpty();
    else
    {
	stdstrm.clear(); BufferString word;
	StrmOper::readWord( stdstrm, true, &word );
	stdstrm.clear(std::ios::badbit);
	errmsg = toUiString("Invalid number found: '%1'").arg(word);
    }
}


#define mNumberNotPresentErrRet() \
	{ errmsg.set("Number not present on last line"); \
	    stdstrm.setstate(std::ios::failbit); return false; }

template <class T>
static bool getNumberWithRetry( od_istream& strm, T& t, uiString& errmsg )
{
    std::istream& stdstrm = strm.stdStream();
    t = (T)0;

    int retrycount = 0;
    if ( !stdstrm.good() )
	return false;
    if ( !StrmOper::skipWhiteSpace(stdstrm) )
	mNumberNotPresentErrRet()
    while ( true )
    {
	if ( stdstrm.eof() )
	    mNumberNotPresentErrRet()
	stdstrm >> t;
	if ( stdstrm.good() || stdstrm.eof() )
	    break;
	else if ( stdstrm.fail() )
	    { fillNumberFmtErrMsg(strm,errmsg); return false; }
	else if ( !StrmOper::resetSoftError(stdstrm,retrycount) )
	    return false;
    }
    return true;
}


#define mGetWithRetry(stmts,rv) \
int retrycount = 0; \
std::istream& strm = stdStream(); \
while ( true ) \
{ \
    if ( !strm.good() ) \
	return rv; \
    { stmts; } \
    if ( strm.good() || strm.eof() ) \
	return rv; \
    else if ( !StrmOper::resetSoftError(strm,retrycount) ) \
	return rv; \
} \
return rv


#define mAddWithRetry(stmts,rv) \
int retrycount = 0; \
std::ostream& strm = stdStream(); \
while ( true ) \
{ \
    { stmts; } \
    if ( strm.good() || !StrmOper::resetSoftError(strm,retrycount) ) \
	return rv; \
} \
return rv


#define mImplStrmAddFn(typ,tostrm) \
od_ostream& od_ostream::add( typ t ) \
{ \
    mAddWithRetry( strm << tostrm, *this ); \
}

#define mImplStrmAddPreciseFn(typ,tostrm) \
od_ostream& od_ostream::addPrecise( typ t ) \
{ \
    mAddWithRetry( strm << tostrm, *this ); \
}

#define mImplNumberGetFn(typ) \
od_istream& od_istream::get( typ& t ) \
{ getNumberWithRetry(*this,t,errmsg_); return *this; }

#define mImplSimpleAddGetFnsNoConv(typ) \
    mImplStrmAddFn(typ,t) mImplNumberGetFn(typ)
#define mImplSimpleAddGetFns(typ) \
    mImplStrmAddFn(typ,toString(t)) mImplNumberGetFn(typ)
#define mImplPreciseAddFn(typ) \
    mImplStrmAddPreciseFn(typ,toStringPrecise(t))

mImplSimpleAddGetFnsNoConv(char)
mImplSimpleAddGetFnsNoConv(unsigned char)
mImplSimpleAddGetFns(od_int16)
mImplSimpleAddGetFns(od_uint16)
mImplSimpleAddGetFns(od_int32)
mImplSimpleAddGetFns(od_uint32)
mImplSimpleAddGetFns(od_int64)
mImplSimpleAddGetFns(od_uint64)

#ifdef __lux64__
mImplSimpleAddGetFnsNoConv(long long)
mImplSimpleAddGetFnsNoConv(unsigned long long)
#else
mImplSimpleAddGetFnsNoConv(long)
mImplSimpleAddGetFnsNoConv(unsigned long)
#endif

mImplSimpleAddGetFns(float)
mImplSimpleAddGetFns(double)
mImplPreciseAddFn(float)
mImplPreciseAddFn(double)

mImplStrmAddFn(const char*,t)
od_istream& od_istream::get( char* str )
    { pErrMsg("Dangerous: od_istream::get(char*)"); return getC( str, 0 ); }

od_ostream& od_ostream::add( const OD::String& ods )
    { return ods.str() ? add( ods.str() ) : *this; }
od_ostream& od_ostream::add( const uiString& ods )
    { return ods.isSet() ? add( ods.getFullString() ) : *this; }
od_istream& od_istream::get( FixedString& fs )
    { pErrMsg("od_istream::get(FixedString&) called"); return *this; }
od_istream& od_istream::get( void* ptr )
    { pErrMsg("od_istream::get(void*) called"); return *this; }


od_istream& od_istream::getC( char* str, int maxnrch )
{
    if ( str )
    {
	*str = '\0';
	if ( isOK() )
	{
	    BufferString bs; get( bs );
	    if ( isBad() )
		*str = '\0';
	    else if ( maxnrch > 0 )
		strncpy( str, bs.buf(), maxnrch );
	    else
		strcpy( str, bs.buf() ); // still dangerous, but intentional
	}
    }
    return *this;
}


bool od_istream::getBin( void* buf, od_stream::Count nrbytes )
{
    if ( nrbytes == 0 || !buf )
	return true;
    return StrmOper::readBlock( stdStream(), buf, nrbytes );
}


bool od_ostream::addBin( const void* buf, od_stream::Count nrbytes )
{
    return nrbytes <= 0 || !buf ? true
	: StrmOper::writeBlock( stdStream(), buf, nrbytes );
}


od_ostream& od_ostream::add( const void* ptr )
{
    pErrMsg( "od_ostream::add(void*) called. If intentional, use addPtr" );
    return addPtr( ptr );
}


od_ostream& od_ostream::addPtr( const void* ptr )
{
    if ( ptr )
	{ mAddWithRetry( strm << ((const int*)ptr), *this ); }
    else
	{ mAddWithRetry( strm << "(null)", *this ); }
}


od_ostream& od_ostream::add( od_istream& strm )
{
    char c;
    while ( isOK() && strm.isOK() )
    {
	strm.get( c );
	add( c );
    }
    return *this;
}


bool od_istream::getWord( BufferString& bs, bool allownl )
{
    return StrmOper::readWord( stdStream(), allownl, &bs );
}


bool od_istream::getLine( BufferString& bs, bool* newline_found )
{
    return StrmOper::readLine( stdStream(), &bs, newline_found );
}


bool od_istream::getAll( BufferString& bs )
{
    return StrmOper::readFile( stdStream(), bs );
}


char od_istream::peek() const
{
    char ch;
    return StrmOper::peekChar( const_cast<od_istream*>(this)->stdStream(), ch )
			? ch : '\0';
}


void od_istream::ignore( od_stream::Count nrbytes )
{
    mGetWithRetry(
	stdStream().ignore( (std::streamsize)nrbytes )
    , );
}


bool od_istream::skipUntil( char tofind )
{
#ifdef __lux32__
    const std::streamsize maxsize = 0x7FFFFFF;
#else
    const std::streamsize maxsize = 0x7FFFFFFFFFFFFFFFLL;
#endif

    mGetWithRetry(
	stdStream().ignore( maxsize, tofind )
    , isOK() );
}


bool od_istream::skipWord()
{
    StrmOper::readWord( stdStream(), true );
    return isOK();
}


bool od_istream::skipLine()
{
    StrmOper::readLine( stdStream() );
    return isOK();
}


od_istream& od_istream::get( IOPar& iop )
{
    ascistream astrm( *this, false );
    mGetWithRetry( iop.getFrom( astrm ), *this );
}

od_ostream& od_ostream::add( const IOPar& iop )
{
    ascostream astrm( *this );
    mAddWithRetry( iop.putTo( astrm ), *this );
}


mImplStrmAddFn(const SeparString&,t.buf())
od_istream& od_istream::get( SeparString& ss )
{
    BufferString bs; get( bs ); ss = bs.buf();
    return *this;
}
mImplStrmAddFn(const CompoundKey&,t.buf())
od_istream& od_istream::get( CompoundKey& ck )
{
    BufferString bs; get( bs ); ck = bs.buf();
    return *this;
}


od_istrstream::od_istrstream( const char* str )
    : od_istream(new std::istringstream(str))
{
}


#define mGetStdStream(clss,stdclss,cnst,var) \
    clss& self = const_cast<clss&>( *this ); \
    cnst std::stdclss& var = static_cast<cnst std::stdclss&>( self.stdStream() )


const char* od_istrstream::input() const
{
    mGetStdStream(od_istrstream,istringstream,const,stdstrstrm);
    return stdstrstrm.str().c_str();
}


void od_istrstream::setInput( const char* inp )
{
    mGetStdStream(od_istrstream,istringstream,,stdstrstrm);
    stdstrstrm.str( inp ? inp : "" );
    stdstrstrm.clear();
    errmsg_.setEmpty();
}


od_ostrstream::od_ostrstream()
    : od_ostream(new std::ostringstream)
{
}


const char* od_ostrstream::result() const
{
    mGetStdStream(od_ostrstream,ostringstream,const,stdstrstrm);
    mDeclStaticString( ret );
    ret = stdstrstrm.str().c_str();
    return ret.buf();
}


void od_ostrstream::setEmpty()
{
    mGetStdStream(od_ostrstream,ostringstream,,stdstrstrm);
    stdstrstrm.str( "" );
    stdstrstrm.clear();
    errmsg_.setEmpty();
}

od_ostream& od_cerr()
{
    mDefineStaticLocalObject( PtrMan<od_ostream>, cout,
			     (new od_ostream( std::cerr )) );
    return *cout;
}
