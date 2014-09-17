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
#include "tcpsocket.h"
#include "tcpserver.h"
#include "timefun.h"
#include "ptrman.h"

using namespace Network;


RequestCommunicator::RequestCommunicator( const char* servername, int port )
    : tcpsocket_( 0 )
    , servername_( servername )
    , serverport_( port )
    , connectionClosed( this )
    , packetArrived( this )
    , tcpserv_( 0 )
{
    connectToHost();
}


RequestCommunicator::RequestCommunicator( int port )
    : tcpsocket_( 0 )
    , serverport_( port )
    , connectionClosed( this )
    , packetArrived( this )
    , tcpserv_( 0 )
{
    startListening();
}


RequestCommunicator::~RequestCommunicator()
{
    deepErase( receivedpackets_ );

    if ( !tcpserv_ )
	deleteAndZeroPtr( tcpsocket_ );
    deleteAndZeroPtr( tcpserv_ );
}


void RequestCommunicator::connectToHost()
{
    Threads::MutexLocker locker( lock_ );

    if ( !tcpsocket_ )
	tcpsocket_ = new TcpSocket;

    if ( tcpsocket_->connectToHost(servername_,serverport_) )
	mAttachCB(tcpsocket_->disconnected,RequestCommunicator::connCloseCB);
}


void RequestCommunicator::startListening()
{
    deleteAndZeroPtr( tcpsocket_ );

    if ( !tcpserv_ )
    {
	tcpserv_ = new TcpServer;
	mAttachCB( tcpserv_->newConnection, RequestCommunicator::newConnectionCB );
    }

    tcpserv_->listen( 0, serverport_ );
}


void RequestCommunicator::newConnectionCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,socketid,cb);
    Threads::MutexLocker locker( lock_ );
    TcpSocket* sock = tcpserv_->getSocket(socketid);

    if ( tcpsocket_ )
    {
	if ( tcpsocket_->isConnected() )
	{
	    //I will only deal with one counterpart.
	    sock->disconnectFromHost();
	    return;
	}
	delete tcpsocket_;
    }

    tcpsocket_ = sock;
    tcpsocket_->setTimeout( 2000 );
    mAttachCB( tcpsocket_->readyRead, RequestCommunicator::dataArrivedCB );
}


bool RequestCommunicator::isOK() const
{
    if ( tcpserv_ )
    {
	if ( !tcpserv_->isListening() )
	    return false;

	return tcpsocket_ ? !tcpsocket_->isBad() : true;
    }

    return tcpsocket_ && !tcpsocket_->isBad();
}


void RequestCommunicator::connCloseCB( CallBacker* )
{
    connectionClosed.trigger();
    tcpserv_ = 0; tcpsocket_ = 0;
}


bool RequestCommunicator::readFromSocket()
{
    while ( tcpsocket_ )
    {
	PtrMan<RequestPacket> nextreceived = new RequestPacket;
	TcpSocket::ReadStatus readres = tcpsocket_->read( *nextreceived );
	if ( readres==TcpSocket::ReadError )
	{
	    tcpsocket_->disconnectFromHost();
	    return false;
	}
	else if ( readres==TcpSocket::ReadOK )
	{
	    const od_int32 receivedid = nextreceived->requestID();
	    const od_int16 receivesubid = nextreceived->subID();

	    Threads::MutexLocker locker ( lock_ );
	    if ( receivesubid==RequestPacket::cBeginSubID() ||
		 ourrequestids_.isPresent( receivedid ) )
	    {
		receivedpackets_ += nextreceived.release();
		locker.unLock();

		packetArrived.trigger(receivedid);
	    }
	}

	if ( !tcpsocket_->bytesAvailable() ) //Not sure this works
	    break;
    }

    return true;
}



bool RequestCommunicator::sendPacket( const RequestPacket& pkt )
{
    if ( !isOK() || !pkt.isOK() || (tcpserv_ && !tcpsocket_) )
	return false;

    const od_int32 reqid = pkt.requestID();
    const od_int16 subid = pkt.subID();

    Threads::MutexLocker locker( lock_ );
    if ( !ourrequestids_.isPresent( reqid ) )
    {
	if ( subid == RequestPacket::cBeginSubID() )
	    ourrequestids_ += reqid;
	else
	{
	    pErrMsg( BufferString("Packet send requested for unknown ID: ",reqid) );
	    return false;
	}
    }

    const bool result = tcpsocket_->write( pkt );
    //Should we wait for written?

    if ( !result || pkt.subID() == RequestPacket::cEndSubID() )
	requestEnded( pkt.requestID() );

    return true;
}


RequestPacket* RequestCommunicator::pickupPacket( od_int32 reqid, int timeout,
						  int* errorcode )
{
    Threads::MutexLocker locker( lock_ );
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
	do
	{
	    const int remaining = timeout - Time::getMilliSeconds() + starttm;
	    if ( remaining<=0 )
		break;

	    locker.unLock();
	    if ( !readFromSocket() )
	    {
		if ( errorcode ) *errorcode = cDisconnected();
		requestEnded( reqid );
		return 0;
	    }
	    locker.lock();

	    pkt = getNextAlreadyRead( reqid );

	} while ( !pkt );
	
	if ( !pkt )
	{
	    if ( errorcode )
		*errorcode = cTimeout();
	    return 0;
	}
    }

    if ( pkt->subID() == RequestPacket::cEndSubID()
	|| pkt->subID() < RequestPacket::cBeginSubID() )
	requestEnded( reqid );

    return pkt;
}


RequestPacket* RequestCommunicator::getNextAlreadyRead( int reqid )
{
    for ( int idx=0; idx<receivedpackets_.size(); idx++ )
    {
	if ( reqid < 0 || receivedpackets_[idx]->requestID()==reqid )
	    return receivedpackets_.removeSingle( idx );
    }

    return 0;
}


RequestPacket* RequestCommunicator::getNextExternalPacket()
{
    Threads::MutexLocker locker( lock_ );

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
    ourrequestids_ -= reqid;

    for ( int idx=receivedpackets_.size()-1; idx>=0; idx-- )
    {
	if ( receivedpackets_[idx]->requestID()==reqid )
	    delete receivedpackets_.removeSingle( idx );
    }
}


void RequestCommunicator::dataArrivedCB( CallBacker* cb )
{
    readFromSocket();
}
