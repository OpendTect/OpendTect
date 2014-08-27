/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "netreqcommunic.h"

#include "netreqpacket.h"
#include "tcpconnection.h"
#include "ptrman.h"

using namespace Network;


static Threads::Atomic<od_int32> freeid;


RequestCommunicator::RequestCommunicator( const char* servername,
					  short port )
    : tcpsocket_( new TcpConnection )
    , servername_( servername )
    , serverport_( port )
{
    tcpsocket_->connectToHost( servername, port );
}


RequestCommunicator::~RequestCommunicator()
{
    deepErase( receivedpackets_ );
    delete tcpsocket_;
}


od_int32 RequestCommunicator::getNewRequestID()
{
    od_int32 res = freeid++;

    Threads::MutexLocker locker( lock_ );
    activerequestids_ += res;

    return res;
}



bool RequestCommunicator::sendPacket( const Network::RequestPacket& req )
{
    if ( !req.isOK() )
	return false;

    const od_int32 reqid = req.requestID();
    Threads::MutexLocker locker( lock_ );

    if ( !activerequestids_.isPresent( reqid ) )
	return false;

    if ( !tcpsocket_->write( req ) )
    {
	cancelRequestInternal( reqid );
	return false;
    }

    return true;
}


Network::RequestPacket*
Network::RequestCommunicator::pickupPacket(od_int32 reqid, int* errorcode,
					   int timeout )
{
    Threads::MutexLocker locker( lock_ );
    if ( !activerequestids_.isPresent( reqid ) )
    {
	if ( errorcode ) *errorcode = cCancelled();
	return 0;
    }

    bool signalled = false;

    while ( tcpsocket_->anythingToRead() )
    {
	Network::RequestPacket* packet = new Network::RequestPacket;
	if ( tcpsocket_->read( *packet ) )
	{
	    const od_int32 receivedid = packet->requestID();
	    if ( receivedid==reqid )
		return packet;

	    if ( !activerequestids_.isPresent( receivedid ))
	    {
		delete packet;
		continue;
	    }
	    
	    receivedpackets_ += packet;

	    if ( !signalled )
	    {
		lock_.signal( true );
		signalled = true;
	    }
	}
	else
	{
	    tcpsocket_->disconnectFromHost();

	    delete packet;

	    activerequestids_.erase();
	    deepErase( receivedpackets_ );

	    tcpsocket_->connectToHost( servername_, serverport_ );
	}
    }

    while ( true )
    {
	for ( int idx=0; idx<receivedpackets_.size(); idx++ )
	{
	    if ( receivedpackets_[idx]->requestID()==reqid )
	    {
		return receivedpackets_.removeSingle( idx );
	    }
	}

	if ( timeout )
	{
	    lock_.wait( timeout );

	    //We may have had a cancel when we were waiting. Check.
	    if ( !activerequestids_.isPresent( reqid ))
	    {
		if ( !errorcode ) *errorcode = cCancelled();
		return 0;
	    }

	    timeout = 0;
	    if ( errorcode ) *errorcode = cTimeout();
	    continue;
	}

	break;
    }

    return 0;
}


void RequestCommunicator::reportFinished( od_int32 reqid )
{
    Threads::MutexLocker locker( lock_ );
    activerequestids_ -= reqid;

}


void RequestCommunicator::cancelRequest( od_int32 reqid )
{
    Threads::MutexLocker locker( lock_ );

    cancelRequestInternal( reqid );
}


void RequestCommunicator::cancelRequestInternal( od_int32 reqid )
{
    activerequestids_ -= reqid;

    for ( int idx=receivedpackets_.size()-1; idx>=0; idx-- )
    {
	if ( receivedpackets_[idx]->requestID()==reqid )
	    delete receivedpackets_.removeSingle( idx );
    }
}


