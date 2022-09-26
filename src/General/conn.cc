/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "streamconn.h"
#include "od_iostream.h"


const bool Conn::Read = true;
const bool Conn::Write = false;
const char* StreamConn::sType() { return "Stream"; }
const char* XConn::sType() { return "X-Group"; }


#define mInitList(s,ismine) strm_(s), mine_(ismine)

StreamConn::StreamConn() : mInitList(0,true) {}
StreamConn::StreamConn( od_istream* s ) : mInitList(s,true) { fillCrMsg(s); }
StreamConn::StreamConn( od_ostream* s ) : mInitList(s,true) { fillCrMsg(s); }
StreamConn::StreamConn( od_istream& s ) : mInitList(&s,false) { fillCrMsg(&s); }
StreamConn::StreamConn( od_ostream& s ) : mInitList(&s,false) { fillCrMsg(&s); }
StreamConn::StreamConn( const char* fnm, bool forread ) : mInitList(0,false)
{ setFileName( fnm, forread ); }


Conn::Conn()
{}


Conn::~Conn()
{}


XConn::XConn()
    : conn_(0), mine_(true)
{}


XConn::~XConn()
{
    if ( mine_ )
	delete conn_;
}


StreamConn::~StreamConn()
{
    close();
}


void StreamConn::fillCrMsg( od_stream* strm )
{
    if ( !strm )
	creationmsg_.set( "No stream" );
    else if ( !strm->isOK() )
    {
	creationmsg_.set( "Error for " ).add( strm->fileName() );
	if ( !strm->isBad() )
	    creationmsg_.set( ": empty file" );
	else
	    strm->addErrMsgTo( creationmsg_ );
    }
    else
	creationmsg_.setEmpty();
}


bool StreamConn::isBad() const
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
	{
	    strm_ = &od_istream::nullStream();
	    mine_ = false;
	}
	else
	    strm_ = new od_istream( nm );
    }
    else
    {
	if ( !nm || !*nm )
	{
	    strm_ = &od_ostream::nullStream();
	    mine_ = false;
	}
	else
	    strm_ = new od_ostream( nm );
    }

    fillCrMsg( strm_ );
}


const char* StreamConn::fileName() const
{
    return strm_ ? strm_->fileName() : "";
}
