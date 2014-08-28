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
#include "timefun.h"

using namespace Network;


RequestCommunicator::RequestCommunicator( const char* servername, int port )
    : tcpconn_( new TcpConnection )
    , servername_( servername )
    , serverport_( port )
    , connectionClosed( this )
{
    tcpconn_->connectToHost( servername, port );
    mAttachCB(tcpconn_->Closed,RequestCommunicator::connCloseCB);
}


RequestCommunicator::~RequestCommunicator()
{
    deepErase( receivedpackets_ );
    delete tcpconn_;
}


bool RequestCommunicator::isOK() const
{
    return tcpconn_ && !tcpconn_->isBad();
}


void RequestCommunicator::connCloseCB( CallBacker* )
{
    connectionClosed.trigger();
}


bool RequestCommunicator::sendPacket( const RequestPacket& req )
{
    if ( !req.isOK() )
	return false;

    const od_int32 reqid = req.requestID();
    const od_int16 subid = req.subID();

    Threads::Locker locker( lock_ );
    if ( !ourrequestids_.isPresent( reqid ) )
    {
	if ( subid == RequestPacket::cBeginSubID() )
	    ourrequestids_ += reqid;
	else
	    return false;
    }
    locker.unlockNow();

    bool ret = tcpconn_->write(req);
    if ( !ret || subid == RequestPacket::cEndSubID() )
	requestEnded( reqid );

    return ret;
}


RequestPacket* RequestCommunicator::pickupPacket( od_int32 reqid, int timeout,
						  int* errorcode )
{
    Threads::Locker locker( lock_ );
    const int idxof = ourrequestids_.indexOf( reqid );
    if ( idxof < 0 )
    {
	if ( errorcode ) *errorcode = cInvalidRequest();
	return 0;
    }

    RequestPacket* pkt = getNextAlreadyRead( reqid );
    if ( !pkt )
    {
	const int starttm = Time::getMilliSeconds();
	while ( Time::getMilliSeconds()-starttm < timeout )
	{
	    pkt = readConnection(reqid);
	    if ( pkt )
		break;
	    Threads::sleep( 0.01 );
	}

	if ( !pkt && errorcode )
	    *errorcode = cTimeout();
    }

    if ( pkt && pkt->subID() == RequestPacket::cEndSubID() )
	requestEnded( reqid );

    return pkt;
}


RequestPacket* RequestCommunicator::readConnection( int targetreqid )
{
    Threads::Locker locker( lock_ );

    while ( tcpconn_->anythingToRead() )
    {
	RequestPacket* packet = new RequestPacket;
	if ( tcpconn_->read( *packet ) )
	{
	    const od_int32 receivedid = packet->requestID();
	    if ( receivedid==targetreqid )
		return packet;

	    if ( targetreqid > 0 && !ourrequestids_.isPresent( receivedid ))
	    {
		delete packet;
		continue;
	    }

	    receivedpackets_ += packet;
	}
	else
	{
	    tcpconn_->disconnectFromHost();

	    delete packet;

	    ourrequestids_.erase();
	    deepErase( receivedpackets_ );

	    tcpconn_->connectToHost( servername_, serverport_ );
	}
    }

    return 0;
}



RequestPacket* RequestCommunicator::getNextAlreadyRead( int reqid )
{
    Threads::Locker locker( lock_ );
    for ( int idx=0; idx<receivedpackets_.size(); idx++ )
    {
	if ( reqid < 0 || receivedpackets_[idx]->requestID()==reqid )
	    return receivedpackets_.removeSingle( idx );
    }

    return 0;
}


RequestPacket* RequestCommunicator::getNextExternalPacket()
{
    Threads::Locker locker( lock_ );
    readConnection( -1 );
    for ( int idx=0; idx<receivedpackets_.size(); idx++ )
    {
	RequestPacket* pkt = receivedpackets_[idx];
	if ( !ourrequestids_.isPresent(pkt->requestID()) )
	    return receivedpackets_.removeSingle(idx);
    }
    return 0;
}


void RequestCommunicator::requestEnded( od_int32 reqid )
{
    Threads::Locker locker( lock_ );
    ourrequestids_ -= reqid;

    for ( int idx=receivedpackets_.size()-1; idx>=0; idx-- )
    {
	if ( receivedpackets_[idx]->requestID()==reqid )
	    delete receivedpackets_.removeSingle( idx );
    }
}
