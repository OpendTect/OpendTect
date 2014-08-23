/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "netreqpacket.h"


bool Network::RequestPacket::isOK() const
{
    return getPayloadSize()>=0 && getRequestID()>=0;
}

od_int32 Network::RequestPacket::getPayloadSize() const
{
    return header_.int32s_[0];
}


od_int32 Network::RequestPacket::getRequestID() const
{
    return header_.int32s_[1];
}


void Network::RequestPacket::setPayload( void* ptr )
{
    delete payload_;
    payload_ = (char*) ptr;
}






