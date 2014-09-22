/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "netreqconnection.h"

#include "netreqpacket.h"
#include "tcpsocket.h"
#include "tcpserver.h"
#include "timefun.h"
#include "ptrman.h"

using namespace Network;


RequestConnection::RequestConnection( const char* servername,
				      unsigned short port )
    : tcpsocket_( 0 )
    , ownssocket_( true )
    , servername_( servername )
    , serverport_( port )
    , connectionClosed( this )
    , packetArrived( this )
{
    connectToHost();
}


RequestConnection::RequestConnection( TcpSocket* socket )
    : tcpsocket_( socket )
    , ownssocket_( false )
    , serverport_( mUdf(unsigned short) )
    , connectionClosed( this )
    , packetArrived( this )
{
    if ( !socket )
	return;

    mAttachCB(tcpsocket_->disconnected,RequestConnection::connCloseCB);
}


RequestConnection::~RequestConnection()
{
    deepErase( receivedpackets_ );

    deleteAndZeroPtr( tcpsocket_, ownssocket_ );
}


void RequestConnection::connectToHost()
{
    Threads::MutexLocker locker( lock_ );

    if ( !tcpsocket_ )
	tcpsocket_ = new TcpSocket;

    if ( tcpsocket_->connectToHost(servername_,serverport_) )
	mAttachCB(tcpsocket_->disconnected,RequestConnection::connCloseCB);
}


bool RequestConnection::isOK() const
{
    return tcpsocket_ && !tcpsocket_->isBad();
}


void RequestConnection::connCloseCB( CallBacker* )
{
    connectionClosed.trigger();

    deleteAndZeroPtr( tcpsocket_, ownssocket_ );
}


bool RequestConnection::readFromSocket()
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



bool RequestConnection::sendPacket( const RequestPacket& pkt )
{
    if ( !isOK() || !pkt.isOK() || !tcpsocket_ )
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
	    pErrMsg(
		BufferString("Packet send requested for unknown ID: ",reqid) );
	    return false;
	}
    }

    const bool result = tcpsocket_->write( pkt );
    //Should we wait for written?

    if ( !result || pkt.subID() == RequestPacket::cEndSubID() )
	requestEnded( pkt.requestID() );

    return true;
}


RequestPacket* RequestConnection::pickupPacket( od_int32 reqid, int timeout,
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

    if ( pkt->subID()==RequestPacket::cEndSubID() ||
	 pkt->subID()<RequestPacket::cBeginSubID() )
	requestEnded( reqid );

    return pkt;
}


RequestPacket* RequestConnection::getNextAlreadyRead( int reqid )
{
    for ( int idx=0; idx<receivedpackets_.size(); idx++ )
    {
	if ( reqid < 0 || receivedpackets_[idx]->requestID()==reqid )
	    return receivedpackets_.removeSingle( idx );
    }

    return 0;
}


RequestPacket* RequestConnection::getNextExternalPacket()
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


void RequestConnection::requestEnded( od_int32 reqid )
{
    ourrequestids_ -= reqid;

    for ( int idx=receivedpackets_.size()-1; idx>=0; idx-- )
    {
	if ( receivedpackets_[idx]->requestID()==reqid )
	    delete receivedpackets_.removeSingle( idx );
    }
}


void RequestConnection::dataArrivedCB( CallBacker* cb )
{
    readFromSocket();
}


RequestServer::RequestServer( unsigned short port )
    : serverport_( port )
    , tcpserv_( new TcpServer )
    , newConnection( this )
{
    if ( !tcpserv_ )
	return;

    mAttachCB( tcpserv_->newConnection, RequestServer::newConnectionCB );
    tcpserv_->listen( 0, serverport_ );
}


RequestServer::~RequestServer()
{
    detachAllNotifiers();

    deepErase( pendingconns_ );
    deleteAndZeroPtr( tcpserv_ );
}


bool RequestServer::isOK() const
{
    return tcpserv_ && tcpserv_->isListening();
}



RequestConnection* RequestServer::pickupNewConnection()
{
    Threads::Locker locker( lock_ );
    return pendingconns_.size() ? pendingconns_.removeSingle( 0 ) : 0;
}


void RequestServer::newConnectionCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,socketid,cb);
    TcpSocket* sock = tcpserv_->getSocket(socketid);

    if ( !sock )
	return;

    if ( sock->isConnected() && !sock->isBad() )
    {
	sock->disconnectFromHost();
	return;
    }

    Threads::Locker locker( lock_ );
    pendingconns_ += new RequestConnection( sock );
    locker.unlockNow();

    newConnection.trigger();
}
