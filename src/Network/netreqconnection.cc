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
				      bool multithreaded,
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
    , threadreadstatus_( None )
    , triggerread_( false )
{
    if ( multithreaded )
    {
	Threads::MutexLocker locker( lock_ );
	socketthread_ =
	    new Threads::Thread( mCB(this,RequestConnection,socketThreadFunc),
				 "RequestConnection socket thread" );
	lock_.wait(); //Wait for thread to create connection.
    }
    else
    {
	 connectToHost();
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
    , threadreadstatus_( None )
    , triggerread_( false )
{
    if ( !sock )
	return;

    mAttachCB(socket_->disconnected,RequestConnection::connCloseCB);
    mAttachCB(socket_->readyRead,RequestConnection::dataArrivedCB);
}


RequestConnection::~RequestConnection()
{
    detachAllNotifiers();

    if ( socketthread_ )
    {
	lock_.lock();
	stopflag_ = true;
	lock_.signal( true );
	lock_.unLock();

	socketthread_->waitForFinish();
	deleteAndZeroPtr( socketthread_ );
    }
    else
    {
	deleteAndZeroPtr( socket_, ownssocket_ );
    }

    deepErase( receivedpackets_ );
}


void RequestConnection::socketThreadFunc( CallBacker* )
{
    if ( socket_ )
    {
	pErrMsg("Thread started with existing socket!");
	return;
    }

    connectToHost();

    lock_.lock();
    while ( !stopflag_ )
    {
	lock_.unLock();
	const bool hasdata = socket_->bytesAvailable();
	lock_.lock();

	if ( triggerread_ || hasdata )
	{
	    lock_.unLock();
	    readFromSocket();
	    lock_.lock();
	    triggerread_ = false;
	    lock_.signal( true );
	    continue;
	}

	if ( packettosend_ && !sendingfinished_ )
	{
	    sendresult_ = doSendPacket( *packettosend_, sendwithwait_ );
	    sendingfinished_ = true;

	    lock_.signal( true );
	    continue;
	}

	//Wait 100 ms. We should in principle be able to wait infinitly
	//if everything is 100% correct. This is however safer
	lock_.wait( 100 );
    }

    deleteAndZeroPtr( socket_, ownssocket_ );
}


void RequestConnection::connectToHost()
{
    if ( socket_ )
    {
	pErrMsg("I did not expect a socket" );
	return;
    }

    Network::Socket* newsocket = new Network::Socket( false );

    if ( timeout_ > 0 )
	newsocket->setTimeout( timeout_ );

    if ( newsocket->connectToHost(servername_,serverport_) )
	mAttachCB(newsocket->disconnected,RequestConnection::connCloseCB);

    Threads::MutexLocker locker( lock_ );

    socket_ = newsocket;
    //Tell eventual constructor waiting that we have at least tried to connect
    lock_.signal( true );
}


bool RequestConnection::isOK() const
{
    return socket_ && !socket_->isBad();
}


void RequestConnection::connCloseCB( CallBacker* )
{
    connectionClosed.trigger();
}


void RequestConnection::flush()
{
}


bool RequestConnection::readFromSocket()
{
    while ( isOK() )
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
		lock_.signal( true );
		locker.unLock();

		packetArrived.trigger(receivedid);
	    }
	}

	if ( !socket_->bytesAvailable() ) //Not sure this works
	    return true;
    }

    return false;
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

    lock_.unLock();
    const bool result = socket_->write( pkt, waitforfinish );
    lock_.lock();

    if ( !result )
	requestEnded( pkt.requestID() );

    return result;
}



bool RequestConnection::sendPacket( const RequestPacket& pkt,
				    bool waitforfinish )
{
    if ( !pkt.isOK() )
	return false;

    if ( !socketthread_ && Threads::currentThread()!=socket_->thread() )
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
    if ( !socketthread_ && Threads::currentThread()!=socket_->thread() )
	return 0;

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
	bool isok = false;
	do
	{
	    const int remaining = timeout - Time::getMilliSeconds() + starttm;
	    if ( remaining<=0 )
		break;

	    if ( !socketthread_ )
	    {
		locker.unLock();
		if( !readFromSocket() )
		{
		    if ( errorcode )
			*errorcode = isOK() ? cTimeout() : cDisconnected();
		    locker.lock();
		    requestEnded( reqid );
		    return 0;
		}

		isok = isOK();

		locker.lock();
	    }
	    else
	    {
		triggerread_ = true;
		lock_.signal( true );
		lock_.wait( remaining );

		locker.unLock();
		isok = isOK();
		locker.lock();
	    }

	    pkt = getNextAlreadyRead( reqid );

	} while ( !pkt && isok );

	if ( !pkt )
	{
	    if ( errorcode )
		*errorcode = isok ? cTimeout() : cDisconnected();
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
