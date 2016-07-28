/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer / Bert
 Date:		Aug 2014
________________________________________________________________________

-*/

#include "netreqpacket.h"
#include "atomic.h"
#include "settings.h"
#include "ptrman.h"


static Threads::Atomic<int> curreqid_;

static const char* sKeySizeLimit = "Network.Limit Packet Size";

od_int32 Network::RequestPacket::systemSizeLimit()
{
    const char* res = Settings::common().find( sKeySizeLimit );
    return toInt( res );
}

void Network::RequestPacket::setSystemSizeLimit( od_int32 val )
{
    const int curval = systemSizeLimit();
    if ( val < 0 ) val = 0;
    if ( curval == val )
	return;

    Settings& setts = Settings::common();
    setts.set( sKeySizeLimit, val );
    setts.write();
}


#define mRequestID_	(header_.int32s_[1])
#define mPayloadSize_	(header_.int32s_[0])
#define mSubID_		(header_.int16s_[4])


Network::RequestPacket::RequestPacket( od_int32 payloadsize )
    : payload_( 0 )
{
    setPayload( allocPayload(payloadsize), payloadsize );
    setSubID( cMoreSubID() );
    setRequestID( -1 );
}


Network::RequestPacket::~RequestPacket()
{ deleteAndZeroArrPtr( payload_ ); }


bool Network::RequestPacket::isOK() const
{
    return payloadSize()>=0 && requestID()>=0;
}


od_int32 Network::RequestPacket::getPayloadSize( const void* buf )
{
    return buf ? ((Header*)buf)->int32s_[0] : 0;
}


int Network::RequestPacket::setIsNewRequest()
{
    const int reqid = ++curreqid_;
    setRequestID( reqid );
    setSubID( cBeginSubID() );
    return reqid;
}


od_int32 Network::RequestPacket::payloadSize() const
{
    return mPayloadSize_;
}


od_int32 Network::RequestPacket::requestID() const
{
    return mRequestID_;
}


void Network::RequestPacket::setRequestID( od_int32 id )
{
    mRequestID_ = id;
}


od_int16 Network::RequestPacket::subID() const
{
    return mSubID_;
}


void Network::RequestPacket::setSubID( od_int16 sid )
{
    mSubID_ = sid;
}


void* Network::RequestPacket::allocPayload( od_int32 size )
{
    if ( size < 1 )
	return 0;

    mDeclareAndTryAlloc( char*, pl, char[size] );
    return pl;
}


void Network::RequestPacket::setPayload( void* ptr, od_int32 size )
{
    deleteAndZeroArrPtr( payload_ );

    payload_ = (char*)ptr;
    mPayloadSize_ = payload_ ? size : 0;
}


void Network::RequestPacket::setStringPayload( const char* str )
{
    const int sz = FixedString( str ).size();
    if ( sz < 1 )
	setPayload( 0, 0 );
    else
    {
	int newplsz = sz + sizeof(int);
	void* newpl = allocPayload( newplsz );
	if ( !newpl )
	    newplsz = 0;
	else
	{
	    *((int*)newpl) = sz;
	    OD::memCopy( ((char*)newpl)+sizeof(int), str, sz );
	}

	setPayload( newpl, newplsz );
    }
}


const void* Network::RequestPacket::payload() const
{
    return payload_;
}


void* Network::RequestPacket::payload( bool takeover )
{
    void* res = payload_;
    if ( takeover )
	payload_ = 0;
    return res;
}


void Network::RequestPacket::getStringPayload( BufferString& str ) const
{
    str.setEmpty();
    if ( !payload_ )
	return;

    const int plsz = mPayloadSize_;
    if ( plsz <= sizeof(int) )
	return;

    int nrchars = *((int*)payload_);
    if ( nrchars > plsz-sizeof(int) )
    {
	pErrMsg("Payload not consistent with string");
	nrchars = plsz-sizeof(int);
    }

    if ( nrchars < 1 )
	return;

    str.setBufSize( nrchars+1 );
    char* cstr = str.getCStr();
    if ( cstr )
    {
	OD::memCopy( cstr, payload_ + sizeof(int), nrchars );
	*(cstr+nrchars) = '\0';
    }
}


void Network::RequestPacket::addErrMsg( BufferString& msg ) const
{
    if ( !isError() )
    {
	pErrMsg( "addErrMsg for non-error packet requested" );
	return;
    }

    BufferString mymsg; getStringPayload( mymsg );
    if ( mymsg.isEmpty() )
    {
	pErrMsg( "No error message in error packet" );
	mymsg = "Unknown network error occurred";
    }
    msg.add( ":\n" ).add( mymsg );

}
