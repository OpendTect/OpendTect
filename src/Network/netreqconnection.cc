/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/

#include "netreqconnection.h"

#include "applicationdata.h"
#include "netreqpacket.h"
#include "netsocket.h"
#include "netserver.h"
#include "timefun.h"
#include "ptrman.h"

#ifndef OD_NO_QT
# include <QObject>
# include <QCoreApplication>
# include "qtcpsocketcomm.h"
#endif

namespace Network
{

class RequestConnectionSender : public QObject
{
public:
    RequestConnectionSender( RequestConnection& conn )
	: conn_( conn )
    {}

    void addToQueue( RequestConnection::PacketSendData* psd )
    {
	lock_.lock();
	queue_ += psd;
	lock_.unLock();
	QCoreApplication::postEvent( this, new QEvent(QEvent::None) );
    }

    bool event( QEvent* ) { sendQueue(); return true; }

    void sendQueue()
    {
	lock_.lock();
	RefObjectSet<RequestConnection::PacketSendData> localqueue = queue_;
	queue_.erase();
	lock_.unLock();

	for ( int idx=0; idx<localqueue.size(); idx++ )
	{
	    localqueue[idx]->trySend( conn_ );
	}
    }

    Threads::SpinLock					lock_;
    RefObjectSet<RequestConnection::PacketSendData>	queue_;
    RequestConnection&					conn_;
};

}; //Network namespace



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
    , packetsender_( 0 )
    , eventloop_( 0 )
    , timeout_( timeout )
{
    if ( multithreaded )
    {
	Threads::MutexLocker locker( lock_ );
	socketthread_ =
	    new Threads::Thread( mCB(this,RequestConnection,socketThreadFunc),
				 "RequestConnection socket thread" );
	while ( !eventloop_ )
	    lock_.wait(); //Wait for thread to create connection.
    }
    else
    {
	 connectToHost( false );
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
    , packetsender_( 0 )
    , eventloop_( 0 )
    , timeout_( 0 )
{
    if ( !sock )
	return;

    mAttachCB(socket_->disconnected,RequestConnection::connCloseCB);
    mAttachCB(socket_->readyRead,RequestConnection::dataArrivedCB);
}


RequestConnection::~RequestConnection()
{
    detachAllNotifiers();

    if ( eventloop_ )
    {
	eventloop_->exit();

	socketthread_->waitForFinish();
	deleteAndZeroPtr( socketthread_ );
    }
    else
    {
	deleteAndZeroPtr( socket_, ownssocket_ );
    }

    deepErase( receivedpackets_ );
}


RequestConnection::PacketSendData::PacketSendData( const RequestPacket& packet,
						   bool wait )
    : packet_( packet )
    , waitforfinish_( wait )
    , sendstatus_( NotAttempted )
{}


void RequestConnection::PacketSendData::trySend( RequestConnection& conn )
{
    if ( sendstatus_!=NotAttempted )   //Threadsafe as only I will set it
				       //apart from constructor
	return;

    conn.lock_.lock();

    sendstatus_ = conn.doSendPacket( packet_, waitforfinish_ )
	 ? Sent
	 : Failed;

    conn.lock_.signal( true );
    conn.lock_.unLock();
}


void RequestConnection::socketThreadFunc( CallBacker* )
{
    if ( socket_ )
    {
	pErrMsg("Thread started with existing socket!");
	return;
    }

    connectToHost( true );

    mAttachCB(socket_->disconnected,RequestConnection::connCloseCB);
    mAttachCB(socket_->readyRead,RequestConnection::dataArrivedCB);
    eventloop_ = new QEventLoop( socket_->qSocket() );
    packetsender_ = new RequestConnectionSender( *this );

    //Tell constructor we are up and running!
    lock_.lock();
    lock_.signal( true );
    lock_.unLock();

    eventloop_->exec();

    deleteAndZeroPtr( eventloop_ );
    deleteAndZeroPtr( packetsender_ );
    deleteAndZeroPtr( socket_, ownssocket_ );
}


void RequestConnection::connectToHost( bool witheventloop )
{
    if ( socket_ )
    {
	pErrMsg("I did not expect a socket" );
	return;
    }

    Network::Socket* newsocket = new Network::Socket( witheventloop );

    if ( timeout_ > 0 )
	newsocket->setTimeout( timeout_ );

    if ( newsocket->connectToHost(servername_,serverport_) )
	mAttachCB(newsocket->disconnected,RequestConnection::connCloseCB);

    Threads::MutexLocker locker( lock_ );
    socket_ = newsocket;
}


bool RequestConnection::isOK() const
{
    return socket_ && !socket_->isBad();
}


bool RequestConnection::stillTrying() const
{
    //TODO
    return false;
}


void RequestConnection::connCloseCB( CallBacker* )
{
    lock_.lock();
    lock_.signal(true);  //isOK has changed
    lock_.unLock();

    connectionClosed.trigger();
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

    //We're non-threaded. Just send
    if ( !socketthread_ )
    {
	return doSendPacket( pkt, waitforfinish );
    }

    RefMan<PacketSendData> senddata = new PacketSendData(pkt,waitforfinish);
    packetsender_->addToQueue( senddata );

    if ( !waitforfinish )
	return true;

    while ( !socket_
	    && senddata->sendstatus_==PacketSendData::NotAttempted )
    {
	lock_.wait();
    }

    return senddata->sendstatus_==PacketSendData::Sent;
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
