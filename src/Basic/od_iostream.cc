/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "od_strstream.h"

#include "ascstream.h"
#include "bufstring.h"
#include "compoundkey.h"
#include "filepath.h"
#include "filesystemaccess.h"
#include "fixedstreambuf.h"
#include "iopar.h"
#include "perthreadrepos.h"
#include "separstr.h"
#include "strmoper.h"
#include "strmprov.h"
#include "uistrings.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>

const char* od_stream::sStdIO()		{ return StreamProvider::sStdIO(); }
const char* od_stream::sStdErr()	{ return StreamProvider::sStdErr(); }


od_istream& od_istream::nullStream()
{
    mDefineStaticLocalObject( PtrMan<od_istream>, ret, = nullptr );
    if ( !ret )
    {
#ifdef __win__
	auto* newret = new od_istream( "NUL" );
#else
	auto* newret = new od_istream( "/dev/null" );
#endif
	newret->setNoClose();
	ret.setIfNull(newret,true);
    }
    return *ret;
}


od_ostream& od_ostream::nullStream()
{
    mDefineStaticLocalObject( PtrMan<od_ostream>, ret, = nullptr );
    if ( !ret )
    {
#ifdef __win__
	auto* newret = new od_ostream( "NUL" );
#else
	auto* newret = new od_ostream( "/dev/null" );
#endif
	newret->setNoClose();

	ret.setIfNull(newret,true);
    }
    return *ret;
}

namespace OD { extern Export_Basic od_ostream& logMsgStrm(); }

od_ostream& od_ostream::logStream()
{
    return OD::logMsgStrm();
}


static bool isCommand( const char* nm )
{
    if ( !nm || !*nm )
	return false;

    BufferString workstr( nm );
    if ( workstr.isEmpty() )
	return false;

    const char* pwork = workstr.buf();
    bool iscomm = false;
    while ( *pwork == '@' )
	{ iscomm = true; pwork++; }

    return iscomm;
}



od_stream::od_stream()
{
}


od_istream::od_istream( od_istream&& o )
{
    sd_ = std::move( o.sd_ );
}


od_istream& od_istream::operator=( od_istream&& o )
{
    sd_ = std::move( o.sd_ );
    return *this;
}


od_stream::od_stream( const char* fnm, bool forwrite, bool useexist )
{
    if ( isCommand(fnm) )
    {
	pErrMsg( "Deprecated. Set from OS::MachineCommand instead" );
	DBG::forceCrash(false);
	return;
    }

    const OD::FileSystemAccess& fsa = OD::FileSystemAccess::get( fnm );
    if ( forwrite )
	sd_ = fsa.createOStream( fnm, true, useexist );
    else
	sd_ = fsa.createIStream( fnm );
}


od_stream::od_stream( const FilePath& fp, bool forwrite, bool useexist )
    : od_stream( fp.fullPath(), forwrite, useexist )
{}


od_stream::od_stream( const OS::MachineCommand& mc, const char* workdir,
		      bool editmode )
{
    setFromCommand( mc, workdir, editmode );
}


od_stream::od_stream( std::ostream* strm )
{
    sd_.setOStrm( strm );
}


od_stream::od_stream( std::istream* strm )
{
    sd_.setIStrm( strm );
}


od_stream::od_stream( std::ostream& strm )
{
    mine_ = false;
    sd_.setOStrm( &strm );
}


od_stream::od_stream( std::istream& strm )
{
    mine_ = false;
    sd_.setIStrm( &strm );
}


od_stream::~od_stream()
{
    close();
}


void od_stream::close()
{
    if ( mine_ && !noclose_ )
	sd_.close();
}


bool od_stream::isOK() const
{
    if ( forWrite() )
	return sd_.oStrm() && sd_.oStrm()->good();
    else
	return sd_.iStrm() && sd_.iStrm()->good();
}


bool od_stream::isBad() const
{
    if ( forWrite() )
	return !sd_.oStrm() || !sd_.oStrm()->good();
    else
	return !sd_.iStrm() || sd_.iStrm()->bad() || sd_.iStrm()->fail();
}


bool od_istream::atEOF() const
{
    return !sd_.iStrm() || sd_.iStrm()->eof();
}


