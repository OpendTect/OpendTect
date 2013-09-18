/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "od_istream.h"
#include "od_ostream.h"
#include "filepath.h"
#include "strmprov.h"
#include "strmoper.h"
#include "bufstring.h"
#include "fixedstring.h"
#include "separstr.h"
#include "compoundkey.h"
#include "iopar.h"
#include <iostream>
#include <string.h>


#define mMkoStrmData(fnm) StreamProvider(fnm).makeOStream()
#define mMkiStrmData(fnm) StreamProvider(fnm).makeIStream()
#define mInitList(ismine) sd_(*new StreamData), mine_(ismine)

od_stream::od_stream( const char* fnm, bool forwrite )
    : mInitList(true)
{
    sd_ = forwrite ? mMkoStrmData( fnm ) : mMkiStrmData( fnm );
}


od_stream::od_stream( const FilePath& fp, bool forwrite )
    : mInitList(true)
{
    const BufferString fnm( fp.fullPath() );
    sd_ = forwrite ? mMkoStrmData( fnm ) : mMkiStrmData( fnm );
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


// all private, setting to empty
od_stream::od_stream(const od_stream&) : mInitList(false) {}
od_stream& od_stream::operator =(const od_stream&) { close(); return *this; }
od_istream& od_istream::operator =(const od_istream&) { close(); return *this; }
od_ostream& od_ostream::operator =(const od_ostream&) { close(); return *this; }


od_stream::~od_stream()
{
    close();
    delete &sd_;
}


void od_stream::close()
{
    if ( mine_ )
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


od_stream::Count od_stream::position() const
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
	: (ref == od_stream::Beg ? std::ios::beg
				 : std::ios::end);
}


void od_stream::setPosition( od_stream::Count offs, od_stream::Ref ref )
{
    if ( sd_.ostrm )
    {
	if ( ref == Abs )
	    StrmOper::seek( *sd_.ostrm, offs );
	else
	    StrmOper::seek( *sd_.ostrm, offs, getSeekdir(ref) );
    }
    else if ( sd_.istrm )
    {
	if ( ref == Abs )
	    StrmOper::seek( *sd_.istrm, offs );
	else
	    StrmOper::seek( *sd_.istrm, offs, getSeekdir(ref) );
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


od_int64 od_stream::endPosition() const
{
    const Count curpos = position();
    od_stream& self = *const_cast<od_stream*>( this );
    self.setPosition( 0, End );
    const Count ret = position();
    self.setPosition( curpos, Abs );
    return ret;
}


bool od_stream::forWrite() const
{
    return sd_.ostrm;
}


void od_stream::releaseStream( StreamData& out )
{
    sd_.transferTo( out );
}


std::ostream& od_ostream::stdStream()
{
    return sd_.ostrm ? *sd_.ostrm : std::cerr;
}


std::istream& od_istream::stdStream()
{
    return sd_.istrm ? *sd_.istrm : std::cin;
}


#define mImplStrmAddFn(typ,tostrm) \
od_ostream& od_ostream::add( typ t ) \
{ \
    stdStream() << tostrm; return *this; \
}

#define mImplStrmGetFn(typ,tostrm) \
od_istream& od_istream::get( typ& t ) \
{ \
    stdStream() >> tostrm; return *this; \
}

#define mImplSimpleAddFn(typ) mImplStrmAddFn(typ,t)
#define mImplSimpleGetFn(typ) mImplStrmGetFn(typ,t)
#define mImplSimpleAddGetFns(typ) mImplSimpleAddFn(typ) mImplSimpleGetFn(typ)

mImplSimpleAddGetFns(od_int16)
mImplSimpleAddGetFns(od_uint16)
mImplSimpleAddGetFns(od_int32)
mImplSimpleAddGetFns(od_uint32)
mImplSimpleAddGetFns(od_int64)
mImplSimpleAddGetFns(od_uint64)
mImplSimpleAddGetFns(float)
mImplSimpleAddGetFns(double)


mImplSimpleAddFn(const char*)
od_istream& od_istream::get( char* str )
    { pErrMsg("Dangerous: od_istream::get(char*)"); return getC( str, 0 ); }

mImplStrmAddFn(const BufferString&,t.buf())
od_istream& od_istream::get( BufferString& bs )
{
    StrmOper::readWord( stdStream(), &bs );
    return *this;
}

od_ostream& od_ostream::add( const FixedString& fs )
    { return fs.str() ? add( fs.str() ) : *this; }
od_istream& od_istream::get( FixedString& fs )
    { pErrMsg("od_istream::get(FixedString&) called"); return *this; }


od_istream& od_istream::getC( char* str, int maxnrch )
{
    if ( str )
    {
	BufferString bs; get( bs );
	if ( maxnrch > 0 )
	    strncpy( str, bs.buf(), maxnrch );
	else
	    strcpy( str, bs.buf() ); // still dangerous, but intentional
    }
    return *this;
}


od_stream::Count od_istream::lastNrBytesRead() const
{
    if ( sd_.istrm )
	return mCast(od_stream::Count,StrmOper::lastNrBytesRead(*sd_.istrm));
    return 0;
}
