/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "netreqconnection.h"

#include "applicationdata.h"
#include "netreqpacket.h"
#include "netsocket.h"
#include "netserver.h"
#include "timefun.h"
#include "ptrman.h"

using namespace Network;

static Threads::Atomic<int> connid;


RequestConnection::RequestConnection( const char* servername,
				      unsigned short servport,
				      bool haseventloop,
				      int timeout )
    : socket_( 0 )
    , ownssocket_( true )
    , servername_( servername )
    , serverport_( servport )
    , connectionClosed( this )
    , packetArrived( this )
    , id_( connid++ )
    , socketthread_( 0 )
    , timeout_( timeout )
    , stopflag_( 0 )
    , packettosend_( 0 )
{
    if ( haseventloop )
    {
	socketthread_ =
	    new Threads::Thread( mCB(this,RequestConnection,socketThreadFunc) );
    }
    else
    {
	connectToHost( haseventloop );
    }
}


RequestConnection::RequestConnection( Network::Socket* sock )
    : socket_( sock )
    , ownssocket_( false )
    , serverport_( mUdf(unsigned short) )
    , connectionClosed( this )
    , packetArrived( this )
    , id_( connid++ )
    , socketthread_( 0 )
    , timeout_( 0 )
    , stopflag_( 0 )
    , packettosend_( 0 )
{
    if ( !sock )
	return;

    mAttachCB(socket_->disconnected,RequestConnection::connCloseCB);
    mAttachCB(socket_->readyRead,RequestConnection::dataArrivedCB);
}


RequestConnection::~RequestConnection()
{
    if ( socketthread_ )
    {
	lock_.lock();
	stopflag_ = true;
	lock_.signal( true );
	lock_.unLock();

	socketthread_->waitForFinish();
	deleteAndZeroPtr( socketthread_ );
    }

    detachAllNotifiers();


    deepErase( receivedpackets_ );
}


void RequestConnection::socketThreadFunc( CallBacker* )
{
    if ( socket_ )
	return;

    connectToHost( true );

    lock_.lock();
    while ( !stopflag_ )
    {
	lock_.unLock();
	if ( socket_ && socket_->bytesAvailable() )
	{
	    readFromSocket();
	    lock_.lock();
	    continue;
	}

	lock_.lock();
	if ( packettosend_ && !sendingfinished_ )
	{
	    sendresult_ = doSendPacket( *packettosend_, sendwithwait_ );
	    sendingfinished_ = true;

	    lock_.signal( true );
	    continue;
	}

	lock_.wait();
    }

    deleteAndZeroPtr( socket_, ownssocket_ );
}


void RequestConnection::connectToHost( bool haseventloop )
{
    Threads::MutexLocker locker( lock_ );

    if ( !socket_ )
	socket_ = new Network::Socket( haseventloop );

    if ( timeout_ > 0 )
	socket_->setTimeout( timeout_ );

    if ( socket_->connectToHost(servername_,serverport_) )
	mAttachCB(socket_->disconnected,RequestConnection::connCloseCB);
}


bool RequestConnection::isOK() const
{
    return socket_ && !socket_->isBad();
}


void RequestConnection::connCloseCB( CallBacker* )
{
    connectionClosed.trigger();

    // deleteAndZeroPtr( socket_, ownssocket_ );
    // The socket_ will often contain a very bad pointer
    socket_ = 0;
}


void RequestConnection::flush()
{
}


bool RequestConnection::readFromSocket()
{
    while ( socket_ )
    {
	PtrMan<RequestPacket> nextreceived = new RequestPacket;
	Network::Socket::ReadStatus readres = socket_->read( *nextreceived );
	if ( readres==Network::Socket::ReadError )
	{
	    errmsg_ = socket_->errMsg();
	    socket_->disconnectFromHost();
	    if ( errmsg_.isEmpty() )
		errmsg_ = tr("Error reading from socket");
	    return false;
	}
	else if ( readres==Network::Socket::ReadOK )
	{
	    if ( !nextreceived->isOK() )
	    {
		socket_->disconnectFromHost();
		errmsg_ = tr("Garbled network packet received. Disconnected.");
		return false;
	    }

	    const od_int32 receivedid = nextreceived->requestID();
	    Threads::MutexLocker locker ( lock_ );
	    if ( nextreceived->isNewRequest() ||
		 ourrequestids_.isPresent( receivedid ) )
	    {
		receivedpackets_ += nextreceived.release();
		locker.unLock();

		packetArrived.trigger(receivedid);
	    }
	}

	if ( !socket_ || !socket_->bytesAvailable() ) //Not sure this works
	    break;
    }

    return true;
}


bool RequestConnection::doSendPacket( const RequestPacket& pkt,
				      bool waitforfinish )
{
    if ( !isOK() )
	return	false;

    const od_int32 reqid = pkt.requestID();

    if ( !ourrequestids_.isPresent( reqid ) )
    {
	if ( pkt.isNewRequest() )
	    ourrequestids_ += reqid;
	else
	{
	    pErrMsg(
		BufferString("Packet send requested for unknown ID: ",reqid) );
	    return false;
	}
    }

    const bool result = socket_->write( pkt, waitforfinish );

    if ( !result )
	requestEnded( pkt.requestID() );

    return result;
}



bool RequestConnection::sendPacket( const RequestPacket& pkt,
				    bool waitforfinish )
{
    if ( !pkt.isOK() )
	return false;

    Threads::MutexLocker locker( lock_ );
    //We're non-threaded. Just send
    if ( !socketthread_ )
    {
	return doSendPacket( pkt, waitforfinish );
    }

    //Someone else is sending now. Wait
    while ( packettosend_ )
	lock_.wait();

    //Setup your batch job and poke thread
    packettosend_ = &pkt;
    sendwithwait_ = waitforfinish;
    sendingfinished_ = false;
    lock_.signal( true );

    //Wait for sending to finish
    while ( !sendingfinished_ )
    {
	lock_.wait();
    }

    //Pickup sending result
    const bool result = sendresult_;

    //Tell someone who may be waiting that he can send now.
    packettosend_ = 0;
    lock_.signal( true );

    return result;
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
		if ( errorcode )
		    *errorcode = cDisconnected();
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

    if ( pkt->isRequestEnd() )
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


RequestServer::RequestServer( unsigned short servport )
    : serverport_( servport )
    , server_( new Network::Server )
    , newConnection( this )
{
    if ( !server_ )
	return;

    mAttachCB( server_->newConnection, RequestServer::newConnectionCB );
    if ( !server_->listen( 0, serverport_ ) )
    {
	errmsg_ = tr("Cannot start listening on port %1").arg( serverport_ );
    }
}


RequestServer::~RequestServer()
{
    detachAllNotifiers();

    deepErase( pendingconns_ );
    deleteAndZeroPtr( server_ );
}


bool RequestServer::isOK() const
{
    return server_ && server_->isListening();
}



RequestConnection* RequestServer::pickupNewConnection()
{
    Threads::Locker locker( lock_ );
    return pendingconns_.size() ? pendingconns_.removeSingle( 0 ) : 0;
}


void RequestServer::newConnectionCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,socketid,cb);
    Network::Socket* sock = server_->getSocket(socketid);

    if ( !sock )
	return;

    if ( !sock->isConnected() || sock->isBad() )
    {
	sock->disconnectFromHost();
	return;
    }

    Threads::Locker locker( lock_ );
    pendingconns_ += new RequestConnection( sock );
    locker.unlockNow();

    newConnection.trigger();
}
