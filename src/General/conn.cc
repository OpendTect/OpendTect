/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Connections
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "streamconn.h"
#include "od_iostream.h"


const bool Conn::Read = true;
const bool Conn::Write = false;
const char* StreamConn::sType() { return "Stream"; }
const char* XConn::sType() { return "X-Group"; }


#define mInitList(s,ismine) strm_(s), mine_(ismine)

StreamConn::StreamConn() : mInitList(0,true) {}
StreamConn::StreamConn( od_istream* s ) : mInitList(s,true) {}
StreamConn::StreamConn( od_ostream* s ) : mInitList(s,true) {}
StreamConn::StreamConn( od_istream& s ) : mInitList(&s,false) {}
StreamConn::StreamConn( od_ostream& s ) : mInitList(&s,false) {}
StreamConn::StreamConn( const char* fnm, bool forread ) : mInitList(0,false)
{ setFileName( fnm, forread ); }


StreamConn::~StreamConn()
{
    close();
}


bool StreamConn::bad() const
{
    return !strm_ || strm_->isBad();
}


bool StreamConn::forRead() const
{
    return strm_ && strm_->forRead();
}


bool StreamConn::forWrite() const
{
    return strm_ && strm_->forWrite();
}


void StreamConn::close()
{
    if ( strm_ && mine_ )
	{ delete strm_; strm_ = 0; }
}


od_stream& StreamConn::odStream()
{
    if ( strm_ )
	return *strm_;
    return od_istream::nullStream();
}


od_istream& StreamConn::iStream()
{
    return forRead() ? *static_cast<od_istream*>(strm_)
		     : od_istream::nullStream();
}


od_ostream& StreamConn::oStream()
{
    return forWrite() ? *static_cast<od_ostream*>(strm_)
		      : od_ostream::nullStream();
}


void StreamConn::setFileName( const char* nm, bool forread )
{
    close();

    mine_ = true;
    if ( forread )
    {
	if ( !nm || !*nm )
	    strm_ = &od_istream::nullStream();
	else
	    strm_ = new od_istream( nm );
    }
    else
    {
	if ( !nm || !*nm )
	    strm_ = &od_ostream::nullStream();
	else
	    strm_ = new od_ostream( nm );
    }
}


const char* StreamConn::fileName() const
{
    return strm_ ? strm_->fileName() : "";
}
