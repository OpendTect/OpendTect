/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2009
________________________________________________________________________

-*/

#include "netreqconnection.h"

#include "netreqpacket.h"
#include "netserver.h"
#include "netsocket.h"
#include "timefun.h"
#include "uistrings.h"

#include <QEventLoop>
#include <QTcpSocket>

using namespace Network;

#ifdef __win__
#include <iphlpapi.h>
#pragma comment( lib, "iphlpapi" )
#endif

namespace Network
{

struct PacketSendData : public RefCount::Referenced
{
    PacketSendData(const RequestPacket&,bool wait);
    ConstRefMan<RequestPacket>  packet_;
    bool                        waitforfinish_;

    enum SendStatus             { NotAttempted, Sent, Failed };
    SendStatus			sendstatus_;
				//Protected by connections' lock_
};


static Threads::Atomic<int> connid;


RequestConnection::RequestConnection( const Authority& authority,
				      bool multithreaded, int timeout )
    : socket_( nullptr )
    , ownssocket_( true )
    , connectionClosed( this )
    , packetArrived( this )
    , id_( connid++ )
    , authority_(new Authority(authority))
    , timeout_( timeout )
{
    if ( multithreaded )
    {
	socketthread_ =
	    new Threads::Thread( mCB(this,RequestConnection,socketThreadFunc),
				 "RequestConnection socket thread" );

	Threads::ConditionVar eventlooplock;
        eventlooplock.lock();
	eventlooplock_ = &eventlooplock;
	while ( !eventloop_ )
	    eventlooplock.wait(); //Wait for thread to create connection.
        eventlooplock.unLock();
	eventlooplock_ = nullptr;
    }
    else
    {
	 connectToHost( false );
    }
}


RequestConnection::RequestConnection( Socket* sock )
    : socket_( sock )
    , ownssocket_( false )
    , connectionClosed( this )
    , packetArrived( this )
    , id_( connid++ )
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

    delete authority_;
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

    if ( !receivedpackets_.isEmpty() )
    {
	pErrMsg("Received packets should be empty");
    }

    if ( sendqueue_.size() )
    {
        pErrMsg("Queue should be empty");
    }
}


PacketSendData::PacketSendData( const RequestPacket& packet, bool wait )
    : packet_( &packet )
    , waitforfinish_( wait )
    , sendstatus_( NotAttempted )
{}


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
    QEventLoop* eventloop = new QEventLoop( socket_->qSocket() );

    createReceiverForCurrentThread();


    //Tell constructor we are up and running!
    eventlooplock_->lock();
    eventloop_ = eventloop;
    eventlooplock_->signal( true );
    eventlooplock_->unLock();

    eventloop_->exec();


    //Go through send queue and make sure eventual waiting threads are
    //notified
    lock_.lock();

    for ( int idx=0; idx<sendqueue_.size(); idx++ )
	sendqueue_[idx]->sendstatus_ = PacketSendData::Failed;

    lock_.signal( true );

    deepUnRef( sendqueue_ );
    lock_.unLock();

    removeReceiverForCurrentThread();
    deleteAndZeroPtr( eventloop_ );
    deleteAndZeroPtr( socket_, ownssocket_ );
}


void RequestConnection::connectToHost( bool witheventloop )
{
    if ( socket_ )
    {
	pErrMsg("I did not expect a socket" );
	return;
    }

    if ( !authority_ )
    {
	pErrMsg("I did not expect no authority" );
	return;
    }

    auto* newsocket = new Socket( authority_->isLocal(), witheventloop );

    if ( timeout_ > 0 )
	newsocket->setTimeout( timeout_ );

    if ( newsocket->connectToHost(*authority_,true) )
	mAttachCB(newsocket->disconnected,RequestConnection::connCloseCB);

    Threads::MutexLocker locker( lock_ );
    socket_ = newsocket;
}


bool RequestConnection::isOK() const
{
    if ( !socket_ )
	return false;

    const bool badsocket = socket_->isBad();
    if ( badsocket && !socket_->errMsg().isEmpty() )
	errmsg_.append( socket_->errMsg(), true );

    return !badsocket;
}


