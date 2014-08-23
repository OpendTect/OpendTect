/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "netreqpacket.h"

#include "ptrman.h"


using namespace Network;

#define mRequestID_	(header_.int32s_[1])
#define mPayloadSize_	(header_.int32s_[0])
#define mSubID_		(header_.int16s_[4])

RequestPacket::RequestPacket( od_int32 payloadsize )
    : payload_( 0 )
{
    setPayload( 0, payloadsize );
    setSubID( 0 );
    setRequestID( -1 );
}


RequestPacket::~RequestPacket()
{ deleteAndZeroArrPtr( payload_ ); }


bool RequestPacket::isOK() const
{
    return getPayloadSize()>=0 && getRequestID()>=0;
}

od_int32 RequestPacket::getPayloadSize() const
{ return mPayloadSize_; }


void RequestPacket::setPayload( void* ptr )
{
    deleteAndZeroArrPtr(payload_);
    payload_ = (char*) ptr;
}


od_int32 RequestPacket::getRequestID() const
{ return mRequestID_; }


void RequestPacket::setRequestID( od_int32 id )
{ mRequestID_ = id; }


od_int16 RequestPacket::getSubID() const
{ return mSubID_; }


void RequestPacket::setSubID( od_int16 sid )
{ mSubID_ = sid; }




void RequestPacket::setStringPayload( const char* str )
{
    const int sz = strlen( str );
    if ( sz )
    {
        int payloadsz = sz+1;
        mDeclareAndTryAlloc( char*, payload, char[payloadsz] );
        if ( payload )
        {
            memcpy( payload, str, payloadsz );
        }
        else
        {
            payloadsz = 0;
        }

        setPayload( payload, payloadsz );
    }
    else
    {
        setPayload( 0, 0 );
    }
}


void RequestPacket::setPayload( void* ptr, od_int32 size )
{
    deleteAndZeroArrPtr( payload_ );

    payload_ = (char*) ptr;
    mPayloadSize_ = size;
}



void* RequestPacket::getPayload( bool takeover )
{
    void* res = payload_;

    if ( takeover )
        payload_ = 0;

    return res;
}