uiString od_stream::errMsg() const
{
    if ( errmsg_.isEmpty() )
	return StrmOper::getErrorMessage( streamData() );

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


void od_stream::addErrMsgTo( uiRetVal& uirv ) const
{
    uiString msg = errMsg();
    if ( !msg.isEmpty() )
	uirv.add( msg );
}




bool od_stream::setFromCommand( const OS::MachineCommand& mc,
				const char* workdir, bool editmode )
{
    if ( editmode )
	sd_ = StreamProvider::createCmdOStream( mc, workdir );
    else
	sd_ = StreamProvider::createCmdIStream( mc, workdir );

    return editmode ? (bool)sd_.oStrm() : (bool)sd_.iStrm();
}


od_stream::Pos od_stream::position() const
{
    if ( sd_.oStrm() )
	return StrmOper::tell( *sd_.oStrm() );
    else if ( sd_.iStrm() )
	return StrmOper::tell( *sd_.iStrm() );
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
    if ( sd_.iStrm() )
	static_cast<od_istream*>(this)->setReadPosition( pos, ref );
    else if ( sd_.oStrm() )
	static_cast<od_ostream*>(this)->setWritePosition( pos, ref );
}


void od_istream::setReadPosition( od_stream::Pos pos, od_stream::Ref ref )
{
    if ( sd_.iStrm() )
    {
	if ( ref == Abs )
	    StrmOper::seek( *sd_.iStrm(), pos );
	else if ( ref == Rel && pos >= 0 )
	    sd_.iStrm()->ignore( pos );
	else
	    StrmOper::seek( *sd_.iStrm(), pos, getSeekdir(ref) );
    }
}


void od_ostream::setWritePosition( od_stream::Pos pos, od_stream::Ref ref )
{
    if ( sd_.oStrm() )
    {
	if ( ref == Abs )
	    StrmOper::seek( *sd_.oStrm(), pos );
	else
	    StrmOper::seek( *sd_.oStrm(), pos, getSeekdir(ref) );
    }
}


od_stream::Pos od_istream::endPosition() const
{
    const Pos curpos = position();
    od_istream& self = *const_cast<od_istream*>( this );
    self.setReadPosition( 0, End );
    const Pos ret = position();
    self.setReadPosition( curpos, Abs );
    return ret;
}


od_stream::Pos od_ostream::lastWrittenPosition() const
{
    const Pos curpos = position();
    od_ostream& self = *const_cast<od_ostream*>( this );
    self.setWritePosition( 0, End );
    const Pos ret = position();
    self.setWritePosition( curpos, Abs );
    return ret;
}


const char* od_stream::fileName() const
{
    if ( sd_.fileName() )
	return sd_.fileName();
    if ( sd_.iStrm() == &std::cin || sd_.oStrm() == &std::cout )
	return StreamProvider::sStdIO();
    else if ( sd_.oStrm() == &std::cerr )
	return StreamProvider::sStdErr();
    return "";
}


void od_stream::setFileName( const char* fnm )
{
    sd_.setFileName( fnm );
}


bool od_stream::forRead() const
{
    return sd_.iStrm();
}


bool od_stream::forWrite() const
{
    return sd_.oStrm();
}


bool od_stream::isLocal() const
{
    std::streambuf* sb = sd_.iStrm() ? sd_.iStrm()->rdbuf()
		      : (sd_.oStrm() ? sd_.oStrm()->rdbuf() : 0);
    if ( !sb )
	return true;

    mDynamicCastGet(std::filebuf*,sfb,sb)
    mDynamicCastGet(std::stringbuf*,ssb,sb)
    mDynamicCastGet(std::fixedstreambuf*,fsb,sb)
    return sfb || ssb || fsb;
}


BufferString od_stream::noStdStreamPErrMsg() const
{
    BufferString msg( "\nstdStream() requested but none available" );
    msg.add( "\n\tfilename=" ).add( sd_.fileName() );
    if ( sd_.iStrm() )
	msg.add( "\n\tistream available" );
    if ( sd_.oStrm() )
	msg.add( "\n\tostream available" );
    msg.add( "\n\tmine_=" ).add( mine_ );
    msg.add( "\tnoclose_=" ).add( noclose_ );
    msg.add( "\terrmsg_='" ).add( toString(errmsg_) ).add( "'\n" );
    return msg;
}


std::istream& od_istream::stdStream()
{
    if ( sd_.iStrm() )
	return *sd_.iStrm();

    BufferString msg( noStdStreamPErrMsg(), "\nFile " );
    const BufferString fnm( sd_.fileName() );
    if ( fnm.isEmpty() || !File::exists(fnm) )
	msg.add( "does not exist.\n" );
    else
	msg.add( "exists.\n" );
    pErrMsg( msg );
    return nullStream().stdStream();
}


std::ostream& od_ostream::stdStream()
{
    if ( sd_.oStrm() )
	return *sd_.oStrm();
    pErrMsg( noStdStreamPErrMsg() );
    return nullStream().stdStream();
}


od_istream& od_cin()
{
    mDefineStaticLocalObject( PtrMan<od_istream>, ret, = nullptr );
    if ( !ret )
    {
	auto* newret = new od_istream( std::cin );
	newret->setNoClose();
	ret.setIfNull(newret,true);
    }
    return *ret;
}


bool od_istream::open( const char* fnm )
{
    sd_ = OD::FileSystemAccess::get( fnm ).createIStream( fnm );
    return isOK();
}


bool od_istream::reOpen()
{
    if ( noclose_ )
	return true;

    close();
    return open( fileName() );
}


bool od_ostream::open( const char* fnm, bool useexist )
{
    sd_ = OD::FileSystemAccess::get( fnm ).createOStream( fnm, true, useexist );
    return isOK();
}


od_stream* od_stream::create( const char* fnm, bool forread,
			      uiString& errmsg )
{
    od_stream* ret = 0;
    if ( !fnm || !*fnm )
    {
	if ( forread )
	    return new od_istream( od_istream::nullStream().fileName() );
	else
	    return new od_ostream( od_ostream::nullStream().fileName() );
    }

    if ( forread )
	ret = new od_istream( fnm );
    else
	ret = new od_ostream( fnm );

    if ( !ret )
	errmsg = tr("Out of memory");
    else if ( !ret->isOK() )
    {
	errmsg = uiStrings::phrCannotOpen( fnm, forread );
	ret->addErrMsgTo( errmsg );
	delete ret; return 0;
    }

    return ret;
}


void od_ostream::flush()
{
    if ( sd_.oStrm() )
	sd_.oStrm()->flush();
}


od_stream::Count od_istream::lastNrBytesRead() const
{
    if ( sd_.iStrm() )
	return StrmOper::lastNrBytesRead( *sd_.iStrm() );
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

od_ostream& od_ostream::add( const OD::String& ods )
    { return ods.str() ? add( ods.str() ) : *this; }
od_ostream& od_ostream::add( const uiString& ods )
    { return ods.isSet() ? add( ods.getFullString() ) : *this; }


od_istream& od_istream::getC( char* str, int sz, int maxnrch )
{
    if ( maxnrch < 1 )
	maxnrch = sz - 1;
    else if ( sz < (maxnrch+1) )
    {
	pErrMsg("Buffer size should be at least one more than maxnrch");
	return *this;
    }

    stdStream().get( str, maxnrch, '\0' );
    if ( isBad() )
	*str = '\0';

    return *this;
}


bool od_istream::getBin( void* buf, od_stream::Count nrbytes )
{
    if ( nrbytes == 0 || !buf )
	return true;
    return StrmOper::readBlock( stdStream(), buf, nrbytes );
}


od_ostream::od_ostream( od_ostream&& o )
{
    sd_ = std::move( o.sd_ );
}


od_ostream& od_ostream::operator=( od_ostream&& o )
{
    if ( mine_ && !noclose_ )
	sd_.close();
    sd_ = std::move( o.sd_ );
    return *this;
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
    while ( strm.getBin(&c,1) && isOK())
	addBin( &c, 1 );

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
    const std::streamsize maxsize = 0x7FFFFFFFFFFFFFFFLL;

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

mImplStrmAddFn(const MultiID&,t.toString().buf())



// od_istrstream
od_istrstream::od_istrstream( const char* str )
    : od_istream(new std::istringstream(str))
{
}


const char* od_istrstream::input() const
{
    auto& self = cCast( od_istrstream&, *this );
    const auto& strm = sCast( const std::istringstream&, self.stdStream() );
    static std::string str;
    str = strm.str();
    return str.c_str();
}


void od_istrstream::setInput( const char* inp )
{
    auto& self = cCast( od_istrstream&, *this );
    auto& strm = sCast( std::istringstream&, self.stdStream() );
    strm.str( inp ? inp : "" );
    strm.clear();
    errmsg_.setEmpty();
}


// od_ostrstream
od_ostrstream::od_ostrstream()
    : od_ostream(new std::ostringstream)
{
}


const char* od_ostrstream::result() const
{
    auto& self = cCast( od_ostrstream&, *this );
    const auto& strm = sCast( const std::ostringstream&, self.stdStream() );
    static std::string str;
    str = strm.str();
    return str.c_str();
}


void od_ostrstream::setEmpty()
{
    auto& self = cCast( od_ostrstream&, *this );
    auto& strm = sCast( std::ostringstream&, self.stdStream() );
    strm.str( "" );
    strm.clear();
    errmsg_.setEmpty();
}


od_ostream& od_cerr()
{
    mDefineStaticLocalObject( PtrMan<od_ostream>, cerr,
			     (new od_ostream( std::cerr )) );
    return *cerr;
}
