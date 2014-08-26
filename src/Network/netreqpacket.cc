/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer / Bert
 Date:		Aug 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "netreqpacket.h"
#include "atomic.h"
#include "ptrman.h"


static Threads::Atomic<int> curreqid_;


#define mRequestID_	(header_.int32s_[1])
#define mPayloadSize_	(header_.int32s_[0])
#define mSubID_		(header_.int16s_[4])


Network::RequestPacket::RequestPacket( od_int32 payloadsize )
    : payload_( 0 )
{
    setPayload( 0, payloadsize );
    setSubID( 0 );
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


void Network::RequestPacket::obtainNewRequestID()
{
    setRequestID( ++curreqid_ );
}


od_int32 Network::RequestPacket::payloadSize() const
{ return mPayloadSize_; }


od_int32 Network::RequestPacket::requestID() const
{ return mRequestID_; }


void Network::RequestPacket::setRequestID( od_int32 id )
{ mRequestID_ = id; }


od_int16 Network::RequestPacket::subID() const
{ return mSubID_; }


void Network::RequestPacket::setSubID( od_int16 sid )
{ mSubID_ = sid; }



void Network::RequestPacket::setPayload( void* ptr )
{
    deleteAndZeroArrPtr(payload_);
    payload_ = (char*) ptr;
}


void Network::RequestPacket::setPayload( void* ptr, od_int32 size )
{
    deleteAndZeroArrPtr( payload_ );

    payload_ = (char*) ptr;
    mPayloadSize_ = size;
}


void Network::RequestPacket::setStringPayload( const char* str )
{
    const int sz = FixedString( str ).size();
    if ( sz < 1 )
	setPayload( 0, 0 );
    else
    {
	int newplsz = sz+1;
	mDeclareAndTryAlloc( char*, newpl, char[newplsz] );
	if ( !newpl )
	    newplsz = 0;
	else
	    OD::memCopy( newpl, str, newplsz );

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
    OD::memCopy( cstr, payload_ + sizeof(int), nrchars );
    *(cstr+nrchars) = '\0';
}