BufferString RequestConnection::server() const
{
    return authority_ ? authority_->getHost() : BufferString::empty();
}


PortNr_Type RequestConnection::port() const
{
    return authority_ ? authority_->getPort() : 0;
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
	RefMan<RequestPacket> nextreceived = new RequestPacket;
	Socket::ReadStatus readres = socket_->read( *nextreceived );

	if ( readres==Socket::ReadOK )
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
		 ourrequestids_.isPresent(receivedid) )
	    {
		receivedpackets_ += nextreceived.release();
		lock_.signal( true );
		locker.unLock();

		packetArrived.trigger(receivedid);
	    }
	    else
	    {
		pErrMsg("Invalid packet arrived");
	    }
	}
	else //Error or timeout
	{
	    errmsg_ = socket_->errMsg();
	    socket_->disconnectFromHost();
	    if ( errmsg_.isEmpty() )
		errmsg_ = uiStrings::phrErrDuringRead( tr("socket","network") );
	    return false;
	}

	//Break the loop when there is nothing left to read
	if ( !socket_->bytesAvailable() )
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


void RequestConnection::sendQueueCB(CallBacker*)
{
    lock_.lock();
    RefObjectSet<PacketSendData> localqueue = sendqueue_;
    deepUnRef( sendqueue_ );
    lock_.unLock();

    for ( int idx=0; idx<localqueue.size(); idx++ )
    {
	if ( localqueue[idx]->sendstatus_!=PacketSendData::NotAttempted )
	   //Threadsafe as only I will set it
	   //apart from constructor
	    continue;

	lock_.lock();

	localqueue[idx]->sendstatus_ = doSendPacket( *localqueue[idx]->packet_,
					       localqueue[idx]->waitforfinish_ )
	     ? PacketSendData::Sent
	     : PacketSendData::Failed;

	lock_.signal( true );
	lock_.unLock();
    }
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
    sendqueue_ += senddata;
    senddata->ref(); //Class assumes all objects in sendqueue is reffed

    //Trigger thread if I'm first. If size is larger, it should already be
    //triggered
    if ( sendqueue_.size()==1 )
    {
	CallBack::addToThread( socketthread_->threadID(),
			   mCB(this, RequestConnection, sendQueueCB) );
    }

    if ( !waitforfinish )
	return true;

    while ( !socket_
	    && senddata->sendstatus_==PacketSendData::NotAttempted )
    {
	lock_.wait();
    }

    return senddata->sendstatus_==PacketSendData::Sent;
}


