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
#include <iostream>
#include <sstream>
#include <string.h>


od_istream& od_istream::nullStream()
{
    static od_istream* ret = 0;
    if ( !ret )
    {
#ifndef __win__
	ret = new od_istream( "/dev/null" );
#else
	ret = new od_istream( "NUL:" );
#endif
	ret->setNoClose();
    }
    return *ret;
}

od_ostream& od_ostream::nullStream()
{
    static od_ostream* ret = 0;
    if ( !ret )
    {
#ifndef __win__
	ret = new od_ostream( "/dev/null" );
#else
	ret = new od_ostream( "NUL" );
#endif
	ret->setNoClose();
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
	return !sd_.istrm || sd_.istrm->bad();
}


const char* od_stream::errMsg() const
{
    return StrmOper::getErrorMessage( streamData() );
}


void od_stream::addErrMsgTo( BufferString& msg ) const
{
    const char* mymsg = errMsg();
    if ( mymsg && *mymsg )
	msg.add( ":\n" ).add( mymsg );
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
    od_stream* ret = 0;
    if ( forread )
    {
	if ( !fnm || !*fnm )
	    return &od_istream::nullStream();

	ret = new od_istream( fnm );
	if ( !ret )
	    errmsg = "Out of memory";
	else if ( !ret->isOK() )
	{
	    errmsg.set( "Cannot open " ).add( fnm ).add( " for read" );
	    ret->addErrMsgTo( errmsg );
	    delete ret; return 0;
	}
    }
    else
    {
	if ( !fnm || !*fnm )
	    return &od_ostream::nullStream();

	ret = new od_ostream( fnm );
	if ( !ret )
	    errmsg = "Out of memory";
	else if ( !ret->isOK() )
	{
	    errmsg.set( "Cannot open " ).add( fnm ).add( " for write" );
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


#define mGetWithRetry(stmts,rv) \
int retrycount = 0; \
std::istream& strm = stdStream(); \
while ( true ) \
{ \
    { stmts; } \
    if ( strm.eof() || !strm.fail() ) \
	return rv; \
    if ( !StrmOper::resetSoftError(stdStream(),retrycount) ) \
	return rv; \
} \
return rv;


#define mAddWithRetry(stmts,rv) \
int retrycount = 0; \
std::ostream& strm = stdStream(); \
while ( true ) \
{ \
    { stmts; } \
    if ( !strm.fail() || !StrmOper::resetSoftError(stdStream(),retrycount) ) \
	return rv; \
} \
return rv;


#define mImplStrmAddFn(typ,tostrm) \
od_ostream& od_ostream::add( typ t ) \
{ \
    mAddWithRetry( strm << tostrm, *this ) \
}

#define mImplStrmGetFn(typ,tostrm) \
od_istream& od_istream::get( typ& t ) \
{ \
    mGetWithRetry( strm >> tostrm, *this ) \
}

#define mImplSimpleAddFn(typ) mImplStrmAddFn(typ,t)
#define mImplSimpleGetFn(typ) mImplStrmGetFn(typ,t)
#define mImplSimpleAddGetFns(typ) mImplSimpleAddFn(typ) mImplSimpleGetFn(typ)

mImplSimpleAddGetFns(char)
mImplSimpleAddGetFns(unsigned char)
mImplSimpleAddGetFns(od_int16)
mImplSimpleAddGetFns(od_uint16)
mImplSimpleAddGetFns(od_int32)
mImplSimpleAddGetFns(od_uint32)
mImplSimpleAddGetFns(od_int64)
mImplSimpleAddGetFns(od_uint64)


mImplStrmAddFn(float,toString(t))
mImplSimpleGetFn(float)
mImplStrmAddFn(double,toString(t))
mImplSimpleGetFn(double)


mImplSimpleAddFn(const char*)
od_istream& od_istream::get( char* str )
    { pErrMsg("Dangerous: od_istream::get(char*)"); return getC( str, 0 ); }

mImplStrmAddFn(const BufferString&,t.buf())
od_istream& od_istream::get( BufferString& bs, bool allownl )
{
    if ( allownl )
	StrmOper::readWord( stdStream(), &bs );
    else
	StrmOper::wordFromLine( stdStream(), bs );
    return *this;
}

od_ostream& od_ostream::add( const FixedString& fs )
    { return fs.str() ? add( fs.str() ) : *this; }
od_istream& od_istream::get( FixedString& fs )
    { pErrMsg("od_istream::get(FixedString&) called"); return *this; }
od_istream& od_istream::get( void* ptr )
    { pErrMsg("od_istream::get(void*) called"); return *this; }


od_istream& od_istream::getC( char* str, int maxnrch )
{
    if ( str )
    {
	BufferString bs; get( bs );
	if ( isBad() )
	    *str = '\0';
	else if ( maxnrch > 0 )
	    strncpy( str, bs.buf(), maxnrch );
	else
	    strcpy( str, bs.buf() ); // still dangerous, but intentional
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
	{ mAddWithRetry( strm << ((const int*)ptr), *this ) }
    else
	{ mAddWithRetry( strm << "(null)", *this ) }
}


bool od_istream::getLine( BufferString& bs )
{
    return StrmOper::readLine( stdStream(), &bs );
}


bool od_istream::getAll( BufferString& bs )
{
    return StrmOper::readFile( stdStream(), bs );
}


char od_istream::peek() const
{
    return (char)(const_cast<od_istream*>(this)->stdStream()).peek();
}


void od_istream::ignore( od_stream::Count nrbytes )
{
    mGetWithRetry(
	stdStream().ignore( (std::streamsize)nrbytes )
    , )
}


bool od_istream::skipUntil( char tofind )
{
    mGetWithRetry(
	stdStream().ignore( 9223372036854775807LL, tofind )
    , isOK() )
}


bool od_istream::skipWord()
{
    StrmOper::readWord( stdStream() );
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
    mGetWithRetry( iop.getFrom( astrm ), *this )
}

od_ostream& od_ostream::add( const IOPar& iop )
{
    ascostream astrm( *this );
    mAddWithRetry( iop.putTo( astrm ), *this )
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
}


od_ostrstream::od_ostrstream()
    : od_ostream(new std::ostringstream)
{
}


const char* od_ostrstream::result() const
{
    mGetStdStream(od_ostrstream,ostringstream,const,stdstrstrm);
    return stdstrstrm.str().c_str();
}


void od_ostrstream::setEmpty()
{
    mGetStdStream(od_ostrstream,ostringstream,,stdstrstrm);
    stdstrstrm.str( "" );
    stdstrstrm.clear();
}