RefMan<RequestPacket> RequestConnection::pickupPacket( od_int32 reqid,
						int timeout,
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

    RefMan<RequestPacket> pkt = getNextAlreadyRead( reqid );
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


RefMan<RequestPacket> RequestConnection::getNextAlreadyRead( int reqid )
{
    for ( int idx=0; idx<receivedpackets_.size(); idx++ )
    {
	RefMan<RequestPacket> pkt = receivedpackets_[idx];
	if ( reqid<0 || pkt->requestID()==reqid )
	{
	    receivedpackets_.removeSingle( idx );
	    pkt->unRef();
	    return pkt;
	}
    }

    return nullptr;
}


RefMan<RequestPacket> RequestConnection::getNextExternalPacket()
{
    Threads::MutexLocker locker( lock_ );

    for ( int idx=0; idx<receivedpackets_.size(); idx++ )
    {
	RefMan<RequestPacket> pkt = receivedpackets_[idx];
	if ( !ourrequestids_.isPresent(pkt->requestID()) )
	{
	    receivedpackets_.removeSingle(idx);
	    pkt->unRef();
	    return pkt;
	}
    }

    return nullptr;
}


void RequestConnection::requestEnded( od_int32 reqid )
{
    ourrequestids_ -= reqid;

    for ( int idx=receivedpackets_.size()-1; idx>=0; idx-- )
    {
	if ( receivedpackets_[idx]->requestID()==reqid )
	    receivedpackets_.removeSingle( idx );
    }
}


void RequestConnection::dataArrivedCB( CallBacker* cb )
{
    readFromSocket();
}



RequestServer::RequestServer( PortNr_Type servport, SpecAddr specaddr )
    : server_(new Server(false))
    , newConnection( this )
{
    if ( !server_ )
	return;

    mAttachCB( server_->newConnection, RequestServer::newConnectionCB );
    if ( !server_->listen(specaddr,servport) )
	errmsg_ = TCPErrMsg().arg( servport );
}


RequestServer::RequestServer( const char* servernm )
    : server_(new Server(true))
    , newConnection(this)
{
    if ( !server_ )
	return;

    mAttachCB( server_->newConnection, RequestServer::newConnectionCB );
    uiRetVal ret;
    if ( !server_->listen(servernm,ret) )
    {
	errmsg_ = LocalErrMsg().arg( servernm );
	errmsg_.append( ret, true );
    }
}


RequestServer::RequestServer( const Network::Authority& auth )
    : newConnection(this)
{
    server_ = new Server( auth.isLocal() );
    if ( !server_ )
	return;

    mAttachCB( server_->newConnection, RequestServer::newConnectionCB );
    const bool islocal = auth.isLocal();
    uiRetVal ret;
    const bool islistening = islocal
			   ? server_->listen( auth.getServerName(), ret )
			   : server_->listen( auth.serverAddress(),
					      auth.getPort() );
    if ( !islistening )
    {
	errmsg_ = islocal
	    ? LocalErrMsg().arg(auth.getServerName()).append(ret, true)
	    : TCPErrMsg().arg(auth.getPort());
    }
}


RequestServer::~RequestServer()
{
    detachAllNotifiers();

    deepErase( pendingconns_ );
    delete server_;
}


uiString RequestServer::TCPErrMsg() const
{
    return tr("Cannot start listening on port %1","port id<number>");
}


uiString RequestServer::LocalErrMsg() const
{
    return tr("Cannot start listening to %1","server name");
}


bool RequestServer::isOK() const
{
    return server_ && server_->isListening();
}


Authority RequestServer::getAuthority() const
{
    return server_ ? server_->authority() : Authority();
}



RequestConnection* RequestServer::pickupNewConnection()
{
    Threads::Locker locker( lock_ );
    return pendingconns_.size() ? pendingconns_.removeSingle( 0 ) : 0;
}


void RequestServer::newConnectionCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,socketid,cb);
    auto* sock = server_->getSocket(socketid);

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

#ifdef __win__
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
#endif

bool isPortFree( PortNr_Type port, uiString* errmsg )
{
#ifdef __win__
    PMIB_TCPTABLE pTcpTable;
    pTcpTable = (MIB_TCPTABLE *) MALLOC(sizeof (MIB_TCPTABLE));
    if ( !pTcpTable )
	return true;
    DWORD dwSize = sizeof (MIB_TCPTABLE);
    DWORD dwRetVal = GetTcpTable( pTcpTable, &dwSize, TRUE);
    if ( dwRetVal == ERROR_INSUFFICIENT_BUFFER )
    {
        FREE(pTcpTable);
        pTcpTable = (MIB_TCPTABLE *) MALLOC(dwSize);
        if ( !pTcpTable )
	    return true;
    }
    dwRetVal = GetTcpTable( pTcpTable, &dwSize, TRUE );
    if ( dwRetVal != NO_ERROR )
    {
	FREE(pTcpTable);
	return true;
    }
    const int nrentries = pTcpTable->dwNumEntries;
    bool isfound = false;
    for ( int idx=0; idx<nrentries; idx++ )
    {
	const auto entry = pTcpTable->table[idx];
	if ( entry.dwState != MIB_TCP_STATE_LISTEN )
	    continue;
	const auto localport = ntohs((u_short)entry.dwLocalPort);
	if ( localport == port )
	{
	    isfound = true;
	    break;
	}
    }

    if ( pTcpTable != NULL )
    {
	FREE(pTcpTable);
	pTcpTable = NULL;
    }

    return !isfound;
#else
    const RequestServer reqserv( port );
    const bool ret = reqserv.isOK();
    if ( errmsg && !reqserv.errMsg().isEmpty() )
	errmsg->set( reqserv.errMsg() );

    return ret;
#endif
}

#ifdef __win__
#undef MALLOC
#undef FREE
#endif

}; //Network
